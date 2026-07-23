# Hashtable

Runtime-generic open-addressing hashtable for the `04data_structure` layer.

The C API receives `TypeDesc` values for both keys and values, so copy and
owned objects can be stored in the same table. The caller also supplies hash
and equality callbacks for keys.

Each bucket uses linear probing and has one of these states:

- `HASH_BUCKET_OCCUPIED`
- `HASH_BUCKET_EMPTY`
- `HASH_BUCKET_DELETED`

Keys, values, and states are stored in parallel RawBuf allocations.
`HashTable` alone manages key and value lifetimes using the TypeDesc values
passed to `hash_table_new`; RawBuf only owns allocation and capacity.

On successful `hash_table_insert`, the table takes ownership of the key and
value. Inserting an existing key drops the new key, drops the old value, and
moves the new value into the existing slot. `hash_table_remove` either moves
the value to the caller or drops it when `out_value` is NULL.

The Rust wrapper is `shyos_hashtable::CHashTable<K, V>`. It uses `Hash + Eq`
for key callbacks and installs Rust drop callbacks in the C TypeDesc values.
The C `HashSet` contains one `HashTable` field at offset zero and uses a `u8`
value internally. Rust mirrors it with `#[repr(C)] RawHashSet` and exposes
`#[repr(transparent)] CHashSet<K>`. Its methods call the C `hash_set_*`
functions directly, so the same object layout crosses the FFI boundary in
both directions.
