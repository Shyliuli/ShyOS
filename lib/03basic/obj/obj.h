#ifndef SHYOS_OBJ_H
#define SHYOS_OBJ_H

#include "shy_type.h"

typedef void (*type_drop_fn)(void *self);
typedef void *(*type_clone_fn)(const void *src);

typedef struct TypeDesc {
	usize size;
	type_drop_fn drop;
	type_clone_fn clone;
} TypeDesc;

#define is_copy(td) ((td).drop == NULL)
#define is_owned(td) ((td).drop != NULL)

#define Type(T) ((TypeDesc){ .size = sizeof(T), .drop = NULL, .clone = NULL })
#define OwnedType(T) T##TypeDesc()

#define Impl_Drop(T)                                                         \
	static inline void T##_autodrop(T *value)                              \
	{                                                                       \
		(void)value;                                                     \
	}

#define Impl_Type_Desc(T, dropfn, clonefn)                                   \
	static inline TypeDesc T##TypeDesc(void)                              \
	{                                                                       \
		static const TypeDesc desc = {                                   \
			.size = sizeof(T),                                         \
			.drop = dropfn,                                            \
			.clone = clonefn,                                          \
		};                                                              \
		return desc;                                                    \
	}                                                                       \
	static inline void T##_autodrop(T *value)                              \
	{                                                                       \
		dropfn(value);                                                   \
	}

#if defined(__GNUC__) || defined(__clang__)
#define let(T) T __attribute__((cleanup(T##_autodrop)))
#else
#error "let(T) requires GCC or Clang cleanup support"
#endif

void obj_move(TypeDesc type, void *dst, void *src);
void *obj_clone(TypeDesc type, const void *src);
void obj_drop(TypeDesc type, void *value);

Impl_Drop(bool)
Impl_Drop(char)
Impl_Drop(i8)
Impl_Drop(i16)
Impl_Drop(i32)
Impl_Drop(i64)
Impl_Drop(u8)
Impl_Drop(u16)
Impl_Drop(u32)
Impl_Drop(u64)
Impl_Drop(isize)
Impl_Drop(usize)
Impl_Drop(f32)
Impl_Drop(f64)
Impl_Drop(int)
Impl_Drop(unsigned)
Impl_Drop(long)
#endif /* SHYOS_OBJ_H */
