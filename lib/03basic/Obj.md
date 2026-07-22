# ShyOS Obj：C 所有权约定

ShyOS Obj 是建立在 C ABI 之上的轻量所有权约定。它不改变 C 的对象布局，也不尝试从
对象内存猜测类型：对象仍然是普通 `struct`，类型和生命周期信息由创建者以
`TypeDesc` 显式传给容器。

## 1. 基本原则

C 的赋值、按值传参、按值返回和 `memcpy` 都只是字节复制。编译器不会因为一个
结构体包含堆指针，就自动运行 clone 或 drop。因此 Obj 只定义两类值：

- copy：没有析构函数，按位复制后源和目标都可继续使用；
- owned：具有析构函数，复制必须显式 clone，转移必须 move。

Obj 不在对象中放 MAGIC、类型编号或 vtable。`Vec` 等泛型组件在创建时得到一份
`TypeDesc`，之后始终使用这份描述符处理元素。

## 2. TypeDesc

```c
typedef struct TypeDesc {
    usize size;
    void (*drop)(void *self);
    void *(*clone)(const void *src);
} TypeDesc;
```

分类只看 `drop`：

```c
is_copy(Type(T));
is_owned(OwnedType(String));
```

`Type(T)` 现场生成普通类型的描述符；`OwnedType(T)` 获取由
`Impl_Type_Desc` 注册的 owned 描述符。

owned 类型必须满足：

1. 全零表示合法的 empty/moved-from 状态；
2. drop 可以安全接收全零对象，并把已释放对象恢复为全零；
3. clone 返回由当前 allocator 分配的新对象指针，失败返回 NULL；
4. 返回对象是独立所有者，不能只复制资源指针。

clone 是显式操作。容器的 `push` 不会暗中 clone：需要副本时，调用者先调用
`string_clone()`、`vec_clone()` 等类型接口，再把得到的临时所有者 move 进容器。
泛型容器递归 clone 时，会把 `TypeDesc.clone` 返回的临时对象 move 到内嵌槽位，再释放
临时对象本身的外壳。

## 3. copy 与 owned 类型注册

普通类型无需静态注册：

```c
TypeDesc number_type = Type(i32);
```

如果普通类型也要用于 `let(T)`，使用 `Impl_Drop(T)` 生成空 cleanup：

```c
Impl_Drop(MyPoint)
```

owned 类型如果只需要 `TypeDesc`，注册自己的 drop 和 clone：

```c
Impl_Type_Desc(MyType, my_type_drop, my_type_clone_obj)
```

注册后可写：

```c
TypeDesc string_type = OwnedType(String);
```

## 4. move、clone 与 take

Obj 的值操作只有两种：

- clone：copy 类型执行 `memcpy`；owned 类型调用描述符中的 clone；源保持有效；
- move：复制对象字节；owned 类型随后把源对象整体清零；源进入 moved-from 状态。

`take`、`pop`、`remove` 都是 move 在容器位置上的名字。容器把元素移出后，原槽位不再
属于活跃元素，因此不会再 drop。

按值返回普通局部变量在 C 中可以承担 move：函数返回后局部副本自然消失，C 不会自动
析构它。但带 cleanup 的 `let` 变量不能直接返回，否则返回值复制完成后 cleanup 会释放
同一份资源。

## 5. var 与 let

`shy_type.h` 提供：

```c
var number = 42;
```

`var` 使用 C23 `auto` 或 GNU/Clang `__auto_type` 做局部类型推断。

`obj.h` 提供：

```c
let(String) name = String_unwrap(&result);
```

`let(T)` 使用 GCC/Clang cleanup，在作用域结束时调用 `T##_autodrop`。约束是：

1. 一条 `let` 只声明一个变量；
2. 必须立即初始化；
3. 不得直接返回 `let` 变量；
4. move 后源已经清零，作用域结束时再次 drop 是空操作。

`longjmp` 不运行 cleanup。Obj 不用运行时机制掩盖这一事实。

## 6. Result 的单一所有权

`ResultT` 和 Rust `CResult<T>` 都是拥有型返回容器。正式状态只有 `Ok(T)` 和
`Err(i32)`；`None()` / `none()` 是 `Err(ERROR_NO_VALUE)` 的语法糖。`unwrap`、
`take` 与 `replace` 消费或移出内部值，Result 随后变为 no-value error。

C owned Result 使用 `IMPL_SHY_OWNED_RESULT(T, dropfn, clonefn)` 一次生成
`T` 的 `TypeDesc`、`ResultT` 布局并注册递归 clone/drop；不需要额外调用
`Impl_Type_Desc` 或 `DECLARE_SHY_RESULT`。owned drop 回调统一使用
`void drop(void *)`，clone 回调使用 `void *clone(const void *)`。
Rust `CResult<T>` 由 enum 自动只析构 `Ok(T)`。Result 不允许通过只读 `unwrap`
复制 owned 值并留下第二个所有者。

## 7. Vec 的递归所有权

`Vec` 按值保存元素描述符：

```c
typedef struct Vec {
    void *data;
    usize cap;
    usize size;
    TypeDesc elem;
} Vec;
```

创建方式：

```c
var numbers_result = vec_new(Type(i32));
var strings_result = vec_new(OwnedType(String));
var nested_result = vec_new(OwnedType(Vec));

Vec numbers = Vec_unwrap(&numbers_result);
Vec strings = Vec_unwrap(&strings_result);
Vec nested = Vec_unwrap(&nested_result);
```

`vec_push()` 的规则：

- copy 元素：复制元素，源保持不变；
- owned 元素：复制元素后清零源，Vec 接管所有权。

`vec_pop()`、`vec_remove()` 和 `vec_swap_remove()` 使用同一 move 规则把元素写入
调用者提供的输出位置；owned 源槽位在移出时清零，不保留第二个所有者。

`vec_clone()` 是显式深复制。copy 元素整段复制；owned 元素逐个调用 `elem.clone`，
把返回的新对象 move 到目标槽位，再释放临时对象外壳。因此 `Vec<Vec<String>>` 的
clone/drop 会沿每个内层 Vec 保存的 `TypeDesc` 自然递归。

## 8. Rust 包装边界

Rust wrapper 仍使用同一 C `Vec` 存储，但不能让 C 把任意 Rust `T` 清零：某些 Rust
类型的全零位模式无效。Rust 的 `CVec<T>::push(T)` 使用 `ManuallyDrop`，再调用不清零源
的 raw-move C 入口；目标 Vec 仍保存 Rust 的 drop callback，在最终析构时正确 drop T。

这不是第二套所有权模型，而是同一 move 语义在 Rust 类型有效性规则下的实现边界。

## 9. 纪律

- 不对 owned 类型做裸赋值后同时保留两个所有者；
- `push` 表示转移，`get` 表示借用，`pop/remove/take` 表示移出；
- clone 必须由调用者显式写出；
- moved-from 对象必须保持全零并允许重复 drop；
- clone 目标必须是未初始化或已清理的内存；
- 容器创建时传入的 TypeDesc 必须与实际元素类型一致；
- 可使用 `clang --analyze` 检查 cleanup 变量返回、未初始化 cleanup 等常见违约。

Obj 的目标不是把 C 伪装成 Rust，而是让所有权转移、深复制和析构在 C 接口中有统一、
可审查的表达方式。
