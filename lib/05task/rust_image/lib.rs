//C-only 05task image 的 Rust 闭包与 panic handler。

#![no_std]

pub use shyos_task::*;

#[panic_handler]
fn panic(info: &::core::panic::PanicInfo) -> ! {
    shyos_task::panic::handle_panic(info)
}
