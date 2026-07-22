#include "hashtable.h"

#include "alloc.h"

enum {
	HASH_TABLE_MIN_CAPACITY = 8,
	HASH_TABLE_MAX_LOAD_NUMERATOR = 3,
	HASH_TABLE_MAX_LOAD_DENOMINATOR = 4,
};

static TypeDesc hash_table_storage_type(TypeDesc type)
{
	return (TypeDesc){
		.size = type.size,
		.drop = NULL,
		.clone = NULL,
	};
}

static u8 *hash_table_states(HashTable *table)
{
	return (u8 *)table->states.data;
}

static const u8 *hash_table_states_const(const HashTable *table)
{
	return (const u8 *)table->states.data;
}

static void *hash_table_key(HashTable *table, usize index)
{
	return vec_get(&table->keys, index);
}

static const void *hash_table_key_const(const HashTable *table, usize index)
{
	return vec_get(&table->keys, index);
}

static void *hash_table_value(HashTable *table, usize index)
{
	return vec_get(&table->values, index);
}

static const void *hash_table_value_const(const HashTable *table, usize index)
{
	return vec_get(&table->values, index);
}

static void hash_table_drop_slot(HashTable *table, usize index)
{
	obj_drop(table->key_type, hash_table_key(table, index));
	obj_drop(table->value_type, hash_table_value(table, index));
	hash_table_states(table)[index] = HASH_BUCKET_EMPTY;
}

static i32 hash_table_init_storage(
	HashTable *table,
	usize capacity,
	TypeDesc key_type,
	TypeDesc value_type,
	hash_fn hash,
	eq_fn eq
)
{
	ResultVec keys_result;
	ResultVec values_result;
	ResultVec states_result;
	usize i;
	usize slot_count;
	u8 empty = HASH_BUCKET_EMPTY;

	memset(table, 0, sizeof(*table));
	table->key_type = key_type;
	table->value_type = value_type;
	table->hash = hash;
	table->eq = eq;

	keys_result = vec_new(hash_table_storage_type(key_type));
	if (Vec_is_err(&keys_result)) {
		return HASH_TABLE_OUT_OF_MEMORY;
	}
	table->keys = Vec_unwrap(&keys_result);

	values_result = vec_new(hash_table_storage_type(value_type));
	if (Vec_is_err(&values_result)) {
		hash_table_drop(table);
		return HASH_TABLE_OUT_OF_MEMORY;
	}
	table->values = Vec_unwrap(&values_result);

	states_result = vec_new(Type(u8));
	if (Vec_is_err(&states_result)) {
		hash_table_drop(table);
		return HASH_TABLE_OUT_OF_MEMORY;
	}
	table->states = Vec_unwrap(&states_result);

	if (vec_reserve(&table->keys, capacity) != 0 ||
	    vec_reserve(&table->values, capacity) != 0 ||
	    vec_reserve(&table->states, capacity) != 0) {
		hash_table_drop(table);
		return HASH_TABLE_OUT_OF_MEMORY;
	}

	/*
	 * keys and values are raw slots. Their Vec sizes cover the allocated
	 * slots, but their objects are initialized only after a bucket is occupied.
	 */
	slot_count = table->keys.cap;
	if (table->values.cap < slot_count ||
	    table->states.cap < slot_count) {
		hash_table_drop(table);
		return HASH_TABLE_OUT_OF_MEMORY;
	}
	table->keys.size = slot_count;
	table->values.size = slot_count;
	for (i = 0; i < slot_count; ++i) {
		if (vec_push(&table->states, &empty) != 0) {
			hash_table_drop(table);
			return HASH_TABLE_OUT_OF_MEMORY;
		}
	}
	return HASH_TABLE_OK;
}

static bool hash_table_should_grow(const HashTable *table)
{
	usize threshold;

	threshold = table->keys.size -
		table->keys.size / HASH_TABLE_MAX_LOAD_DENOMINATOR;
	return table->len >= threshold;
}

static usize hash_table_start(const HashTable *table, const void *key)
{
	return table->hash(key) % table->keys.size;
}

/*
 * Finds an existing key or an insertion slot. Deleted slots are remembered
 * but probing continues, because a later occupied slot may contain the key.
 */
