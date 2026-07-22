#![no_std]

use core::ffi::c_void;

pub type TypeDropFn = unsafe extern "C" fn(*mut c_void);
pub type TypeCloneFn = unsafe extern "C" fn(*const c_void) -> *mut c_void;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct TypeDesc {
    pub size: usize,
    //Option<函数指针> 受rust承诺在ffi边界为:
    //NULL :None
    //非NULL :Some(函数指针)
    pub drop: Option<TypeDropFn>,
    pub clone: Option<TypeCloneFn>,
}

impl TypeDesc {
    pub const fn copy<T>() -> Self {
        Self {
            size: core::mem::size_of::<T>(),
            drop: None,
            clone: None,
        }
    }

    pub const fn is_copy(self) -> bool {
        self.drop.is_none()
    }

    pub const fn is_owned(self) -> bool {
        self.drop.is_some()
    }
}
