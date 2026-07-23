#include "hashset.h"

#include "alloc.h"

_Static_assert(offsetof(HashSet, table) == 0, "HashSet.table must be first");
_Static_assert(sizeof(HashSet) == sizeof(HashTable),
	"HashSet and HashTable size must match");
_Static_assert(_Alignof(HashSet) == _Alignof(HashTable),
	"HashSet and HashTable alignment must match");

ResultHashSet hash_set_new(
	usize capacity,
	TypeDesc key_type,
	hash_fn hash,
	eq_fn eq
)
{
	ResultHashTable table_result;
	HashSet set;

	table_result = hash_table_new(
		capacity,
		key_type,
		Type(u8),
		hash,
		eq
	);
	if (HashTable_is_err(&table_result)) {
		return HashSetErr(HashTable_error(&table_result));
	}
	set.table = HashTable_unwrap(&table_result);
	return HashSetOk(set);
}

usize hash_set_len(const HashSet *set)
{
	return hash_table_len(&set->table);
}

usize hash_set_capacity(const HashSet *set)
{
	return hash_table_capacity(&set->table);
}

bool hash_set_is_empty(const HashSet *set)
{
	return hash_table_is_empty(&set->table);
}

bool hash_set_contains(const HashSet *set, const void *value)
{
	return hash_table_contains(&set->table, value);
}

i32 hash_set_insert(HashSet *set, void *value)
{
	bool present;
	u8 marker = 0;
	i32 result;

	if (set == NULL || value == NULL) {
		return HASH_TABLE_INVALID_ARGUMENT;
	}
	present = hash_set_contains(set, value);
	result = hash_table_insert(&set->table, value, &marker);
	if (result != HASH_TABLE_OK) {
		return result;
	}
	return present ? HASH_SET_ALREADY_PRESENT : HASH_SET_INSERTED;
}

i32 hash_set_remove(HashSet *set, const void *value)
{
	if (set == NULL || value == NULL) {
		return HASH_TABLE_INVALID_ARGUMENT;
	}
	return hash_table_remove(&set->table, value, NULL);
}

void hash_set_clear(HashSet *set)
{
	if (set != NULL) {
		hash_table_clear(&set->table);
	}
}

void hash_set_drop(void *self)
{
	HashSet *set = self;

	if (set != NULL) {
		hash_table_drop(&set->table);
	}
}

void *hash_set_clone_obj(const void *source_ptr)
{
	const HashSet *source = source_ptr;
	ResultHashTable table_result;
	HashSet *copy;

	table_result = hash_table_clone(&source->table);
	if (HashTable_is_err(&table_result)) {
		return NULL;
	}
	copy = malloc(sizeof(*copy));
	if (copy == NULL) {
		HashTable table = HashTable_unwrap(&table_result);
		hash_table_drop(&table);
		return NULL;
	}
	copy->table = HashTable_unwrap(&table_result);
	return copy;
}

ResultHashSet hash_set_clone(const HashSet *source)
{
	HashSet *copy;
	HashSet value;

	if (source == NULL) {
		return HashSetErr(ERROR_INVALID_ARGUMENT);
	}
	copy = hash_set_clone_obj(source);
	if (copy == NULL) {
		return HashSetErr(ERROR_OUT_OF_MEMORY);
	}
	value = *copy;
	memset(copy, 0, sizeof(*copy));
	free(copy);
	return HashSetOk(value);
}
