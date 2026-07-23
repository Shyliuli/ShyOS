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
	ResultRawBuf result;
	String string;

	if (capacity == (usize)-1) {
		return StringErr(ERROR_INVALID_ARGUMENT);
	}
	result = raw_buf_new(sizeof(char));
	if (RawBuf_is_err(&result)) {
		return StringErr(RawBuf_error(&result));
	}
	string.raw = RawBuf_unwrap(&result);
	string.len = 0;
	if (raw_buf_reserve(&string.raw, capacity + 1) != 0) {
		string_drop(&string);
		return StringErr(ERROR_OUT_OF_MEMORY);
	}
	((char *)string.raw.data)[0] = '\0';
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
	string.len = len;
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
	return string->len;
}

usize string_capacity(const String *string)
{
	return string->raw.cap == 0 ? 0 : string->raw.cap - 1;
}

bool string_is_empty(const String *string)
{
	return string->len == 0;
}

const char *string_as_ptr(const String *string)
{
	return string->raw.data == NULL ? STRING_EMPTY : string->raw.data;
}

i32 string_reserve(String *string, usize additional)
{
	usize required;
	ResultRawBuf result;

	if (additional > (usize)-1 - string->len - 1) {
		return -1;
	}
	if (string->raw.elem_size == 0) {
		result = raw_buf_new(sizeof(char));
		if (RawBuf_is_err(&result)) {
			return -1;
		}
		string->raw = RawBuf_unwrap(&result);
		string->len = 0;
	}
	required = string->len + additional + 1;
	if (raw_buf_reserve(&string->raw, required) != 0) {
		return -1;
	}
	if (string->len == 0) {
		((char *)string->raw.data)[0] = '\0';
	}
	return 0;
}

i32 string_push(String *string, u8 value)
{
	char *data;

	if (value == '\0') {
		return -2;
	}
	if (string_reserve(string, 1) != 0) {
		return -1;
	}
	data = string->raw.data;
	data[string->len++] = value;
	data[string->len] = '\0';
	return 0;
}

i32 string_push_str_n(String *string, const char *value, usize len)
{
	char *data;

	if (value == NULL || memchr(value, '\0', len) != NULL ||
	    string_reserve(string, len) != 0) {
		return -1;
	}
	data = string->raw.data;
	if (len != 0) {
		memcpy(data + string->len, value, len);
	}
	string->len += len;
	data[string->len] = '\0';
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
	char *data;

	if (index > string->len) {
		return -1;
	}
	if (value == '\0') {
		return -2;
	}
	if (string_reserve(string, 1) != 0) {
		return -1;
	}
	data = string->raw.data;
	memmove(data + index + 1, data + index, string->len - index + 1);
	data[index] = value;
	string->len++;
	return 0;
}

i32 string_insert_str_n(String *string, usize index, const char *value, usize len)
{
	char *data;

	if (index > string->len || value == NULL ||
	    memchr(value, '\0', len) != NULL ||
	    string_reserve(string, len) != 0) {
		return -1;
	}
	data = string->raw.data;
	memmove(data + index + len, data + index, string->len - index + 1);
	if (len != 0) {
		memcpy(data + index, value, len);
	}
	string->len += len;
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
	char *data;

	if (index >= string->len || out == NULL) {
		return -1;
	}
	data = string->raw.data;
	*out = (u8)data[index];
	memmove(data + index, data + index + 1, string->len - index);
	string->len--;
	return 0;
}

ResultString string_split_off(String *string, usize at)
{
	ResultString tail;

	if (at > string->len) {
		return StringErr(ERROR_INVALID_ARGUMENT);
	}
	tail = string_new_n(string_as_ptr(string) + at, string->len - at);
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
	u8 *data;

	if (keep == NULL || string->raw.data == NULL) {
		return;
	}
	data = string->raw.data;
	for (read_index = 0; read_index < string->len; ++read_index) {
		if (keep(data[read_index], context)) {
			data[write_index++] = data[read_index];
		}
	}
	string->len = write_index;
	data[string->len] = '\0';
}

void string_clear(String *string)
{
	string->len = 0;
	if (string->raw.data != NULL) {
		((char *)string->raw.data)[0] = '\0';
	}
}

i32 string_truncate(String *string, usize len)
{
	if (len > string->len) {
		return -1;
	}
	string->len = len;
	if (string->raw.data != NULL) {
		((char *)string->raw.data)[len] = '\0';
	}
	return 0;
}

i32 string_pop(String *string, u8 *out)
{
	if (string->len == 0 || out == NULL) {
		return -1;
	}
	string->len--;
	*out = ((u8 *)string->raw.data)[string->len];
	((char *)string->raw.data)[string->len] = '\0';
	return 0;
}

void string_drop(void *self)
{
	String *string = self;

	raw_buf_drop(&string->raw);
	memset(string, 0, sizeof(*string));
}

void *string_clone_obj(const void *source_ptr)
{
	const String *source = source_ptr;
	ResultString result = string_new_n(string_as_ptr(source), source->len);
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
