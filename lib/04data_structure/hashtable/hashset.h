#ifndef SHYOS_HASHSET_H
#define SHYOS_HASHSET_H

#include "hashtable.h"

enum {
	HASH_SET_ALREADY_PRESENT = 0,
	HASH_SET_INSERTED = 1,
};

typedef struct HashSet HashSet;
struct HashSet {
	HashTable table;
};

void hash_set_drop(void *self);
void *hash_set_clone_obj(const void *source);

IMPL_SHY_OWNED_RESULT(HashSet, hash_set_drop, hash_set_clone_obj)

ResultHashSet hash_set_new(
	usize capacity,
	TypeDesc key_type,
	hash_fn hash,
	eq_fn eq
);

ResultHashSet hash_set_clone(const HashSet *source);

usize hash_set_len(const HashSet *set);
usize hash_set_capacity(const HashSet *set);
bool hash_set_is_empty(const HashSet *set);
bool hash_set_contains(const HashSet *set, const void *value);

/*
 * On HASH_SET_INSERTED or HASH_SET_ALREADY_PRESENT, insert consumes value.
 * A duplicate value is dropped because the existing value remains in the set.
 */
i32 hash_set_insert(HashSet *set, void *value);
i32 hash_set_remove(HashSet *set, const void *value);

void hash_set_clear(HashSet *set);

#endif /* SHYOS_HASHSET_H */
