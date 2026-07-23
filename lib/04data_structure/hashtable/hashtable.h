#ifndef SHYOS_HASHTABLE_H
#define SHYOS_HASHTABLE_H

#include "obj.h"
#include "rawbuf.h"
#include "result.h"

typedef usize (*hash_fn)(const void *key);
typedef bool (*eq_fn)(const void *left, const void *right);

enum {
	HASH_BUCKET_OCCUPIED = 0,
	HASH_BUCKET_EMPTY = 1,
	HASH_BUCKET_DELETED = 2,
};

enum {
	HASH_TABLE_OK = 0,
	HASH_TABLE_OUT_OF_MEMORY = -1,
	HASH_TABLE_INVALID_ARGUMENT = -2,
	HASH_TABLE_NOT_FOUND = -3,
};

typedef struct HashTable HashTable;
struct HashTable {
	RawBuf keys;
	RawBuf values;
	RawBuf states;
	usize len;
	TypeDesc key_type;
	TypeDesc value_type;
	hash_fn hash;
	eq_fn eq;
};

void hash_table_drop(void *self);
void *hash_table_clone_obj(const void *source);

IMPL_SHY_OWNED_RESULT(HashTable, hash_table_drop, hash_table_clone_obj)

ResultHashTable hash_table_new(
	usize capacity,
	TypeDesc key_type,
	TypeDesc value_type,
	hash_fn hash,
	eq_fn eq
);

ResultHashTable hash_table_clone(const HashTable *source);

usize hash_table_len(const HashTable *table);
usize hash_table_capacity(const HashTable *table);
bool hash_table_is_empty(const HashTable *table);

/*
 * On success, insert takes ownership of key and value.
 * If key already exists, the old value is dropped and replaced; the new key
 * is dropped because the existing key remains in the table.
 */
i32 hash_table_insert(HashTable *table, void *key, void *value);

const void *hash_table_get(const HashTable *table, const void *key);
void *hash_table_get_mut(HashTable *table, const void *key);
bool hash_table_contains(const HashTable *table, const void *key);

/*
 * Removes key. If out_value is non-NULL, value is moved into it; otherwise
 * value is dropped. The caller owns out_value after a successful removal.
 */
i32 hash_table_remove(HashTable *table, const void *key, void *out_value);

void hash_table_clear(HashTable *table);

#endif /* SHYOS_HASHTABLE_H */
