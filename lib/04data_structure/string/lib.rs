#![no_std]

use core::borrow::Borrow;
use core::ffi::{c_char, c_void, CStr};
use core::fmt;
use core::ops::Deref;
use shyos_rawbuf::RawBuf;
use shyos_result::{CResult, ERROR_INVALID_ARGUMENT, ERROR_NO_VALUE, ERROR_OUT_OF_MEMORY};

type StringRetainFn = unsafe extern "C" fn(u8, *mut c_void) -> bool;
pub use string_no_alloc::*;

#[repr(C)]
pub struct RawString {
    pub raw: RawBuf,
    pub len: usize,
}

unsafe extern "C" {
    #[link_name = "string_new_n"]
    fn c_string_new_n(value: *const c_char, len: usize) -> CResult<RawString>;

    #[link_name = "string_new_with_capacity"]
    fn c_string_new_with_capacity(capacity: usize) -> CResult<RawString>;

    #[link_name = "string_clone"]
    fn c_string_clone(value: *const RawString) -> CResult<RawString>;

    #[link_name = "string_drop"]
    fn c_string_drop(value: *mut RawString);

    #[link_name = "string_len"]
    fn c_string_len(value: *const RawString) -> usize;

    #[link_name = "string_capacity"]
    fn c_string_capacity(value: *const RawString) -> usize;

    #[link_name = "string_as_ptr"]
    fn c_string_as_ptr(value: *const RawString) -> *const c_char;

    #[link_name = "string_reserve"]
    fn c_string_reserve(value: *mut RawString, additional: usize) -> i32;

    #[link_name = "string_push"]
    fn c_string_push(value: *mut RawString, byte: u8) -> i32;

    #[link_name = "string_push_str_n"]
    fn c_string_push_str_n(value: *mut RawString, data: *const c_char, len: usize) -> i32;

    #[link_name = "string_insert"]
    fn c_string_insert(value: *mut RawString, index: usize, byte: u8) -> i32;

    #[link_name = "string_insert_str_n"]
    fn c_string_insert_str_n(
        value: *mut RawString,
        index: usize,
        data: *const c_char,
        len: usize,
    ) -> i32;

    #[link_name = "string_remove"]
    fn c_string_remove(value: *mut RawString, index: usize, out: *mut u8) -> i32;

    #[link_name = "string_split_off"]
    fn c_string_split_off(value: *mut RawString, at: usize) -> CResult<RawString>;

    #[link_name = "string_retain"]
    fn c_string_retain(value: *mut RawString, keep: Option<StringRetainFn>, context: *mut c_void);

    #[link_name = "string_clear"]
    fn c_string_clear(value: *mut RawString);

    #[link_name = "string_truncate"]
    fn c_string_truncate(value: *mut RawString, len: usize) -> i32;

    #[link_name = "string_pop"]
    fn c_string_pop(value: *mut RawString, out: *mut u8) -> i32;
}

#[repr(transparent)]
pub struct CString {
    raw: RawString,
}

unsafe impl Send for CString {}
unsafe impl Sync for CString {}

impl CString {
    pub fn new() -> CResult<Self> {
        Self::from_bytes(b"")
    }

    pub fn with_capacity(capacity: usize) -> CResult<Self> {
        match unsafe { c_string_new_with_capacity(capacity) } {
            CResult::Ok(raw) => CResult::ok(Self { raw }),
            CResult::Err(error) => CResult::err(error),
        }
    }

    pub fn from_c_str(value: &CStr) -> CResult<Self> {
        match unsafe { c_string_new_n(value.as_ptr(), value.to_bytes().len()) } {
            CResult::Ok(raw) => CResult::ok(Self { raw }),
            CResult::Err(error) => CResult::err(error),
        }
    }

    pub fn from_bytes(value: &[u8]) -> CResult<Self> {
        if value.contains(&0) {
            return CResult::err(ERROR_INVALID_ARGUMENT);
        }
        match unsafe { c_string_new_n(value.as_ptr().cast(), value.len()) } {
            CResult::Ok(raw) => CResult::ok(Self { raw }),
            CResult::Err(error) => CResult::err(error),
        }
    }

    pub fn len(&self) -> usize {
        unsafe { c_string_len(&self.raw) }
    }

