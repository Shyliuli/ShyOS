#![no_std]

use core::panic::PanicInfo;
use shyos_arch::{board, irq, rand, shutdown, uart};

#[unsafe(no_mangle)]
pub extern "C" fn main() -> i32 {
    if !board::HEAP_START.is_null() || board::HEAP_SIZE == 0 {
        return 1;
    }

    uart::raw_putc(b'1');
    uart::raw_putc(b'\n');

    if unsafe { irq::set_trap_entry(0) } != Err(irq::IrqError::Unsupported) {
        return 2;
    }
    if irq::get_irq() != 0 {
        return 3;
    }
    if irq::claim_external_irq() != Ok(None) {
        return 4;
    }

    rand::seed(1);
    let first = rand::random();
    rand::seed(1);
    if rand::random() != first {
        return 5;
    }
    for _ in 0..32 {
        let value = rand::randint(3, 7);
        if !(3..=7).contains(&value) {
            return 6;
        }
    }

    0
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    shutdown::shutdown(-1)
}