static bool hash_table_find_slot(
	const HashTable *table,
	const void *key,
	usize *index,
	bool *found
)
{
	const u8 *states = hash_table_states_const(table);
	usize first_deleted = (usize)-1;
	usize current = hash_table_start(table, key);
	usize probes;

	for (probes = 0; probes < table->keys.size; ++probes) {
		switch (states[current]) {
		case HASH_BUCKET_EMPTY:
			*index = first_deleted == (usize)-1 ? current : first_deleted;
			*found = false;
			return true;
		case HASH_BUCKET_DELETED:
			if (first_deleted == (usize)-1) {
				first_deleted = current;
			}
			break;
		case HASH_BUCKET_OCCUPIED:
			if (table->eq(
				    key,
				    hash_table_key_const(table, current))) {
				*index = current;
				*found = true;
				return true;
			}
			break;
		default:
			return false;
		}
		current = (current + 1) % table->keys.size;
	}

	if (first_deleted != (usize)-1) {
		*index = first_deleted;
		*found = false;
		return true;
	}
	return false;
}

static void hash_table_place_move(
	HashTable *table,
	void *key,
	void *value
)
{
	usize index;
	bool found;

	(void)hash_table_find_slot(table, key, &index, &found);
	obj_move(table->key_type, hash_table_key(table, index), key);
	obj_move(table->value_type, hash_table_value(table, index), value);
	hash_table_states(table)[index] = HASH_BUCKET_OCCUPIED;
	table->len++;
}

static i32 hash_table_resize(HashTable *table, usize capacity)
{
	HashTable resized;
	Vec old_keys;
	Vec old_values;
	Vec old_states;
	const u8 *old_state;
	usize i;
	i32 result;

	result = hash_table_init_storage(
		&resized,
		capacity,
		table->key_type,
		table->value_type,
		table->hash,
		table->eq
	);
	if (result != HASH_TABLE_OK) {
		return result;
	}

	old_state = hash_table_states_const(table);
	for (i = 0; i < table->keys.size; ++i) {
		if (old_state[i] != HASH_BUCKET_OCCUPIED) {
			continue;
		}
		hash_table_place_move(
			&resized,
			hash_table_key(table, i),
			hash_table_value(table, i)
		);
	}

	old_keys = table->keys;
	old_values = table->values;
	old_states = table->states;
	table->keys = resized.keys;
	table->values = resized.values;
	table->states = resized.states;
	table->len = resized.len;
	memset(&resized, 0, sizeof(resized));

	vec_drop(&old_keys);
	vec_drop(&old_values);
	vec_drop(&old_states);
	return HASH_TABLE_OK;
}

ResultHashTable hash_table_new(
	usize capacity,
	TypeDesc key_type,
	TypeDesc value_type,
	hash_fn hash,
	eq_fn eq
)
{
	HashTable table;
	i32 result;

	if (capacity < HASH_TABLE_MIN_CAPACITY ||
	    key_type.size == 0 ||
	    value_type.size == 0 ||
	    hash == NULL ||
	    eq == NULL) {
		return HashTableErr(ERROR_INVALID_ARGUMENT);
	}

	result = hash_table_init_storage(
		&table,
		capacity,
		key_type,
		value_type,
		hash,
		eq
	);
	if (result != HASH_TABLE_OK) {
		return HashTableErr(ERROR_OUT_OF_MEMORY);
	}
	return HashTableOk(table);
}

usize hash_table_len(const HashTable *table)
{
	return table->len;
}

usize hash_table_capacity(const HashTable *table)
{
	return table->keys.size;
}

bool hash_table_is_empty(const HashTable *table)
{
	return table->len == 0;
}

i32 hash_table_insert(HashTable *table, void *key, void *value)
{
	usize index;
	bool found;
	usize new_capacity;

	if (table == NULL || key == NULL || value == NULL) {
		return HASH_TABLE_INVALID_ARGUMENT;
	}

	if (!hash_table_find_slot(table, key, &index, &found)) {
		return HASH_TABLE_OUT_OF_MEMORY;
	}
	if (found) {
		obj_drop(table->value_type, hash_table_value(table, index));
		obj_drop(table->key_type, key);
		obj_move(table->value_type, hash_table_value(table, index), value);
		return HASH_TABLE_OK;
	}

	if (hash_table_should_grow(table)) {
		if (table->keys.size > (usize)-1 / 2) {
			return HASH_TABLE_OUT_OF_MEMORY;
		}
		new_capacity = table->keys.size * 2;
		if (hash_table_resize(table, new_capacity) != HASH_TABLE_OK) {
			return HASH_TABLE_OUT_OF_MEMORY;
		}
		if (!hash_table_find_slot(table, key, &index, &found)) {
			return HASH_TABLE_OUT_OF_MEMORY;
		}
	}

	obj_move(table->key_type, hash_table_key(table, index), key);
	obj_move(table->value_type, hash_table_value(table, index), value);
	hash_table_states(table)[index] = HASH_BUCKET_OCCUPIED;
	table->len++;
	return HASH_TABLE_OK;
}

