#ifndef SHYOS_VEC_H
#define SHYOS_VEC_H

#include "obj.h"
#include "rawbuf.h"
#include "result.h"

typedef struct Vec Vec;
struct Vec {
	RawBuf raw;
	usize size;
	TypeDesc elem;
};

void *vec_clone_obj(const void *source);
void vec_drop(void *self);

IMPL_SHY_OWNED_RESULT(Vec, vec_drop, vec_clone_obj)

ResultVec vec_new(TypeDesc elem);
Vec vec_from_raw_parts(void *data, usize size, usize cap, TypeDesc elem);
ResultVec vec_clone(const Vec *source);

usize vec_len(const Vec *v);
usize vec_capacity(const Vec *v);
bool vec_is_empty(const Vec *v);

i32 vec_reserve(Vec *v, usize additional);

i32 vec_push(Vec *v, void *data);
i32 vec_insert(Vec *v, usize index, void *data);
i32 vec_push_raw(Vec *v, const void *data);
i32 vec_insert_raw(Vec *v, usize index, const void *data);

i32 vec_pop(Vec *v, void *out);
void *vec_get(const Vec *v, usize i);
i32 vec_set(Vec *v, usize index, void *data);
i32 vec_remove(Vec *v, usize index, void *out);
i32 vec_swap_remove(Vec *v, usize index, void *out);

void vec_truncate(Vec *v, usize len);
void vec_clear(Vec *v);

#endif /* SHYOS_VEC_H */
