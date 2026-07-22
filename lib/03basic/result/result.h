#ifndef SHYOS_RESULT_H
#define SHYOS_RESULT_H

#include "alloc.h"
#include "errno.h"
#include "obj.h"
#include "panic.h"
#include "shy_type.h"
#include "string_noalloc.h"

enum {
	SHY_RESULT_OK = 0,
	SHY_RESULT_ERR = 1,
};

#define SHY_RESULT_TYPE_INNER(T) Result##T
#define SHY_RESULT_TYPE(T) SHY_RESULT_TYPE_INNER(T)
#define SHY_RESULT_DROP_INNER(T) Result##T##_drop
#define SHY_RESULT_DROP(T) SHY_RESULT_DROP_INNER(T)
#define SHY_RESULT_AUTODROP_INNER(T) Result##T##_autodrop
#define SHY_RESULT_AUTODROP(T) SHY_RESULT_AUTODROP_INNER(T)
#define SHY_RESULT_CLONE_OBJ_INNER(T) Result##T##_clone_obj
#define SHY_RESULT_CLONE_OBJ(T) SHY_RESULT_CLONE_OBJ_INNER(T)
#define SHY_RESULT_TYPE_DESC_INNER(T) Result##T##TypeDesc
#define SHY_RESULT_TYPE_DESC(T) SHY_RESULT_TYPE_DESC_INNER(T)

#define DECLARE_SHY_RESULT(T)                                                \
	typedef struct SHY_RESULT_TYPE(T) SHY_RESULT_TYPE(T);                 \
	struct SHY_RESULT_TYPE(T) {                                            \
		i32 tag;                                                       \
		union {                                                        \
			T ok;                                                  \
			i32 error;                                             \
		};                                                             \
	};

#define SHY_RESULT_NO_DROP(value) ((void)(value))

