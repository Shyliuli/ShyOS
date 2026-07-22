#include "alloc.h"
#include "obj.h"
#include "early_stdio.h"
#include "string.h"
#include "vec.h"

static i32 test_copy_vec(void)
{
	ResultVec result = vec_new(Type(i32));
	Vec values;
	i32 first = 6;
	i32 second = 7;
	i32 inserted = 8;
	i32 removed;
	i32 popped;

	if (Vec_is_err(&result))
		return -1;
	values = Vec_unwrap(&result);
	if (vec_push(&values, &first) != 0 ||
	    vec_push(&values, &second) != 0 ||
	    vec_insert(&values, 1, &inserted) != 0)
		return -1;
	if (first != 6 || second != 7 || inserted != 8)
		return -1;
	if (*(i32 *)vec_get(&values, 0) != 6 ||
	    *(i32 *)vec_get(&values, 1) != 8)
		return -1;
	if (vec_remove(&values, 1, &removed) != 0 || removed != 8)
		return -1;
	if (vec_pop(&values, &popped) != 0 || popped != 7)
		return -1;
	vec_drop(&values);
	return 0;
}

static bool keep_not_dash(u8 value, void *context)
{
	(void)context;
	return value != '-';
}

static i32 test_string_edit(void)
{
	ResultString result = string_new("abc");
	ResultString tail_result;
	String value;
	String tail;
	u8 removed;
	u8 popped;

	if (String_is_err(&result))
		return -1;
	value = String_unwrap(&result);
	if (string_insert_str(&value, 1, "XY") != 0 ||
	    strcmp(string_as_ptr(&value), "aXYbc") != 0 ||
	    string_remove(&value, 1, &removed) != 0 || removed != 'X' ||
	    strcmp(string_as_ptr(&value), "aYbc") != 0 ||
	    string_push(&value, '-') != 0 ||
	    string_pop(&value, &popped) != 0 || popped != '-')
		return -1;
	tail_result = string_split_off(&value, 2);
	if (String_is_err(&tail_result))
		return -1;
	tail = String_unwrap(&tail_result);
	if (strcmp(string_as_ptr(&value), "aY") != 0 ||
	    strcmp(string_as_ptr(&tail), "bc") != 0)
		return -1;
	if (string_push_str(&tail, "--") != 0)
		return -1;
	string_retain(&tail, keep_not_dash, NULL);
	if (strcmp(string_as_ptr(&tail), "bc") != 0 ||
	    string_truncate(&tail, 1) != 0 ||
	    strcmp(string_as_ptr(&tail), "b") != 0)
		return -1;
	string_clear(&tail);
	if (!string_is_empty(&tail))
		return -1;
	string_drop(&tail);
	string_drop(&value);
	return 0;
}

