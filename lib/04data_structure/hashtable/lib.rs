#![no_std]

use core::ffi::c_void;
use core::hash::{Hash, Hasher};
use core::marker::PhantomData;
use core::mem::{align_of, size_of, ManuallyDrop, MaybeUninit};
use core::ptr;
use shyos_obj::TypeDesc;
use shyos_result::{
    CResult, ERROR_INVALID_ARGUMENT, ERROR_NO_VALUE, ERROR_OUT_OF_MEMORY,
};
use shyos_vec::RawVec;

const HASH_TABLE_MIN_CAPACITY: usize = 8;
const HASH_TABLE_OK: i32 = 0;
const HASH_TABLE_OUT_OF_MEMORY: i32 = -1;
const HASH_TABLE_INVALID_ARGUMENT: i32 = -2;
const HASH_TABLE_NOT_FOUND: i32 = -3;
const HASH_SET_ALREADY_PRESENT: i32 = 0;
const HASH_SET_INSERTED: i32 = 1;

type HashFn = unsafe extern "C" fn(*const c_void) -> usize;
type EqFn = unsafe extern "C" fn(*const c_void, *const c_void) -> bool;

#[repr(C)]
#[doc(hidden)]
pub struct RawHashTable {
    pub keys: RawVec,
    pub values: RawVec,
    pub states: RawVec,
    pub len: usize,
    pub key_type: TypeDesc,
    pub value_type: TypeDesc,
    hash: Option<HashFn>,
    eq: Option<EqFn>,
}

#[repr(C)]
#[doc(hidden)]
pub struct RawHashSet {
    pub table: RawHashTable,
}

const _: () = {
    assert!(size_of::<RawHashSet>() == size_of::<RawHashTable>());
    assert!(align_of::<RawHashSet>() == align_of::<RawHashTable>());
};

unsafe extern "C" {
    #[link_name = "hash_table_new"]
    fn c_hash_table_new(
        capacity: usize,
        key_type: TypeDesc,
        value_type: TypeDesc,
        hash: Option<HashFn>,
        eq: Option<EqFn>,
    ) -> CResult<RawHashTable>;

    #[link_name = "hash_table_capacity"]
    fn c_hash_table_capacity(table: *const RawHashTable) -> usize;

    #[link_name = "hash_table_insert"]
    fn c_hash_table_insert(
        table: *mut RawHashTable,
        key: *mut c_void,
        value: *mut c_void,
    ) -> i32;

    #[link_name = "hash_table_get"]
    fn c_hash_table_get(
        table: *const RawHashTable,
        key: *const c_void,
    ) -> *const c_void;

    #[link_name = "hash_table_get_mut"]
    fn c_hash_table_get_mut(
        table: *mut RawHashTable,
        key: *const c_void,
    ) -> *mut c_void;

    #[link_name = "hash_table_remove"]
    fn c_hash_table_remove(
        table: *mut RawHashTable,
        key: *const c_void,
        out_value: *mut c_void,
    ) -> i32;

    #[link_name = "hash_table_clear"]
    fn c_hash_table_clear(table: *mut RawHashTable);

    #[link_name = "hash_table_drop"]
    fn c_hash_table_drop(table: *mut RawHashTable);

    #[link_name = "hash_set_new"]
    fn c_hash_set_new(
        capacity: usize,
        key_type: TypeDesc,
        hash: Option<HashFn>,
        eq: Option<EqFn>,
    ) -> CResult<RawHashSet>;

    #[link_name = "hash_set_len"]
    fn c_hash_set_len(set: *const RawHashSet) -> usize;

    #[link_name = "hash_set_capacity"]
    fn c_hash_set_capacity(set: *const RawHashSet) -> usize;

    #[link_name = "hash_set_contains"]
    fn c_hash_set_contains(set: *const RawHashSet, value: *const c_void) -> bool;

    #[link_name = "hash_set_insert"]
    fn c_hash_set_insert(set: *mut RawHashSet, value: *mut c_void) -> i32;

    #[link_name = "hash_set_remove"]
    fn c_hash_set_remove(set: *mut RawHashSet, value: *const c_void) -> i32;

    #[link_name = "hash_set_clear"]
    fn c_hash_set_clear(set: *mut RawHashSet);

    #[link_name = "hash_set_drop"]
    fn c_hash_set_drop(set: *mut RawHashSet);
}

