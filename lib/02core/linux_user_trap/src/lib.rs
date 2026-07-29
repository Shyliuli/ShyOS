/* linux_user_trap 的 Rust 包装：信号驱动的 trap 分发。
 * 仅 linux_user backend 提供实现，其余 backend 为空 crate。
 */
#![no_std]
#![cfg(SHYOS_BACKEND_LINUX_USER)]

/// 对应 C 的 `signal_trap_handler_t`。
pub type SignalTrapHandler = unsafe extern "C" fn(i32);

unsafe extern "C" {
    #[link_name = "signal_trap_init"]
    fn c_signal_trap_init();
    #[link_name = "signal_trap_register"]
    fn c_signal_trap_register(signo: i32, handler: SignalTrapHandler) -> i32;
}

/// 清空分发表。
pub fn signal_trap_init() {
    unsafe { c_signal_trap_init() }
}

/// 注册 signo 的 handler 并安装信号动作；失败返回 -1。
pub fn signal_trap_register(signo: i32, handler: SignalTrapHandler) -> i32 {
    unsafe { c_signal_trap_register(signo, handler) }
}
