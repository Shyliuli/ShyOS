# Result

Result 是 `03basic` 的正式拥有型返回容器。

C 使用 `ResultT`。`Ok(T)` 携带值，`Err(i32)` 携带正数错误码，`None()` 是
`Err(ERROR_NO_VALUE)` 的语法糖。基础标量类型由 `result.h` 内建，owned 类型在各自
模块中使用 `IMPL_SHY_OWNED_RESULT` 实例化。

`IMPL_SHY_OWNED_RESULT(T, dropfn, clonefn)` 同时生成 `T` 的 `TypeDesc`、声明
`ResultT` 布局并生成 owned 方法，不需要额外调用 `Impl_Type_Desc` 或
`DECLARE_SHY_RESULT(T)`。drop 回调签名为 `void(void *)`，clone 回调签名为
`void *(const void *)`，且必须在宏调用前完成声明。

Rust 使用 `CResult<T>`，并可直接匹配 `CResult::Ok(value)` 和
`CResult::Err(error)`。`none()` 返回 `Err(ERROR_NO_VALUE)`。

`c_try!(result)` 在 `Ok` 时取出值，在 `Err` 时从当前 `CResult` 返回函数原样传播
错误码，不依赖自定义 `Try` trait。
