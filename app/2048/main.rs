#![no_std]

use core::panic::PanicInfo;
use shyos_basic::arch::rand::randint;
use shyos_basic::result::*;
use shyos_data_structure::CVec;

#[repr(C)]
pub struct State {
    //当然 其实最节省开销的是固定数组
    //这只是为了测试我们的库(和ffi)
    //在ffi中传递
    pub num: CVec<CResult<i32>>,
}

impl State {
    pub fn init() -> CResult<Self> {
        let mut c: CVec<CResult<i32>> = c_try!(CVec::with_capacity(16));
        for _ in 0..16 {
            c_try!(c.push(CResult::none()));
        }
        Ok(Self { num: c })
    }

    fn index_mut(&mut self, i: usize, j: usize) -> &mut CResult<i32> {
        //01 02 03 04  i
        //05 06 07 08
        //09 10 11 12
        //13 14 15 16
        //
        //j
        &mut self.num[i + 4 * j]
    }

    fn index(&self, i: usize, j: usize) -> &CResult<i32> {
        //01 02 03 04  i
        //05 06 07 08
        //09 10 11 12
        //13 14 15 16
        //
        //j
        &self.num[i + 4 * j]
    }

    fn up(&mut self) {
        for i in 0..4 {
            let mut merged = 4;
            for j in 1..4 {
                let value = match self.index(i, j) {
                    Ok(value) => *value,
                    Err(_) => continue,
                };
                let mut highest = j;
                while highest > 0 && self.index(i, highest - 1).is_none() {
                    highest -= 1;
                }
                if highest > 0 && highest - 1 != merged && *self.index(i, highest - 1) == Ok(value)
                {
                    self.merge_to(i, j, i, highest - 1);
                    merged = highest - 1;
                } else if highest != j {
                    self.move_to(i, j, i, highest);
                }
            }
        }
    }

    fn down(&mut self) {
        for i in 0..4 {
            let mut merged = 4;
            for j in (0..3).rev() {
                let value = match self.index(i, j) {
                    Ok(value) => *value,
                    Err(_) => continue,
                };
                let mut lowest = j;
                while lowest < 3 && self.index(i, lowest + 1).is_none() {
                    lowest += 1;
                }
                if lowest < 3 && lowest + 1 != merged && *self.index(i, lowest + 1) == Ok(value) {
                    self.merge_to(i, j, i, lowest + 1);
                    merged = lowest + 1;
                } else if lowest != j {
                    self.move_to(i, j, i, lowest);
                }
            }
        }
    }

    fn left(&mut self) {
        for j in 0..4 {
            let mut merged = 4;
            for i in 1..4 {
                let value = match self.index(i, j) {
                    Ok(value) => *value,
                    Err(_) => continue,
                };
                let mut leftmost = i;
                while leftmost > 0 && self.index(leftmost - 1, j).is_none() {
                    leftmost -= 1;
                }
                if leftmost > 0
                    && leftmost - 1 != merged
                    && *self.index(leftmost - 1, j) == Ok(value)
                {
                    self.merge_to(i, j, leftmost - 1, j);
                    merged = leftmost - 1;
                } else if leftmost != i {
                    self.move_to(i, j, leftmost, j);
                }
            }
        }
    }

    fn right(&mut self) {
        for j in 0..4 {
            let mut merged = 4;
            for i in (0..3).rev() {
                let value = match self.index(i, j) {
                    Ok(value) => *value,
                    Err(_) => continue,
                };
                let mut rightmost = i;
                while rightmost < 3 && self.index(rightmost + 1, j).is_none() {
                    rightmost += 1;
                }
                if rightmost < 3
                    && rightmost + 1 != merged
                    && *self.index(rightmost + 1, j) == Ok(value)
                {
                    self.merge_to(i, j, rightmost + 1, j);
                    merged = rightmost + 1;
                } else if rightmost != i {
                    self.move_to(i, j, rightmost, j);
                }
            }
        }
    }

    fn move_to(&mut self, i_1: usize, j_1: usize, i_2: usize, j_2: usize) {
        let value = self.index_mut(i_1, j_1).take();
        *self.index_mut(i_2, j_2) = value;
    }

    fn merge_to(&mut self, i_1: usize, j_1: usize, i_2: usize, j_2: usize) {
        let value = match self.index_mut(i_1, j_1).take() {
            Ok(value) => value,
            Err(_) => return,
        };
        if let Ok(target) = self.index_mut(i_2, j_2) {
            *target += value;
        }
    }

    fn add_random(&mut self) {
        let empty = self.num.iter().filter(|value| value.is_none()).count();
        if empty == 0 {
            return;
        }

        let selected = randint(0, empty as i32 - 1) as usize;
        let mut current = 0;
        let mut index = 0;
        while index < self.num.len() {
            if self.num[index].is_none() {
                if current == selected {
                    break;
                }
                current += 1;
            }
            index += 1;
        }

        let random = randint(0, 511);
        let mut value = 2;
        let mut mask = 256;
        while random & mask != 0 {
            value *= 2;
            mask >>= 1;
        }
        self.num[index] = Ok(value);
    }
}

#[repr(u32)]
pub enum Key {
    Up = 1,
    Down = 2,
    Left = 3,
    Right = 4,
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn update(state: *mut State, key: Key) {
    let state = unsafe { &mut *state };
    match key {
        Key::Up => state.up(),
        Key::Down => state.down(),
        Key::Left => state.left(),
        Key::Right => state.right(),
    }
    state.add_random();
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn index(state: *mut State, i: usize, j: usize) -> *mut CResult<i32> {
    unsafe { (&mut *state).index_mut(i, j) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn init() -> CResult<State> {
    State::init()
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    shyos_data_structure::panic::handle_panic(info)
}
