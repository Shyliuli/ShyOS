//C 实现原子操作的 Rust 最小包装

#![no_std]

unsafe extern "C" {
    #[link_name = "xchg"]
    fn c_xchg(ptr: *mut i32, new_value: i32) -> i32;
}

/// 原子地把 `*ptr` 换成 `new_value`，返回旧值。
///
/// 安全性：调用方必须保证 `ptr` 指向一个有效且对齐的 `i32`。
pub unsafe fn xchg(ptr: *mut i32, new_value: i32) -> i32 {
    unsafe { c_xchg(ptr, new_value) }
}
