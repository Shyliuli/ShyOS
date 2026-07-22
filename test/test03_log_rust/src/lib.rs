#![no_std]

use core::panic::PanicInfo;

#[unsafe(no_mangle)]
pub extern "C" fn main() -> i32 {
    shyos_log::print::early_println!("test03 log rust {}", 3);
    shyos_log::stdio::println!("test03 formal stdio");
    if shyos_log::print::early_putchar(b'R' as i32) != b'R' as i32 {
        return 1;
    }
    shyos_log::print::early_putchar(b'\n' as i32);
    0
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    shyos_log::panic::handle_panic(info)
}
