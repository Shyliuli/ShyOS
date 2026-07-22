#![no_std]

use core::ffi::c_void;
use core::fmt;
use core::marker::PhantomData;
use core::mem::{align_of, size_of, ManuallyDrop, MaybeUninit};
use core::ops::{Deref, DerefMut};
use core::ptr::{self, NonNull};
use core::slice;
use shyos_obj::TypeDesc;
use shyos_result::{CResult, ERROR_INVALID_ARGUMENT, ERROR_NO_VALUE, ERROR_OUT_OF_MEMORY};

#[repr(C)]
#[doc(hidden)]
pub struct RawVec {
    pub data: *mut c_void,
    pub cap: usize,
    pub size: usize,
    pub elem: TypeDesc,
}

unsafe extern "C" {
    #[link_name = "vec_new"]
    fn c_vec_new(elem: TypeDesc) -> CResult<RawVec>;

    #[link_name = "vec_reserve"]
    fn c_vec_reserve(vec: *mut RawVec, additional: usize) -> i32;

    #[link_name = "vec_push_raw"]
    fn c_vec_push_raw(vec: *mut RawVec, data: *const c_void) -> i32;

    #[link_name = "vec_insert_raw"]
    fn c_vec_insert_raw(vec: *mut RawVec, index: usize, data: *const c_void) -> i32;

    #[link_name = "vec_pop"]
    fn c_vec_pop(vec: *mut RawVec, out: *mut c_void) -> i32;

    #[link_name = "vec_remove"]
    fn c_vec_remove(vec: *mut RawVec, index: usize, out: *mut c_void) -> i32;

    #[link_name = "vec_swap_remove"]
    fn c_vec_swap_remove(vec: *mut RawVec, index: usize, out: *mut c_void) -> i32;

    #[link_name = "vec_truncate"]
    fn c_vec_truncate(vec: *mut RawVec, len: usize);

    #[link_name = "vec_clear"]
    fn c_vec_clear(vec: *mut RawVec);

    #[link_name = "vec_drop"]
    fn c_vec_drop(vec: *mut RawVec);
}
//FFI可传递！
#[repr(transparent)]
pub struct CVec<T> {
    raw: RawVec,
    marker: PhantomData<T>,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct CVecAllocError;

unsafe impl<T: Send> Send for CVec<T> {}
unsafe impl<T: Sync> Sync for CVec<T> {}

unsafe extern "C" fn cvec_drop_element<T>(element: *mut c_void) {
    ptr::drop_in_place(element.cast::<T>());
}

impl<T> CVec<T> {
    fn type_desc() -> TypeDesc {
        TypeDesc {
            size: size_of::<T>(),
            drop: Some(cvec_drop_element::<T>),
            clone: None,
        }
    }

    pub fn new() -> CResult<Self> {
        if size_of::<T>() == 0 || align_of::<T>() > align_of::<usize>() {
            return CResult::err(ERROR_INVALID_ARGUMENT);
        }

        match unsafe { c_vec_new(Self::type_desc()) } {
            CResult::Ok(raw) => CResult::ok(Self {
                raw,
                marker: PhantomData,
            }),
            CResult::Err(error) => CResult::err(error),
        }
    }

    pub fn with_capacity(capacity: usize) -> CResult<Self> {
        let mut vec = match Self::new() {
            CResult::Ok(vec) => vec,
            CResult::Err(error) => return CResult::err(error),
        };
        if let CResult::Err(error) = vec.reserve(capacity) {
            return CResult::err(error);
        }
        CResult::ok(vec)
    }

    pub fn len(&self) -> usize {
        self.raw.size
    }

    pub fn capacity(&self) -> usize {
        self.raw.cap
    }

    pub fn is_empty(&self) -> bool {
        self.raw.size == 0
    }

    pub fn as_ptr(&self) -> *const T {
        if self.raw.data.is_null() {
            NonNull::<T>::dangling().as_ptr()
        } else {
            self.raw.data.cast::<T>()
        }
    }

    pub fn as_mut_ptr(&mut self) -> *mut T {
        if self.raw.data.is_null() {
            NonNull::<T>::dangling().as_ptr()
        } else {
            self.raw.data.cast::<T>()
        }
    }

    pub fn from_slice(values: &[T]) -> CResult<Self>
    where
        T: Clone,
    {
        let mut vec = match Self::with_capacity(values.len()) {
            CResult::Ok(vec) => vec,
            CResult::Err(error) => return CResult::err(error),
        };
        for value in values {
            if let CResult::Err(error) = vec.push(value.clone()) {
                return CResult::err(error);
            }
        }
        CResult::ok(vec)
    }

    /// Takes ownership of an allocation described by its raw components.
    ///
    /// # Safety
    ///
    /// `data` must come from the active ShyOS allocator for `cap` elements of
    /// `T`, or be a correctly aligned dangling pointer when `cap` is zero.
    /// The first `len` elements must be initialized and `len <= cap`.
    pub unsafe fn from_raw_parts(data: *mut T, len: usize, cap: usize) -> Self {
        assert!(
            size_of::<T>() != 0,
            "CVec does not support zero-sized types"
        );
        assert!(
            align_of::<T>() <= align_of::<usize>(),
            "CVec does not support over-aligned types"
        );
        assert!(!data.is_null(), "CVec raw pointer must not be null");
        assert!(len <= cap, "CVec raw length exceeds capacity");

        Self {
            raw: RawVec {
                data: data.cast(),
                cap,
                size: len,
                elem: Self::type_desc(),
            },
            marker: PhantomData,
        }
    }

    pub fn into_raw_parts(self) -> (*mut T, usize, usize) {
        let this = ManuallyDrop::new(self);
        let data = if this.raw.data.is_null() {
            NonNull::<T>::dangling().as_ptr()
        } else {
            this.raw.data.cast::<T>()
        };
        (data, this.raw.size, this.raw.cap)
    }