struct ShyHasher(u64);

impl ShyHasher {
    const OFFSET: u64 = 0xcbf29ce484222325;
    const PRIME: u64 = 0x100000001b3;

    const fn new() -> Self {
        Self(Self::OFFSET)
    }
}

impl Hasher for ShyHasher {
    fn finish(&self) -> u64 {
        self.0
    }

    fn write(&mut self, bytes: &[u8]) {
        for byte in bytes {
            self.0 ^= u64::from(*byte);
            self.0 = self.0.wrapping_mul(Self::PRIME);
        }
    }
}

unsafe extern "C" fn drop_value<T>(value: *mut c_void) {
    unsafe { ptr::drop_in_place(value.cast::<T>()) }
}

unsafe extern "C" fn hash_key<K: Hash>(key: *const c_void) -> usize {
    let key = unsafe { &*key.cast::<K>() };
    let mut hasher = ShyHasher::new();
    key.hash(&mut hasher);
    hasher.finish() as usize
}

unsafe extern "C" fn eq_key<K: Eq>(
    left: *const c_void,
    right: *const c_void,
) -> bool {
    unsafe { &*left.cast::<K>() == &*right.cast::<K>() }
}

fn type_desc<T>() -> TypeDesc {
    TypeDesc {
        size: size_of::<T>(),
        drop: Some(drop_value::<T>),
        clone: None,
    }
}

fn status_error(status: i32) -> i32 {
    match status {
        HASH_TABLE_INVALID_ARGUMENT => ERROR_INVALID_ARGUMENT,
        HASH_TABLE_NOT_FOUND => ERROR_NO_VALUE,
        HASH_TABLE_OUT_OF_MEMORY => ERROR_OUT_OF_MEMORY,
        _ => ERROR_INVALID_ARGUMENT,
    }
}

#[repr(transparent)]
pub struct CHashTable<K, V> {
    raw: RawHashTable,
    marker: PhantomData<(K, V)>,
}

unsafe impl<K: Send, V: Send> Send for CHashTable<K, V> {}
unsafe impl<K: Sync, V: Sync> Sync for CHashTable<K, V> {}

#[repr(transparent)]
pub struct CHashSet<K> {
    raw: RawHashSet,
    marker: PhantomData<K>,
}

unsafe impl<K: Send> Send for CHashSet<K> {}
unsafe impl<K: Sync> Sync for CHashSet<K> {}

impl<K: Hash + Eq, V> CHashTable<K, V> {
    pub fn new() -> CResult<Self> {
        Self::with_capacity(HASH_TABLE_MIN_CAPACITY)
    }

    pub fn with_capacity(capacity: usize) -> CResult<Self> {
        if capacity < HASH_TABLE_MIN_CAPACITY
            || size_of::<K>() == 0
            || size_of::<V>() == 0
            || align_of::<K>() > align_of::<usize>()
            || align_of::<V>() > align_of::<usize>()
        {
            return CResult::err(ERROR_INVALID_ARGUMENT);
        }

        match unsafe {
            c_hash_table_new(
                capacity,
                type_desc::<K>(),
                type_desc::<V>(),
                Some(hash_key::<K>),
                Some(eq_key::<K>),
            )
        } {
            CResult::Ok(raw) => CResult::ok(Self {
                raw,
                marker: PhantomData,
            }),
            CResult::Err(error) => CResult::err(error),
        }
    }

    pub fn len(&self) -> usize {
        self.raw.len
    }

    pub fn capacity(&self) -> usize {
        unsafe { c_hash_table_capacity(&self.raw) }
    }

    pub fn is_empty(&self) -> bool {
        self.raw.len == 0
    }