const void *hash_table_get(const HashTable *table, const void *key)
{
	usize index;
	bool found;

	if (table == NULL || key == NULL ||
	    !hash_table_find_slot(table, key, &index, &found) ||
	    !found) {
		return NULL;
	}
	return hash_table_value_const(table, index);
}

void *hash_table_get_mut(HashTable *table, const void *key)
{
	return (void *)hash_table_get(table, key);
}

bool hash_table_contains(const HashTable *table, const void *key)
{
	return hash_table_get(table, key) != NULL;
}

i32 hash_table_remove(HashTable *table, const void *key, void *out_value)
{
	usize index;
	bool found;

	if (table == NULL || key == NULL) {
		return HASH_TABLE_INVALID_ARGUMENT;
	}
	if (!hash_table_find_slot(table, key, &index, &found) || !found) {
		return HASH_TABLE_NOT_FOUND;
	}

	obj_drop(table->key_type, hash_table_key(table, index));
	if (out_value == NULL) {
		obj_drop(table->value_type, hash_table_value(table, index));
	} else {
		obj_move(
			table->value_type,
			out_value,
			hash_table_value(table, index)
		);
	}
	hash_table_states(table)[index] = HASH_BUCKET_DELETED;
	table->len--;
	return HASH_TABLE_OK;
}

void hash_table_clear(HashTable *table)
{
	usize i;
	u8 empty = HASH_BUCKET_EMPTY;

	if (table == NULL) {
		return;
	}
	for (i = 0; i < table->states.size; ++i) {
		if (hash_table_states(table)[i] == HASH_BUCKET_OCCUPIED) {
			hash_table_drop_slot(table, i);
		} else {
			hash_table_states(table)[i] = empty;
		}
	}
	table->len = 0;
}

void hash_table_drop(void *self)
{
	HashTable *table = self;

	if (table == NULL) {
		return;
	}
	hash_table_clear(table);
	vec_drop(&table->keys);
	vec_drop(&table->values);
	vec_drop(&table->states);
	memset(table, 0, sizeof(*table));
}

void *hash_table_clone_obj(const void *source_ptr)
{
	const HashTable *source = source_ptr;
	HashTable *copy;
	ResultHashTable result;
	usize i;

	copy = malloc(sizeof(*copy));
	if (copy == NULL) {
		return NULL;
	}
	result = hash_table_new(
		source->keys.size,
		source->key_type,
		source->value_type,
		source->hash,
		source->eq
	);
	if (HashTable_is_err(&result)) {
		free(copy);
		return NULL;
	}
	*copy = HashTable_unwrap(&result);

	for (i = 0; i < source->keys.size; ++i) {
		const u8 *states = hash_table_states_const(source);
		void *key_copy;
		void *value_copy;

		if (states[i] != HASH_BUCKET_OCCUPIED) {
			continue;
		}
		key_copy = obj_clone(
			source->key_type,
			hash_table_key_const(source, i)
		);
		if (key_copy == NULL) {
			hash_table_drop(copy);
			free(copy);
			return NULL;
		}
		value_copy = obj_clone(
			source->value_type,
			hash_table_value_const(source, i)
		);
		if (value_copy == NULL) {
			obj_drop(source->key_type, key_copy);
			free(key_copy);
			hash_table_drop(copy);
			free(copy);
			return NULL;
		}
		hash_table_place_move(&*copy, key_copy, value_copy);
		free(key_copy);
		free(value_copy);
	}
	return copy;
}

ResultHashTable hash_table_clone(const HashTable *source)
{
	HashTable *copy;
	HashTable value;

	if (source == NULL) {
		return HashTableErr(ERROR_INVALID_ARGUMENT);
	}
	copy = hash_table_clone_obj(source);
	if (copy == NULL) {
		return HashTableErr(ERROR_OUT_OF_MEMORY);
	}
	value = *copy;
	memset(copy, 0, sizeof(*copy));
	free(copy);
	return HashTableOk(value);
}