    pub fn reserve(&mut self, additional: usize) -> CResult<()> {
        if unsafe { c_vec_reserve(&mut self.raw, additional) } == 0 {
            CResult::ok(())
        } else {
            CResult::err(ERROR_OUT_OF_MEMORY)
        }
    }

    pub fn push(&mut self, value: T) -> CResult<()> {
        let value = ManuallyDrop::new(value);
        let result =
            unsafe { c_vec_push_raw(&mut self.raw, (&*value as *const T).cast::<c_void>()) };

        if result == 0 {
            CResult::ok(())
        } else {
            drop(ManuallyDrop::into_inner(value));
            CResult::err(ERROR_OUT_OF_MEMORY)
        }
    }

    pub fn pop(&mut self) -> CResult<T> {
        let mut value = MaybeUninit::<T>::uninit();
        let result = unsafe { c_vec_pop(&mut self.raw, value.as_mut_ptr().cast()) };
        if result == 0 {
            CResult::ok(unsafe { value.assume_init() })
        } else {
            CResult::err(ERROR_NO_VALUE)
        }
    }

    pub fn as_slice(&self) -> &[T] {
        unsafe { slice::from_raw_parts(self.as_ptr(), self.raw.size) }
    }

    pub fn as_mut_slice(&mut self) -> &mut [T] {
        let len = self.raw.size;
        unsafe { slice::from_raw_parts_mut(self.as_mut_ptr(), len) }
    }

    pub fn insert(&mut self, index: usize, value: T) -> CResult<()> {
        assert!(index <= self.len(), "insertion index out of bounds");

        let value = ManuallyDrop::new(value);
        let result = unsafe {
            c_vec_insert_raw(&mut self.raw, index, (&*value as *const T).cast::<c_void>())
        };
        if result == 0 {
            CResult::ok(())
        } else {
            drop(ManuallyDrop::into_inner(value));
            CResult::err(ERROR_OUT_OF_MEMORY)
        }
    }

    pub fn remove(&mut self, index: usize) -> T {
        assert!(index < self.len(), "removal index out of bounds");

        let mut value = MaybeUninit::<T>::uninit();
        let result =
            unsafe { c_vec_remove(&mut self.raw, index, value.as_mut_ptr().cast::<c_void>()) };
        assert!(result == 0, "C vec_remove failed");
        unsafe { value.assume_init() }
    }

    pub fn swap_remove(&mut self, index: usize) -> T {
        assert!(index < self.len(), "swap_remove index out of bounds");

        let mut value = MaybeUninit::<T>::uninit();
        let result =
            unsafe { c_vec_swap_remove(&mut self.raw, index, value.as_mut_ptr().cast::<c_void>()) };
        assert!(result == 0, "C vec_swap_remove failed");
        unsafe { value.assume_init() }
    }

    pub fn truncate(&mut self, len: usize) {
        unsafe { c_vec_truncate(&mut self.raw, len) }
    }

    pub fn clear(&mut self) {
        unsafe { c_vec_clear(&mut self.raw) }
    }

    pub fn retain(&mut self, mut keep: impl FnMut(&T) -> bool) {
        let mut index = 0;
        while index < self.len() {
            if keep(&self[index]) {
                index += 1;
            } else {
                drop(self.remove(index));
            }
        }
    }

    pub fn extend_from_slice(&mut self, values: &[T]) -> CResult<()>
    where
        T: Clone,
    {
        for value in values {
            if let CResult::Err(error) = self.push(value.clone()) {
                return CResult::err(error);
            }
        }
        CResult::ok(())
    }
}

impl<T: Clone> Clone for CVec<T> {
    fn clone(&self) -> Self {
        Self::from_slice(self.as_slice()).unwrap()
    }
}

impl<T> Deref for CVec<T> {
    type Target = [T];

    fn deref(&self) -> &Self::Target {
        self.as_slice()
    }
}

impl<T> DerefMut for CVec<T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.as_mut_slice()
    }
}

impl<T> AsRef<[T]> for CVec<T> {
    fn as_ref(&self) -> &[T] {
        self.as_slice()
    }
}

impl<T> AsMut<[T]> for CVec<T> {
    fn as_mut(&mut self) -> &mut [T] {
        self.as_mut_slice()
    }
}

impl<T: Clone> TryFrom<&[T]> for CVec<T> {
    type Error = CVecAllocError;

    fn try_from(values: &[T]) -> Result<Self, Self::Error> {
        match Self::from_slice(values) {
            CResult::Ok(vec) => Ok(vec),
            CResult::Err(_) => Err(CVecAllocError),
        }
    }
}

impl<T: fmt::Debug> fmt::Debug for CVec<T> {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        formatter
            .debug_list()
            .entries(self.as_slice().iter())
            .finish()
    }
}

impl<T: PartialEq> PartialEq for CVec<T> {
    fn eq(&self, other: &Self) -> bool {
        self.as_slice() == other.as_slice()
    }
}

impl<T: Eq> Eq for CVec<T> {}

impl<'a, T> IntoIterator for &'a CVec<T> {
    type Item = &'a T;
    type IntoIter = slice::Iter<'a, T>;

    fn into_iter(self) -> Self::IntoIter {
        self.as_slice().iter()
    }
}

impl<'a, T> IntoIterator for &'a mut CVec<T> {
    type Item = &'a mut T;
    type IntoIter = slice::IterMut<'a, T>;

    fn into_iter(self) -> Self::IntoIter {
        self.as_mut_slice().iter_mut()
    }
}

impl<T> Drop for CVec<T> {
    fn drop(&mut self) {
        unsafe { c_vec_drop(&mut self.raw) }
    }
}
