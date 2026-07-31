//C-only 0timerimage 的 Rust 闭包与 panic handler。

#![no_std]

pub use shyos_05timer::*;

#[panic_handler]
fn panic(info: &::core::panic::PanicInfo) -> ! {
    shyos_05timer::panic::handle_panic(info)
}
