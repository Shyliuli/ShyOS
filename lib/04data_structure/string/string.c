#include "string.h"

#include "alloc.h"

static const char STRING_EMPTY[] = "";

char *strdup(const char *str)
{
	usize size = strlen(str) + 1u;
	char *copy = malloc(size);

	if (copy == NULL) {
		return NULL;
	}
	memcpy(copy, str, size);
	return copy;
}

char *strndup(const char *str, usize max_len)
{
	usize len = strnlen(str, max_len);
	char *copy = malloc(len + 1u);

	if (copy == NULL) {
		return NULL;
	}
	memcpy(copy, str, len);
	copy[len] = '\0';
	return copy;
}

ResultString string_new_with_capacity(usize capacity)
{
	ResultVec result;
	String string;
	char nul = '\0';

	if (capacity == (usize)-1) {
		return StringErr(ERROR_INVALID_ARGUMENT);
	}
	result = vec_new(Type(char));
	if (Vec_is_err(&result)) {
		return StringErr(Vec_error(&result));
	}
	string.raw = Vec_unwrap(&result);
	if (vec_reserve(&string.raw, capacity + 1) != 0 ||
	    vec_push(&string.raw, &nul) != 0) {
		string_drop(&string);
		return StringErr(ERROR_OUT_OF_MEMORY);
	}
	return StringOk(string);
}

ResultString string_new_n(const char *data, usize len)
{
	ResultString result;
	String string;

	if (data == NULL || memchr(data, '\0', len) != NULL) {
		return StringErr(ERROR_INVALID_ARGUMENT);
	}
	result = string_new_with_capacity(len);
	if (String_is_err(&result)) {
		return result;
	}
	string = String_unwrap(&result);
	if (len != 0) {
		memcpy(string.raw.data, data, len);
	}
	((char *)string.raw.data)[len] = '\0';
	string.raw.size = len + 1;
	return StringOk(string);
}

ResultString string_new(const char *cstr)
{
	if (cstr == NULL) {
		return StringErr(ERROR_INVALID_ARGUMENT);
	}
	return string_new_n(cstr, strlen(cstr));
}

usize string_len(const String *string)
{
	if (string->raw.data == NULL || string->raw.size == 0) {
		return 0;
	}
	return string->raw.size - 1;
}

usize string_capacity(const String *string)
{
	if (string->raw.cap == 0) {
		return 0;
	}
	return string->raw.cap - 1;
}

bool string_is_empty(const String *string)
{
	return string_len(string) == 0;
}

const char *string_as_ptr(const String *string)
{
	if (string->raw.data == NULL || string->raw.size == 0) {
		return STRING_EMPTY;
	}
	return string->raw.data;
}

i32 string_reserve(String *string, usize additional)
{
	if (string->raw.elem.size == 0) {
		ResultVec result = vec_new(Type(char));
		if (Vec_is_err(&result)) {
			return -1;
		}
		string->raw = Vec_unwrap(&result);
	}
	if (string->raw.size == 0) {
		char nul = '\0';
		if (vec_push(&string->raw, &nul) != 0) {
			return -1;
		}
	}
	return vec_reserve(&string->raw, additional);
}

i32 string_push(String *string, u8 value)
{
	if (value == '\0') {
		return -2;
	}
	if (string_reserve(string, 1) != 0 ||
	    vec_insert(&string->raw, string_len(string), &value) != 0) {
		return -1;
	}
	return 0;
}

i32 string_push_str_n(String *string, const char *value, usize len)
{
	usize old_len;
	char *data;

	if (value == NULL || memchr(value, '\0', len) != NULL ||
	    string_reserve(string, len) != 0) {
		return -1;
	}
	old_len = string_len(string);
	data = string->raw.data;
	if (len != 0) {
		memcpy(data + old_len, value, len);
	}
	data[old_len + len] = '\0';
	string->raw.size += len;
	return 0;
}