#define IMPL_SHY_RESULT_METHODS(T, dropfn)                                   \
	static inline SHY_RESULT_TYPE(T) T##Ok(T value)                     \
	{                                                                     \
		return (SHY_RESULT_TYPE(T)){                                  \
			.tag = SHY_RESULT_OK,                                 \
			.ok = value,                                          \
		};                                                            \
	}                                                                     \
	static inline SHY_RESULT_TYPE(T) T##Err(i32 error)                  \
	{                                                                     \
		return (SHY_RESULT_TYPE(T)){                                  \
			.tag = SHY_RESULT_ERR,                                \
			.error = error,                                       \
		};                                                            \
	}                                                                     \
	static inline SHY_RESULT_TYPE(T) T##None(void)                       \
	{                                                                     \
		return T##Err(ERROR_NO_VALUE);                                 \
	}                                                                     \
	static inline bool T##_is_ok(const SHY_RESULT_TYPE(T) *result)      \
	{                                                                     \
		return result->tag == SHY_RESULT_OK;                           \
	}                                                                     \
	static inline bool T##_is_err(const SHY_RESULT_TYPE(T) *result)     \
	{                                                                     \
		return result->tag == SHY_RESULT_ERR;                          \
	}                                                                     \
	static inline bool T##_is_none(const SHY_RESULT_TYPE(T) *result)    \
	{                                                                     \
		return T##_is_err(result) && result->error == ERROR_NO_VALUE;  \
	}                                                                     \
	static inline i32 T##_error(const SHY_RESULT_TYPE(T) *result)       \
	{                                                                     \
		return T##_is_err(result) ? result->error : 0;                 \
	}                                                                     \
	static inline SHY_RESULT_TYPE(T) T##_take(                           \
		SHY_RESULT_TYPE(T) *result)                                  \
	{                                                                     \
		SHY_RESULT_TYPE(T) value = *result;                           \
		*result = T##None();                                         \
		return value;                                                \
	}                                                                     \
	static inline SHY_RESULT_TYPE(T) T##_replace(                        \
		SHY_RESULT_TYPE(T) *result, T value)                         \
	{                                                                     \
		SHY_RESULT_TYPE(T) previous = *result;                        \
		*result = T##Ok(value);                                      \
		return previous;                                             \
	}                                                                     \
	static inline T const *T##_unwrap_ref(                               \
		const SHY_RESULT_TYPE(T) *result)                           \
	{                                                                     \
		if (T##_is_err(result))                                      \
			panic("error: unwrap_ref an Err CResult");            \
		return &result->ok;                                          \
	}                                                                     \
	static inline T *T##_unwrap_ref_mut(                                 \
		SHY_RESULT_TYPE(T) *result)                                  \
	{                                                                     \
		if (T##_is_err(result))                                      \
			panic("error: unwrap_ref_mut an Err CResult");        \
		return &result->ok;                                          \
	}                                                                     \
	static inline T T##_unwrap(SHY_RESULT_TYPE(T) *result)              \
	{                                                                     \
		T value;                                                      \
		if (T##_is_err(result))                                      \
			panic("error: unwrap an Err CResult");                \
		value = result->ok;                                          \
		*result = T##None();                                         \
		return value;                                                \
	}                                                                     \
	static inline T T##_unwrap_or(                                      \
		SHY_RESULT_TYPE(T) *result, T value)                         \
	{                                                                     \
		return T##_is_err(result) ? value : T##_unwrap(result);      \
	}                                                                     \
	static inline void SHY_RESULT_DROP(T)(                               \
		SHY_RESULT_TYPE(T) *result)                                  \
	{                                                                     \
		if (T##_is_ok(result))                                       \
			dropfn(&result->ok);                                 \
		*result = T##None();                                         \
	}                                                                     \
	static inline void SHY_RESULT_AUTODROP(T)(                           \
		SHY_RESULT_TYPE(T) *result)                                  \
	{                                                                     \
		SHY_RESULT_DROP(T)(result);                                    \
	}

#define IMPL_SHY_RESULT(T)                                                   \
	DECLARE_SHY_RESULT(T)                                                  \
	IMPL_SHY_RESULT_METHODS(T, SHY_RESULT_NO_DROP)

#define IMPL_SHY_OWNED_RESULT_METHODS(T, dropfn, clonefn)                    \
	IMPL_SHY_RESULT_METHODS(T, dropfn)                                    \
	static inline void *SHY_RESULT_CLONE_OBJ(T)(                         \
		const void *source_ptr)                                      \
	{                                                                     \
		const SHY_RESULT_TYPE(T) *source = source_ptr;                \
		SHY_RESULT_TYPE(T) *copy = malloc(sizeof(*copy));             \
		void *value;                                                  \
		if (copy == NULL)                                             \
			return NULL;                                          \
		if (T##_is_err(source)) {                                     \
			*copy = T##Err(source->error);                         \
			return copy;                                          \
		}                                                             \
		value = clonefn(&source->ok);                                 \
		if (value == NULL) {                                          \
			free(copy);                                           \
			return NULL;                                          \
		}                                                             \
		*copy = T##Ok(*(T *)value);                                   \
		memset(value, 0, sizeof(T));                                  \
		free(value);                                                  \
		return copy;                                                  \
	}                                                                     \
	static inline TypeDesc SHY_RESULT_TYPE_DESC(T)(void)                 \
	{                                                                     \
		static const TypeDesc desc = {                                \
			.size = sizeof(SHY_RESULT_TYPE(T)),                    \
			.drop = (type_drop_fn)SHY_RESULT_DROP(T),              \
			.clone = SHY_RESULT_CLONE_OBJ(T),                      \
		};                                                            \
		return desc;                                                  \
	}

#define IMPL_SHY_OWNED_RESULT(T, dropfn, clonefn)                            \
	Impl_Type_Desc(T, dropfn, clonefn)                                    \
	DECLARE_SHY_RESULT(T)                                                  \
	IMPL_SHY_OWNED_RESULT_METHODS(T, dropfn, clonefn)

IMPL_SHY_RESULT(bool)
IMPL_SHY_RESULT(char)
IMPL_SHY_RESULT(i8)
IMPL_SHY_RESULT(i16)
IMPL_SHY_RESULT(i32)
IMPL_SHY_RESULT(i64)
IMPL_SHY_RESULT(u8)
IMPL_SHY_RESULT(u16)
IMPL_SHY_RESULT(u32)
IMPL_SHY_RESULT(u64)
IMPL_SHY_RESULT(isize)
IMPL_SHY_RESULT(usize)
IMPL_SHY_RESULT(f32)
IMPL_SHY_RESULT(f64)
IMPL_SHY_RESULT(int)
IMPL_SHY_RESULT(unsigned)
IMPL_SHY_RESULT(long)

#endif /* SHYOS_RESULT_H */
