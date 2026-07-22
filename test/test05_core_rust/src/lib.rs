#![no_std]

use core::ffi::CStr;
use core::panic::PanicInfo;
use shyos_core::string_no_alloc::{CStrExt, MemExt, MemMutExt};

#[unsafe(no_mangle)]
pub extern "C" fn main() -> i32 {
    let text = CStr::from_bytes_with_nul(b"core-rust\0").unwrap();
    let mut buffer = [0u8; 9];

    if text.strlen() != 9 || !text.starts_with_cstr(c"core") {
        return 1;
    }
    if b"abcabc".memmem(b"cab") != Some(2) {
        return 2;
    }
    buffer.memset(0x5a);
    if buffer.memchr(0x5a) != Some(0) {
        return 3;
    }
    buffer.memzero_explicit();
    if buffer.memchr(0) != Some(0) {
        return 4;
    }

    shyos_core::print::early_println!("test05 {}", text.strlen());
    0
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    shyos_core::panic::handle_panic(info)
}
