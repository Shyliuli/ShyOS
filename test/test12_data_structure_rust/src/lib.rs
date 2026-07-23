#![no_std]

extern crate alloc;

use alloc::string::String;
use alloc::vec::Vec;
use core::panic::PanicInfo;
use core::sync::atomic::{AtomicUsize, Ordering};
use shyos_data_structure::simple_alloc::HeapBox;
use shyos_data_structure::string::CString;
use shyos_data_structure::string_no_alloc::MemExt;
use shyos_data_structure::{
    error::ERROR_NO_VALUE, hashtable::CHashSet, hashtable::CHashTable, CResult, CVec,
};

static DROP_COUNT: AtomicUsize = AtomicUsize::new(0);

struct DropProbe(u8);

impl Drop for DropProbe {
    fn drop(&mut self) {
        let _ = self.0;
        DROP_COUNT.fetch_add(1, Ordering::SeqCst);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn main() -> i32 {
    let mut block = match HeapBox::alloc(16) {
        Some(block) => block,
        None => return 1,
    };
    block.as_mut_slice().fill(0x7a);
    if block.as_slice().memchr(0x7a) != Some(0) {
        return 2;
    }

    let mut values = Vec::new();
    values.push(7u32);
    values.push(8u32);
    if values.pop() != Some(8) || values[0] != 7 {
        return 3;
    }

    let mut c_values = match CVec::new() {
        CResult::Ok(values) => values,
        CResult::Err(_) => return 4,
    };
    if c_values.push(9u32).is_err() || c_values.push(11u32).is_err() {
        return 5;
    }
    if c_values.insert(1, 10).is_err() || c_values.as_slice() != [9, 10, 11] {
        return 6;
    }
    if c_values.remove(1) != 10 || c_values.pop() != CResult::Ok(11) {
        return 7;
    }
    for value in 12..=16 {
        if c_values.push(value).is_err() {
            return 8;
        }
    }
    if c_values.capacity() <= 5 || c_values.last() != Some(&16) {
        return 9;
    }
    c_values.retain(|value| value % 2 == 0);
    if c_values.as_slice() != [12, 14, 16] {
        return 10;
    }

    let from_slice = match CVec::try_from(&[21u32, 22, 23][..]) {
        Ok(values) => values,
        Err(_) => return 11,
    };
    if from_slice.as_slice() != [21, 22, 23] {
        return 12;
    }

    let mut table = match CHashTable::<u32, String>::with_capacity(8) {
        CResult::Ok(table) => table,
        CResult::Err(_) => return 13,
    };
    if table.insert(1, String::from("one")).is_err()
        || table.insert(9, String::from("nine")).is_err()
        || table.insert(17, String::from("seventeen")).is_err()
    {
        return 14;
    }
    if table.len() != 3
        || table.get(&17).map(String::as_str) != Some("seventeen")
    {
        return 15;
    }
    if let Some(value) = table.get_mut(&1) {
        value.push_str("-updated");
    } else {
        return 16;
    }
    if table.get(&1).map(String::as_str) != Some("one-updated") {
        return 17;
    }
    if table.remove(&9) != CResult::Ok(String::from("nine")) {
        return 18;
    }
    if table.contains_key(&17) != true || table.contains_key(&9) {
        return 19;
    }
    table.clear();
    if !table.is_empty() {
        return 20;
    }
    let initial_capacity = table.capacity();
    for key in 0..12 {
        if table.insert(key, String::from("value")).is_err() {
            return 21;
        }
    }
    if table.capacity() <= initial_capacity
        || (0..12).any(|key| table.get(&key).map(String::as_str) != Some("value"))
    {
        return 22;
    }

    let mut set = match CHashSet::<u32>::with_capacity(8) {
        CResult::Ok(set) => set,
        CResult::Err(_) => return 23,
    };
    if set.insert(7) != CResult::Ok(true)
        || set.insert(7) != CResult::Ok(false)
        || !set.contains(&7)
        || set.len() != 1
    {
        return 24;
    }
    if !set.remove(&7) || set.contains(&7) || set.remove(&7) {
        return 25;
    }
    set.clear();
    if !set.is_empty() {
        return 26;
    }

    let mut reserved = match CVec::with_capacity(3) {
        CResult::Ok(values) => values,
        CResult::Err(_) => return 13,
    };
    if reserved.capacity() < 3
        || reserved.extend_from_slice(&[24u32, 25, 26]).is_err()
        || reserved.as_slice() != [24, 25, 26]
    {
        return 14;
    }

    let raw_source = match CVec::try_from(&[27u32, 28, 29][..]) {
        Ok(values) => values,
        Err(_) => return 15,
    };
    let (data, len, cap) = raw_source.into_raw_parts();
    let raw_roundtrip = unsafe { CVec::from_raw_parts(data, len, cap) };
    if raw_roundtrip.as_slice() != [27, 28, 29] {
        return 16;
    }
    drop(raw_roundtrip);

    let mut strings = match CVec::new() {
        CResult::Ok(values) => values,
        CResult::Err(_) => return 11,
    };
    if strings.push(String::from("shy")).is_err() || strings.push(String::from("os")).is_err() {
        return 18;
    }
    if strings.remove(0).as_str() != "shy" {
        return 19;
    }
    strings.clear();

    {
        let mut probes = match CVec::new() {
            CResult::Ok(values) => values,
            CResult::Err(_) => return 20,
        };
        if probes.push(DropProbe(1)).is_err() || probes.push(DropProbe(2)).is_err() {
            return 21;
        }
        match probes.pop() {
            CResult::Ok(probe) => drop(probe),
            CResult::Err(_) => return 22,
        }
    }
    if DROP_COUNT.load(Ordering::SeqCst) != 2 {
        return 22;
    }

    {
        let mut probes = match CVec::new() {
            CResult::Ok(values) => values,
            CResult::Err(_) => return 23,
        };
        if probes.push(DropProbe(3)).is_err() || probes.push(DropProbe(4)).is_err() {
            return 24;
        }
        probes.clear();
    }
    if DROP_COUNT.load(Ordering::SeqCst) != 4 {
        return 25;
    }

    let mut owned = match CString::with_capacity(8) {
        CResult::Ok(value) => value,
        CResult::Err(_) => return 26,
    };
    if owned.push_str(c"shy").is_err()
        || owned.push_str(c"os").is_err()
        || owned.as_c_str() != c"shyos"
    {
        return 34;
    }
    if owned.insert(3, b'-').is_err() || owned.as_c_str() != c"shy-os" {
        return 37;
    }
    if owned.remove(3) != b'-' || owned.as_c_str() != c"shyos" {
        return 38;
    }
    if owned.insert_str(3, c"-").is_err() || owned.as_c_str() != c"shy-os" {
        return 39;
    }
    if owned.remove(3) != b'-' || owned.as_c_str() != c"shyos" {
        return 40;
    }
    let cloned = owned.clone();
    if cloned.as_c_str() != c"shyos" || cloned.as_ptr() == owned.as_ptr() {
        return 35;
    }

    let mut edited = match CString::from_c_str(c"abc") {
        CResult::Ok(value) => value,
        CResult::Err(_) => return 41,
    };
    if edited.insert_str(1, c"XY").is_err() || edited.as_c_str() != c"aXYbc" {
        return 42;
    }
    if edited.remove(1) != b'X' || edited.as_c_str() != c"aYbc" {
        return 44;
    }
    let mut tail = match edited.split_off(2) {
        CResult::Ok(value) => value,
        CResult::Err(_) => return 45,
    };
    if edited.as_c_str() != c"aY" || tail.as_c_str() != c"bc" {
        return 46;
    }
    if tail.push_str(c"--").is_err() {
        return 47;
    }
    tail.retain(|byte| byte != b'-');
    if tail.as_c_str() != c"bc" {
        return 48;
    }
    tail.truncate(1);
    if tail.pop() != CResult::Ok(b'b') || !tail.is_empty() {
        return 49;
    }

    let mut result_strings = match CVec::new() {
        CResult::Ok(values) => values,
        CResult::Err(_) => return 50,
    };
    let present = CString::from_c_str(c"nested");
    if result_strings.push(present).is_err()
        || result_strings.push(CResult::<CString>::none()).is_err()
    {
        return 51;
    }
    match &result_strings[0] {
        CResult::Ok(value) if value.as_c_str() == c"nested" => {}
        _ => return 52,
    }
    match &result_strings[1] {
        CResult::Err(ERROR_NO_VALUE) => {}
        _ => return 53,
    }
    let cloned_results = result_strings.clone();
    match (&result_strings[0], &cloned_results[0]) {
        (CResult::Ok(source), CResult::Ok(clone))
            if source.as_c_str() == clone.as_c_str() && source.as_ptr() != clone.as_ptr() => {}
        _ => return 54,
    }
    match &cloned_results[1] {
        CResult::Err(ERROR_NO_VALUE) => {}
        _ => return 55,
    }

    shyos_data_structure::print::early_println!("test12 data structure rust");
    0
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    shyos_data_structure::panic::handle_panic(info)
}