    pub fn capacity(&self) -> usize {
        unsafe { c_string_capacity(&self.raw) }
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    pub fn as_ptr(&self) -> *const c_char {
        unsafe { c_string_as_ptr(&self.raw) }
    }

    pub fn as_c_str(&self) -> &CStr {
        unsafe { CStr::from_ptr(self.as_ptr()) }
    }

    pub fn as_bytes(&self) -> &[u8] {
        self.as_c_str().to_bytes()
    }

    pub fn reserve(&mut self, additional: usize) -> CResult<()> {
        let result = unsafe { c_string_reserve(&mut self.raw, additional) };
        if result == 0 {
            CResult::ok(())
        } else {
            CResult::err(ERROR_OUT_OF_MEMORY)
        }
    }

    pub fn push(&mut self, byte: u8) -> CResult<()> {
        let result = unsafe { c_string_push(&mut self.raw, byte) };
        match result {
            0 => CResult::ok(()),
            -2 => CResult::err(ERROR_INVALID_ARGUMENT),
            _ => CResult::err(ERROR_OUT_OF_MEMORY),
        }
    }

    pub fn push_str(&mut self, value: &CStr) -> CResult<()> {
        let result =
            unsafe { c_string_push_str_n(&mut self.raw, value.as_ptr(), value.to_bytes().len()) };
        if result == 0 {
            CResult::ok(())
        } else {
            CResult::err(ERROR_OUT_OF_MEMORY)
        }
    }

    pub fn insert(&mut self, index: usize, byte: u8) -> CResult<()> {
        assert!(index <= self.len(), "string insertion index out of bounds");
        let result = unsafe { c_string_insert(&mut self.raw, index, byte) };
        match result {
            0 => CResult::ok(()),
            -2 => CResult::err(ERROR_INVALID_ARGUMENT),
            _ => CResult::err(ERROR_OUT_OF_MEMORY),
        }
    }

    pub fn insert_str(&mut self, index: usize, value: &CStr) -> CResult<()> {
        assert!(index <= self.len(), "string insertion index out of bounds");
        let result = unsafe {
            c_string_insert_str_n(&mut self.raw, index, value.as_ptr(), value.to_bytes().len())
        };
        if result == 0 {
            CResult::ok(())
        } else {
            CResult::err(ERROR_OUT_OF_MEMORY)
        }
    }

    pub fn remove(&mut self, index: usize) -> u8 {
        assert!(index < self.len(), "string removal index out of bounds");
        let mut byte = 0;
        let result = unsafe { c_string_remove(&mut self.raw, index, &mut byte) };
        assert!(result == 0, "C string remove failed");
        byte
    }

    pub fn split_off(&mut self, at: usize) -> CResult<Self> {
        assert!(at <= self.len(), "string split index out of bounds");
        match unsafe { c_string_split_off(&mut self.raw, at) } {
            CResult::Ok(raw) => CResult::ok(Self { raw }),
            CResult::Err(error) => CResult::err(error),
        }
    }

    pub fn retain<F>(&mut self, mut keep: F)
    where
        F: FnMut(u8) -> bool,
    {
        unsafe extern "C" fn call<F: FnMut(u8) -> bool>(value: u8, context: *mut c_void) -> bool {
            unsafe { (&mut *context.cast::<F>())(value) }
        }

        let context = (&mut keep as *mut F).cast::<c_void>();
        unsafe { c_string_retain(&mut self.raw, Some(call::<F>), context) }
    }

    pub fn clear(&mut self) {
        unsafe { c_string_clear(&mut self.raw) }
    }

    pub fn truncate(&mut self, len: usize) {
        assert!(len <= self.len(), "string truncate out of bounds");
        let result = unsafe { c_string_truncate(&mut self.raw, len) };
        assert!(result == 0, "C string truncate failed");
    }

    pub fn pop(&mut self) -> CResult<u8> {
        let mut byte = 0;
        let result = unsafe { c_string_pop(&mut self.raw, &mut byte) };
        if result == 0 {
            CResult::ok(byte)
        } else {
            CResult::err(ERROR_NO_VALUE)
        }
    }
}

impl Clone for CString {
    fn clone(&self) -> Self {
        match unsafe { c_string_clone(&self.raw) } {
            CResult::Ok(raw) => Self { raw },
            CResult::Err(_) => panic!("CString clone allocation failed"),
        }
    }
}

impl Deref for CString {
    type Target = CStr;

    fn deref(&self) -> &Self::Target {
        self.as_c_str()
    }
}

impl AsRef<CStr> for CString {
    fn as_ref(&self) -> &CStr {
        self.as_c_str()
    }
}

impl Borrow<CStr> for CString {
    fn borrow(&self) -> &CStr {
        self.as_c_str()
    }
}

impl fmt::Debug for CString {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.as_c_str().fmt(formatter)
    }
}

impl PartialEq for CString {
    fn eq(&self, other: &Self) -> bool {
        self.as_bytes() == other.as_bytes()
    }
}

impl Eq for CString {}

impl Drop for CString {
    fn drop(&mut self) {
        unsafe { c_string_drop(&mut self.raw) }
    }
}