    pub fn insert(&mut self, key: K, value: V) -> CResult<()> {
        let mut key = ManuallyDrop::new(key);
        let mut value = ManuallyDrop::new(value);
        let status = unsafe {
            c_hash_table_insert(
                &mut self.raw,
                (&mut *key as *mut K).cast(),
                (&mut *value as *mut V).cast(),
            )
        };

        if status == HASH_TABLE_OK {
            CResult::ok(())
        } else {
            drop(ManuallyDrop::into_inner(key));
            drop(ManuallyDrop::into_inner(value));
            CResult::err(status_error(status))
        }
    }

    pub fn get(&self, key: &K) -> Option<&V> {
        let value = unsafe {
            c_hash_table_get(
                &self.raw,
                (key as *const K).cast::<c_void>(),
            )
        };
        unsafe { value.cast::<V>().as_ref() }
    }

    pub fn get_mut(&mut self, key: &K) -> Option<&mut V> {
        let value = unsafe {
            c_hash_table_get_mut(
                &mut self.raw,
                (key as *const K).cast::<c_void>(),
            )
        };
        unsafe { value.cast::<V>().as_mut() }
    }

    pub fn contains_key(&self, key: &K) -> bool {
        self.get(key).is_some()
    }

    pub fn remove(&mut self, key: &K) -> CResult<V> {
        let mut value = MaybeUninit::<V>::uninit();
        let status = unsafe {
            c_hash_table_remove(
                &mut self.raw,
                (key as *const K).cast::<c_void>(),
                value.as_mut_ptr().cast(),
            )
        };
        if status == HASH_TABLE_OK {
            CResult::ok(unsafe { value.assume_init() })
        } else {
            CResult::err(status_error(status))
        }
    }

    pub fn clear(&mut self) {
        unsafe { c_hash_table_clear(&mut self.raw) }
    }
}

impl<K, V> Drop for CHashTable<K, V> {
    fn drop(&mut self) {
        unsafe { c_hash_table_drop(&mut self.raw) }
    }
}

impl<K: Hash + Eq> CHashSet<K> {
    pub fn new() -> CResult<Self> {
        Self::with_capacity(HASH_TABLE_MIN_CAPACITY)
    }

    pub fn with_capacity(capacity: usize) -> CResult<Self> {
        if capacity < HASH_TABLE_MIN_CAPACITY
            || size_of::<K>() == 0
            || align_of::<K>() > align_of::<usize>()
        {
            return CResult::err(ERROR_INVALID_ARGUMENT);
        }

        match unsafe {
            c_hash_set_new(
                capacity,
                type_desc::<K>(),
                Some(hash_key::<K>),
                Some(eq_key::<K>),
            )
        } {
            CResult::Ok(raw) => CResult::ok(Self {
                raw,
                marker: PhantomData,
            }),
            CResult::Err(error) => CResult::err(error),
        }
    }

    pub fn len(&self) -> usize {
        unsafe { c_hash_set_len(&self.raw) }
    }

    pub fn capacity(&self) -> usize {
        unsafe { c_hash_set_capacity(&self.raw) }
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    pub fn contains(&self, value: &K) -> bool {
        unsafe {
            c_hash_set_contains(
                &self.raw,
                (value as *const K).cast::<c_void>(),
            )
        }
    }

    pub fn insert(&mut self, value: K) -> CResult<bool> {
        let mut value = ManuallyDrop::new(value);
        let status = unsafe {
            c_hash_set_insert(
                &mut self.raw,
                (&mut *value as *mut K).cast::<c_void>(),
            )
        };
        match status {
            HASH_SET_INSERTED => CResult::ok(true),
            HASH_SET_ALREADY_PRESENT => CResult::ok(false),
            _ => {
                drop(ManuallyDrop::into_inner(value));
                CResult::err(status_error(status))
            }
        }
    }

    pub fn remove(&mut self, value: &K) -> bool {
        unsafe {
            c_hash_set_remove(
                &mut self.raw,
                (value as *const K).cast::<c_void>(),
            ) == HASH_TABLE_OK
        }
    }

    pub fn clear(&mut self) {
        unsafe { c_hash_set_clear(&mut self.raw) }
    }
}

impl<K> Drop for CHashSet<K> {
    fn drop(&mut self) {
        unsafe { c_hash_set_drop(&mut self.raw) }
    }
}
