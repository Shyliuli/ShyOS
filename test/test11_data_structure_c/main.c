#include "alloc.h"
#include "deque.h"
#include "obj.h"
#include "early_stdio.h"
#include "hashtable.h"
#include "hashset.h"
#include "linked_list.h"
#include "queue.h"
#include "rawbuf.h"
#include "ringbuffer.h"
#include "stack.h"
#include "string.h"
#include "vec.h"

typedef struct OwnedProbe {
	i32 value;
} OwnedProbe;

static usize owned_probe_drop_count;

static void owned_probe_drop(void *self)
{
	OwnedProbe *probe = self;

	owned_probe_drop_count++;
	probe->value = 0;
}

static i32 test_rawbuf(void)
{
	ResultRawBuf result = raw_buf_new(sizeof(u32));
	RawBuf raw;

	if (RawBuf_is_err(&result))
		return -1;
	raw = RawBuf_unwrap(&result);
	if (raw_buf_reserve(&raw, 3) != 0 || raw.cap < 3)
		return -1;
	*(u32 *)raw_buf_get(&raw, 2) = 42;
	if (*(u32 *)raw_buf_get(&raw, 2) != 42)
		return -1;
	raw_buf_drop(&raw);
	return 0;
}

static i32 test_copy_vec(void)
{
	ResultVec result = vec_new(Type(i32));
	Vec values;
	i32 first = 6;
	i32 second = 7;
	i32 inserted = 8;
	i32 replacement = 9;
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
	if (vec_set(&values, 0, &replacement) != 0 ||
	    *(i32 *)vec_get(&values, 0) != 9 || replacement != 9)
		return -1;
	if (vec_remove(&values, 1, &removed) != 0 || removed != 8)
		return -1;
	if (vec_pop(&values, &popped) != 0 || popped != 7)
		return -1;
	vec_drop(&values);
	return 0;
}

static i32 test_owned_vec_set(void)
{
	TypeDesc probe_type = {
		.size = sizeof(OwnedProbe),
		.drop = owned_probe_drop,
		.clone = NULL,
	};
	ResultVec result = vec_new(probe_type);
	Vec values;
	OwnedProbe first = { .value = 1 };
	OwnedProbe replacement = { .value = 2 };

	if (Vec_is_err(&result))
		return -1;
	values = Vec_unwrap(&result);
	owned_probe_drop_count = 0;
	if (vec_push(&values, &first) != 0 || first.value != 0)
		return -1;
	if (vec_set(&values, 0, &replacement) != 0 ||
	    replacement.value != 0 ||
	    ((OwnedProbe *)vec_get(&values, 0))->value != 2 ||
	    owned_probe_drop_count != 1)
		return -1;
	vec_drop(&values);
	if (owned_probe_drop_count != 2)
		return -1;
	return 0;
}

static i32 test_deque_stack_queue(void)
{
	ResultDeque deque_result = deque_new(Type(i32));
	ResultStack stack_result = stack_new(Type(i32));
	ResultQueue queue_result = queue_new(Type(i32));
	Deque deque;
	Stack stack;
	Queue queue;
	i32 one = 1;
	i32 two = 2;
	i32 three = 3;
	i32 out;

	if (Deque_is_err(&deque_result) || Stack_is_err(&stack_result) ||
	    Queue_is_err(&queue_result))
		return -1;
	deque = Deque_unwrap(&deque_result);
	stack = Stack_unwrap(&stack_result);
	queue = Queue_unwrap(&queue_result);

	if (deque_push_back(&deque, &two) != 0 ||
	    deque_push_front(&deque, &one) != 0 ||
	    deque_push_back(&deque, &three) != 0 ||
	    *(const i32 *)deque_front(&deque) != 1 ||
	    *(const i32 *)deque_back(&deque) != 3 ||
	    deque_pop_front(&deque, &out) != 0 || out != 1 ||
	    deque_pop_back(&deque, &out) != 0 || out != 3 ||
	    deque_pop_back(&deque, &out) != 0 || out != 2 ||
	    deque_pop_back(&deque, &out) != ERROR_EMPTY)
		return -1;

	if (stack_push(&stack, &one) != 0 ||
	    stack_push(&stack, &two) != 0 ||
	    *(const i32 *)stack_peek(&stack) != 2 ||
	    stack_pop(&stack, &out) != 0 || out != 2 ||
	    stack_pop(&stack, &out) != 0 || out != 1)
		return -1;

	if (queue_push(&queue, &one) != 0 ||
	    queue_push(&queue, &two) != 0 ||
	    *(const i32 *)queue_front(&queue) != 1 ||
	    *(const i32 *)queue_back(&queue) != 2 ||
	    queue_pop(&queue, &out) != 0 || out != 1 ||
	    queue_pop(&queue, &out) != 0 || out != 2)
		return -1;

	queue_drop(&queue);
	stack_drop(&stack);
	deque_drop(&deque);
	return 0;
}

