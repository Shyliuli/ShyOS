#![no_std]

use core::panic::PanicInfo;
use core::sync::atomic::{AtomicUsize, Ordering};
use shyos_basic::simple_alloc::HeapBox;
use shyos_basic::{error::ERROR_NO_VALUE, CResult};

static DROP_COUNT: AtomicUsize = AtomicUsize::new(0);

struct DropProbe;

impl Drop for DropProbe {
    fn drop(&mut self) {
        DROP_COUNT.fetch_add(1, Ordering::SeqCst);
    }
}

fn add_one(value: CResult<u32>) -> CResult<u32> {
    let value = shyos_basic::c_try!(value);
    CResult::ok(value + 1)
}

#[unsafe(no_mangle)]
pub extern "C" fn main() -> i32 {
    let mut value = CResult::ok(DropProbe);
    if value.is_err() {
        return 1;
    }
    match value.take() {
        CResult::Ok(probe) => drop(probe),
        CResult::Err(_) => return 2,
    }
    if !value.is_none() || DROP_COUNT.load(Ordering::SeqCst) != 1 {
        return 2;
    }

    let mut value = CResult::ok(DropProbe);
    match value.replace(DropProbe) {
        CResult::Ok(probe) => drop(probe),
        CResult::Err(_) => return 3,
    }
    if DROP_COUNT.load(Ordering::SeqCst) != 2 {
        return 3;
    }
    drop(value);
    if DROP_COUNT.load(Ordering::SeqCst) != 3 {
        return 4;
    }

    if CResult::none().unwrap_or(7u32) != 7 {
        return 5;
    }
    match CResult::<u32>::none() {
        CResult::Ok(_) => return 6,
        CResult::Err(ERROR_NO_VALUE) => {}
        CResult::Err(_) => return 6,
    }
    if add_one(CResult::ok(6)) != CResult::ok(7) || add_one(CResult::none()) != CResult::none() {
        return 7;
    }

    let mut block = match HeapBox::alloc(16) {
        Some(block) => block,
        None => return 8,
    };
    block.as_mut_slice().fill(0x7a);
    if block.as_slice()[15] != 0x7a {
        return 9;
    }

    shyos_basic::print::early_println!("test07 basic result rust");
    0
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    shyos_basic::panic::handle_panic(info)
}
