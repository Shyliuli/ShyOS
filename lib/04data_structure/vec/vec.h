#ifndef SHYOS_VEC_H
#define SHYOS_VEC_H

#include "obj.h"
#include "result.h"
#include "shy_type.h"

typedef struct Vec Vec;
struct Vec {
	void *data;
	usize cap;
	usize size;
	TypeDesc elem;
};

/* owned 类型回调。clone 返回 allocator 分配的 Vec 对象。 */
void *vec_clone_obj(const void *source);

/* 释放全部活跃元素和存储，并把 Vec 清零。 */
void vec_drop(void *self);

IMPL_SHY_OWNED_RESULT(Vec, vec_drop, vec_clone_obj)

/* 创建一个空 Vec；TypeDesc 由调用者显式声明。 */
ResultVec vec_new(TypeDesc elem);

/* 接管已由当前 allocator 分配并初始化的元素数组。 */
Vec vec_from_raw_parts(void *data, usize size, usize cap, TypeDesc elem);

/* 显式深复制；不可 clone 的 owned 元素或分配失败返回 None。 */
ResultVec vec_clone(const Vec *source);

usize vec_len(const Vec *v);
usize vec_capacity(const Vec *v);
bool vec_is_empty(const Vec *v);

/* 保证至少还能容纳 additional 个元素；失败时 Vec 不变。 */
i32 vec_reserve(Vec *v, usize additional);

/*
 * push/insert 对 copy 元素执行复制；对 owned 元素执行 move，并将源对象清零。
 */
i32 vec_push(Vec *v, void *data);
i32 vec_insert(Vec *v, usize index, void *data);

/* Rust wrapper/internal 使用：复制位模式但不清零源，源生命周期由调用方终止。 */
i32 vec_push_raw(Vec *v, const void *data);
i32 vec_insert_raw(Vec *v, usize index, const void *data);

/* 从尾部移出一个元素到 out；out 不得为 NULL。 */
i32 vec_pop(Vec *v, void *out);

/* 返回第 i 个元素的借用指针，不做边界检查。 */
void *vec_get(const Vec *v, usize i);

/* out 非 NULL 时移出元素；out 为 NULL 时 drop 元素。 */
i32 vec_remove(Vec *v, usize index, void *out);
i32 vec_swap_remove(Vec *v, usize index, void *out);

void vec_truncate(Vec *v, usize len);
void vec_clear(Vec *v);

#endif /* SHYOS_VEC_H */