static i32 test_deque_wrapped_growth(void)
{
	ResultDeque result = deque_new(Type(i32));
	Deque deque;
	i32 value;
	i32 out;
	usize index;

	if (Deque_is_err(&result))
		return -1;
	deque = Deque_unwrap(&result);
	if (deque_reserve(&deque, 3) != 0)
		return -1;
	for (value = 0; value < 5; ++value) {
		if (deque_push_back(&deque, &value) != 0)
			return -1;
	}
	if (deque_pop_front(&deque, &out) != 0 || out != 0 ||
	    deque_pop_front(&deque, &out) != 0 || out != 1)
		return -1;
	for (value = 5; value < 8; ++value) {
		if (deque_push_back(&deque, &value) != 0)
			return -1;
	}
	if (deque_len(&deque) != 6)
		return -1;
	for (index = 0; index < deque_len(&deque); ++index) {
		if (*(const i32 *)deque_get(&deque, index) !=
		    (i32)index + 2)
			return -1;
	}
	deque_drop(&deque);
	return 0;
}

static i32 test_ring_buffer_overwrite(void)
{
	TypeDesc probe_type = {
		.size = sizeof(OwnedProbe),
		.drop = owned_probe_drop,
		.clone = NULL,
	};
	ResultRingBuffer result = ring_buffer_new(2, probe_type);
	RingBuffer buffer;
	OwnedProbe first = { .value = 1 };
	OwnedProbe second = { .value = 2 };
	OwnedProbe third = { .value = 3 };
	OwnedProbe out;

	if (RingBuffer_is_err(&result))
		return -1;
	buffer = RingBuffer_unwrap(&result);
	owned_probe_drop_count = 0;
	if (ring_buffer_push(&buffer, &first) != 0 ||
	    ring_buffer_push(&buffer, &second) != 0 ||
	    ring_buffer_push(&buffer, &third) != 0 ||
	    first.value != 0 || second.value != 0 || third.value != 0 ||
	    owned_probe_drop_count != 1 ||
	    ring_buffer_len(&buffer) != 2 ||
	    ((const OwnedProbe *)ring_buffer_front(&buffer))->value != 2 ||
	    ((const OwnedProbe *)ring_buffer_back(&buffer))->value != 3)
		return -1;
	if (ring_buffer_pop(&buffer, &out) != 0 || out.value != 2)
		return -1;
	owned_probe_drop(&out);
	ring_buffer_drop(&buffer);
	if (owned_probe_drop_count != 3)
		return -1;
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
	if (strings.raw.data != NULL)
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

static usize hash_u32(const void *value)
{
	return *(const u32 *)value;
}

static bool eq_u32(const void *left, const void *right)
{
	return *(const u32 *)left == *(const u32 *)right;
}

static i32 test_hashtable(void)
{
	ResultHashTable table_result;
	ResultString first_result;
	ResultString second_result;
	ResultString third_result;
	ResultString fourth_result;
	ResultString replacement_result;
	ResultHashTable clone_result;
	HashTable table;
	HashTable clone;
	String first;
	String second;
	String third;
	String fourth;
	String replacement;
	String removed;
	const String *value;
	u32 key_one = 1;
	u32 key_nine = 9;
	u32 key_seventeen = 17;
	u32 key_twenty_five = 25;

	table_result = hash_table_new(
		8,
		Type(u32),
		OwnedType(String),
		hash_u32,
		eq_u32
	);
	if (HashTable_is_err(&table_result))
		return -1;
	table = HashTable_unwrap(&table_result);

	first_result = string_new("one");
	second_result = string_new("nine");
	third_result = string_new("seventeen");
	fourth_result = string_new("twenty-five");
	replacement_result = string_new("ONE");
	if (String_is_err(&first_result) ||
	    String_is_err(&second_result) ||
	    String_is_err(&third_result) ||
	    String_is_err(&fourth_result) ||
	    String_is_err(&replacement_result))
		return -1;
	first = String_unwrap(&first_result);
	second = String_unwrap(&second_result);
	third = String_unwrap(&third_result);
	fourth = String_unwrap(&fourth_result);
	replacement = String_unwrap(&replacement_result);

	if (hash_table_insert(&table, &key_one, &first) != 0 ||
	    hash_table_insert(&table, &key_nine, &second) != 0 ||
	    hash_table_insert(&table, &key_seventeen, &third) != 0)
		return -1;
	value = hash_table_get(&table, &key_seventeen);
	if (value == NULL || strcmp(string_as_ptr(value), "seventeen") != 0)
		return -1;

	if (hash_table_remove(&table, &key_nine, NULL) != 0)
		return -1;
	value = hash_table_get(&table, &key_seventeen);
	if (value == NULL || strcmp(string_as_ptr(value), "seventeen") != 0)
		return -1;

	if (hash_table_insert(&table, &key_twenty_five, &fourth) != 0)
		return -1;
	if (hash_table_insert(&table, &key_one, &replacement) != 0)
		return -1;
	value = hash_table_get(&table, &key_one);
	if (value == NULL || strcmp(string_as_ptr(value), "ONE") != 0)
		return -1;

	clone_result = hash_table_clone(&table);
	if (HashTable_is_err(&clone_result))
		return -1;
	clone = HashTable_unwrap(&clone_result);
	value = hash_table_get(&clone, &key_one);
	if (value == NULL || strcmp(string_as_ptr(value), "ONE") != 0 ||
	    value->raw.data ==
		    ((const String *)hash_table_get(&table, &key_one))->raw.data)
		return -1;
	hash_table_drop(&clone);

	if (hash_table_remove(&table, &key_seventeen, &removed) != 0 ||
	    strcmp(string_as_ptr(&removed), "seventeen") != 0)
		return -1;
	string_drop(&removed);

	hash_table_clear(&table);
	if (!hash_table_is_empty(&table) ||
	    hash_table_contains(&table, &key_one))
		return -1;
	hash_table_drop(&table);
	return 0;
}

static i32 test_hashset(void)
{
	ResultHashSet set_result;
	ResultHashSet clone_result;
	HashSet set;
	HashSet clone;
	u32 first = 1;
	u32 collision = 9;

	set_result = hash_set_new(8, Type(u32), hash_u32, eq_u32);
	if (HashSet_is_err(&set_result))
		return -1;
	set = HashSet_unwrap(&set_result);

	if (hash_set_insert(&set, &first) != HASH_SET_INSERTED ||
	    hash_set_insert(&set, &first) != HASH_SET_ALREADY_PRESENT ||
	    hash_set_insert(&set, &collision) != HASH_SET_INSERTED ||
	    !hash_set_contains(&set, &first) ||
	    !hash_set_contains(&set, &collision) ||
	    hash_set_len(&set) != 2)
		return -1;

	clone_result = hash_set_clone(&set);
	if (HashSet_is_err(&clone_result))
		return -1;
	clone = HashSet_unwrap(&clone_result);
	if (!hash_set_contains(&clone, &first) ||
	    !hash_set_contains(&clone, &collision))
		return -1;
	hash_set_drop(&clone);

	if (hash_set_remove(&set, &first) != HASH_TABLE_OK ||
	    hash_set_contains(&set, &first) ||
	    hash_set_remove(&set, &first) != HASH_TABLE_NOT_FOUND)
		return -1;
	hash_set_clear(&set);
	if (!hash_set_is_empty(&set))
		return -1;
	hash_set_drop(&set);
	return 0;
}

static i32 test_linked_list(void)
{
	ResultLinkedList result = linked_list_new_with_capacity(3, Type(i32));
	LinkedList list;
	usize first_index;
	usize second_index;
	usize reused_index;
	i32 first = 1;
	i32 second = 2;
	i32 replacement = 3;
	i32 out;

	if (LinkedList_is_err(&result))
		return -1;
	list = LinkedList_unwrap(&result);
	if (linked_list_push_back(&list, &first, &first_index) != 0 ||
	    linked_list_push_back(&list, &second, &second_index) != 0 ||
	    linked_list_first_index(&list) != first_index ||
	    linked_list_last_index(&list) != second_index ||
	    linked_list_next_index(&list, first_index) != second_index ||
	    *(const i32 *)linked_list_get(&list, second_index) != 2)
		return -1;
	if (linked_list_remove(&list, first_index, &out) != 0 || out != 1 ||
	    linked_list_get(&list, first_index) != NULL)
		return -1;
	if (linked_list_push_front(&list, &replacement, &reused_index) != 0 ||
	    reused_index != first_index ||
	    *(const i32 *)linked_list_front(&list) != 3 ||
	    linked_list_pop_back(&list, &out) != 0 || out != 2)
		return -1;
	linked_list_drop(&list);
	return 0;
}

i32 main(void)
{
	var inferred = 42;
	void *block;
	char *whole;
	char *prefix;

	if (inferred != 42 || test_rawbuf() != 0 || test_copy_vec() != 0 ||
	    test_owned_vec_set() != 0 ||
	    test_deque_stack_queue() != 0 ||
	    test_deque_wrapped_growth() != 0 ||
	    test_ring_buffer_overwrite() != 0 ||
	    test_string_edit() != 0 || test_owned_nested_vec() != 0 ||
	    test_let_cleanup() != 0 || test_result_string_vec() != 0 ||
	    test_hashtable() != 0 || test_hashset() != 0 ||
	    test_linked_list() != 0)
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