static i32 test_owned_nested_vec(void)
{
	ResultString source_result = string_new("shy");
	ResultString cloned_result;
	ResultVec strings_result;
	ResultVec outer_result;
	ResultVec outer_clone_result;
	String source;
	String cloned;
	Vec strings;
	Vec outer;
	Vec outer_clone;
	Vec *inner;
	String *element;

	if (String_is_err(&source_result))
		return -1;
	source = String_unwrap(&source_result);
	if (string_push_str(&source, "os") != 0 ||
	    strcmp(string_as_ptr(&source), "shyos") != 0)
		return -1;
	if (string_insert(&source, 3, '-') != 0 ||
	    strcmp(string_as_ptr(&source), "shy-os") != 0)
		return -1;
	{
		u8 removed;
		if (string_remove(&source, 3, &removed) != 0 || removed != '-' ||
		    strcmp(string_as_ptr(&source), "shyos") != 0)
			return -1;
	}

	cloned_result = string_clone(&source);
	if (String_is_err(&cloned_result))
		return -1;
	cloned = String_unwrap(&cloned_result);
	if (cloned.raw.data == source.raw.data)
		return -1;

	strings_result = vec_new(OwnedType(String));
	if (Vec_is_err(&strings_result))
		return -1;
	strings = Vec_unwrap(&strings_result);
	if (vec_push(&strings, &cloned) != 0)
		return -1;
	if (cloned.raw.data != NULL)
		return -1;
	element = vec_get(&strings, 0);
	if (strcmp(string_as_ptr(element), "shyos") != 0)
		return -1;

	outer_result = vec_new(OwnedType(Vec));
	if (Vec_is_err(&outer_result))
		return -1;
	outer = Vec_unwrap(&outer_result);
	if (vec_push(&outer, &strings) != 0)
		return -1;
	if (strings.data != NULL)
		return -1;

	outer_clone_result = vec_clone(&outer);
	if (Vec_is_err(&outer_clone_result))
		return -1;
	outer_clone = Vec_unwrap(&outer_clone_result);
	inner = vec_get(&outer_clone, 0);
	element = vec_get(inner, 0);
	if (strcmp(string_as_ptr(element), "shyos") != 0)
		return -1;
	if (element->raw.data ==
	    ((String *)vec_get((Vec *)vec_get(&outer, 0), 0))->raw.data)
		return -1;

	vec_drop(&outer_clone);
	vec_drop(&outer);
	string_drop(&source);
	return 0;
}

static i32 test_let_cleanup(void)
{
	ResultString result = string_new("scope");

	if (String_is_err(&result))
		return -1;
	{
		let(String) scoped = String_unwrap(&result);
		if (strcmp(string_as_ptr(&scoped), "scope") != 0)
			return -1;
	}
	return 0;
}

static i32 test_result_string_vec(void)
{
	ResultVec vec_result = vec_new(OwnedType(ResultString));
	ResultVec clone_result;
	ResultString present = string_new("value");
	ResultString absent = StringNone();
	Vec values;
	Vec clone;
	ResultString *element;

	if (Vec_is_err(&vec_result) || String_is_err(&present))
		return -1;
	values = Vec_unwrap(&vec_result);
	if (vec_push(&values, &present) != 0 ||
	    vec_push(&values, &absent) != 0)
		return -1;
	element = vec_get(&values, 0);
	if (String_is_err(element) ||
	    strcmp(string_as_ptr(&element->ok), "value") != 0)
		return -1;
	element = vec_get(&values, 1);
	if (!String_is_none(element))
		return -1;
	clone_result = vec_clone(&values);
	if (Vec_is_err(&clone_result))
		return -1;
	clone = Vec_unwrap(&clone_result);
	element = vec_get(&clone, 0);
	if (String_is_err(element) ||
	    strcmp(string_as_ptr(&element->ok), "value") != 0 ||
	    element->ok.raw.data ==
		    ((ResultString *)vec_get(&values, 0))->ok.raw.data)
		return -1;
	element = vec_get(&clone, 1);
	if (!String_is_none(element))
		return -1;
	vec_drop(&clone);
	vec_drop(&values);
	return 0;
}

i32 main(void)
{
	var inferred = 42;
	void *block;
	char *whole;
	char *prefix;

	if (inferred != 42 || test_copy_vec() != 0 ||
	    test_string_edit() != 0 || test_owned_nested_vec() != 0 ||
	    test_let_cleanup() != 0 || test_result_string_vec() != 0)
		return 1;

	block = malloc(32);
	if (block == NULL)
		return 2;
	memset(block, 0x3c, 32);
	if (((u8 *)block)[31] != 0x3c)
		return 3;
	free(block);

	whole = strdup("shyos");
	if (whole == NULL || strcmp(whole, "shyos") != 0)
		return 4;
	free(whole);

	prefix = strndup("shyos", 3);
	if (prefix == NULL || strcmp(prefix, "shy") != 0)
		return 5;
	free(prefix);

	early_printf("test11 data structure c\n");
	return 0;
}
