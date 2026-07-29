/* linux_user_signal 的 Rust 包装：用信号掩码模拟 CPU 中断屏蔽。
 * 仅 linux_user backend 提供实现，其余 backend 为空 crate。
 */
#![no_std]
#![cfg(SHYOS_BACKEND_LINUX_USER)]

unsafe extern "C" {
    #[link_name = "signal_disable"]
    fn c_signal_disable(mask: u64) -> u64;
    #[link_name = "signal_enable"]
    fn c_signal_enable(mask: u64) -> u64;
    #[link_name = "signal_unblock"]
    fn c_signal_unblock(mask: u64) -> u64;
}

/* 与 linux_user_signal.h 的位定义对齐 */
pub const SIGNAL_MASK_SOFTWARE: u64 = 1 << 0;
pub const SIGNAL_MASK_TIMER: u64 = 1 << 1;
pub const SIGNAL_MASK_EXTERNAL: u64 = 1 << 2;
pub const SIGNAL_MASK_ALL: u64 =
    SIGNAL_MASK_SOFTWARE | SIGNAL_MASK_TIMER | SIGNAL_MASK_EXTERNAL;

/// 屏蔽 mask 对应的信号，返回之前的掩码。
#[inline]
pub fn signal_disable(mask: u64) -> u64 {
    unsafe { c_signal_disable(mask) }
}

/// 恢复信号掩码为 mask（SIG_SETMASK），返回之前的掩码。
#[inline]
pub fn signal_enable(mask: u64) -> u64 {
    unsafe { c_signal_enable(mask) }
}

/// 解除 mask 对应信号的屏蔽（SIG_UNBLOCK），返回之前的掩码。
#[inline]
pub fn signal_unblock(mask: u64) -> u64 {
    unsafe { c_signal_unblock(mask) }
}
