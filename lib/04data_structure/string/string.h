#ifndef SHYOS_STRING_H
#define SHYOS_STRING_H

#include "rawbuf.h"
#include "result.h"
#include "string_noalloc.h"

char *strdup(const char *str);
char *strndup(const char *str, usize max_len);

typedef struct String String;
struct String {
	RawBuf raw;
	usize len;
};

typedef bool (*string_retain_fn)(u8 value, void *context);

void *string_clone_obj(const void *source);
void string_drop(void *self);

IMPL_SHY_OWNED_RESULT(String, string_drop, string_clone_obj)

ResultString string_new(const char *cstr);
ResultString string_new_n(const char *data, usize len);
ResultString string_new_with_capacity(usize capacity);
ResultString string_clone(const String *source);

usize string_len(const String *string);
usize string_capacity(const String *string);
bool string_is_empty(const String *string);
const char *string_as_ptr(const String *string);

i32 string_reserve(String *string, usize additional);
i32 string_push(String *string, u8 value);
i32 string_push_str(String *string, const char *value);
i32 string_push_str_n(String *string, const char *value, usize len);
i32 string_insert(String *string, usize index, u8 value);
i32 string_insert_str(String *string, usize index, const char *value);
i32 string_insert_str_n(String *string, usize index, const char *value, usize len);
i32 string_remove(String *string, usize index, u8 *out);
ResultString string_split_off(String *string, usize at);
void string_retain(String *string, string_retain_fn keep, void *context);
void string_clear(String *string);
i32 string_truncate(String *string, usize len);
i32 string_pop(String *string, u8 *out);

#endif /* SHYOS_STRING_H */