i32 string_push_str(String *string, const char *value)
{
	if (value == NULL) {
		return -1;
	}
	return string_push_str_n(string, value, strlen(value));
}

i32 string_insert(String *string, usize index, u8 value)
{
	usize len = string_len(string);

	if (index > len) {
		return -1;
	}
	if (value == '\0') {
		return -2;
	}
	if (string_reserve(string, 1) != 0 ||
	    vec_insert(&string->raw, index, &value) != 0) {
		return -1;
	}
	return 0;
}

i32 string_insert_str_n(String *string, usize index, const char *value, usize len)
{
	usize old_len = string_len(string);
	char *data;

	if (index > old_len || value == NULL ||
	    memchr(value, '\0', len) != NULL) {
		return -1;
	}
	if (string_reserve(string, len) != 0) {
		return -1;
	}
	data = string->raw.data;
	memmove(data + index + len, data + index, old_len - index + 1);
	if (len != 0) {
		memcpy(data + index, value, len);
	}
	string->raw.size += len;
	return 0;
}

i32 string_insert_str(String *string, usize index, const char *value)
{
	if (value == NULL) {
		return -1;
	}
	return string_insert_str_n(string, index, value, strlen(value));
}

i32 string_remove(String *string, usize index, u8 *out)
{
	usize len = string_len(string);

	if (index >= len || out == NULL) {
		return -1;
	}
	return vec_remove(&string->raw, index, out);
}

ResultString string_split_off(String *string, usize at)
{
	usize len = string_len(string);
	ResultString tail;

	if (at > len) {
		return StringErr(ERROR_INVALID_ARGUMENT);
	}
	tail = string_new_n(string_as_ptr(string) + at, len - at);
	if (String_is_err(&tail)) {
		return tail;
	}
	(void)string_truncate(string, at);
	return tail;
}

void string_retain(String *string, string_retain_fn keep, void *context)
{
	usize read_index;
	usize write_index = 0;
	usize len = string_len(string);
	u8 *data;

	if (keep == NULL || string->raw.data == NULL) {
		return;
	}
	data = string->raw.data;
	for (read_index = 0; read_index < len; ++read_index) {
		if (keep(data[read_index], context)) {
			data[write_index++] = data[read_index];
		}
	}
	data[write_index] = '\0';
	string->raw.size = write_index + 1;
}

void string_clear(String *string)
{
	if (string->raw.data == NULL) {
		return;
	}
	((char *)string->raw.data)[0] = '\0';
	string->raw.size = 1;
}

i32 string_truncate(String *string, usize len)
{
	if (len > string_len(string)) {
		return -1;
	}
	if (string->raw.data == NULL) {
		return 0;
	}
	((char *)string->raw.data)[len] = '\0';
	string->raw.size = len + 1;
	return 0;
}

i32 string_pop(String *string, u8 *out)
{
	usize len = string_len(string);

	if (len == 0 || out == NULL) {
		return -1;
	}
	return vec_remove(&string->raw, len - 1, out);
}

void string_drop(void *self)
{
	String *string = self;
	vec_drop(&string->raw);
}

void *string_clone_obj(const void *source_ptr)
{
	const String *source = source_ptr;
	ResultString result = string_new_n(string_as_ptr(source), string_len(source));
	String *copy;

	if (String_is_err(&result)) {
		return NULL;
	}
	copy = malloc(sizeof(*copy));
	if (copy == NULL) {
		String value = String_unwrap(&result);
		string_drop(&value);
		return NULL;
	}
	*copy = String_unwrap(&result);
	return copy;
}

ResultString string_clone(const String *source)
{
	String *copy = string_clone_obj(source);
	String value;

	if (copy == NULL) {
		return StringErr(ERROR_OUT_OF_MEMORY);
	}
	value = *copy;
	memset(copy, 0, sizeof(*copy));
	free(copy);
	return StringOk(value);
}
