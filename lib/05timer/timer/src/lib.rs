/* 软件定时器。
 * STATIC_TIMER 由 LazySpinlock 空初始化，timer_init 显式构造；
 * 到期驱动按 backend 条件编译，两侧同为固定 10ms 周期：
 *   QEMU virt : 时钟中断注册到 trap 分发，set_timer 周期武装
 *   linux_user: setitimer 周期 SIGALRM（见 timer.c），handler 调 timer_tick
 */
#![no_std]

use core::cmp::Ordering;

use shyos_binary_heap::*;
use shyos_spinlock::*;

/// TimerItem 外部构造
#[repr(C)]
pub struct TimerItem {
    /*开机后的毫秒*/
    pub time: u64,
    /*callback函数,Option是为了处理和C交互时可能传入的NULL*/
    pub callback: Option<unsafe extern "C" fn()>,
}

impl TimerItem {
    pub fn new(time: u64, callback: unsafe extern "C" fn()) -> Self {
        TimerItem {
            time,
            callback: Some(callback),
        }
    }
}

/* timer_remove 按 time 匹配第一个成员，eq 只比较 time */
impl PartialEq for TimerItem {
    fn eq(&self, other: &Self) -> bool {
        self.time == other.time
    }
}
impl Eq for TimerItem {}

impl PartialOrd for TimerItem {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}
impl Ord for TimerItem {
    fn cmp(&self, other: &Self) -> Ordering {
        self.time.cmp(&other.time)
    }
}

#[repr(transparent)]
struct Timer {
    timer_queue: CBinaryHeap<TimerItem>,
}

impl Timer {
    pub fn new() -> Self {
        Timer {
            timer_queue: match CBinaryHeap::new(HeapOrder::Min) {
                Ok(t) => t,
                Err(e) => {
                    panic!("Failed to create timer queue: Error Code{}", e);
                }
            },
        }
    }
    pub fn tick(&mut self) {
        let now = shyos_time::get_time();
        while let Some(timer_item) = self.timer_queue.peek() {
            if timer_item.time <= now {
                match timer_item.callback {
                    Some(callback) => unsafe { callback() },
                    None => (),
                }
                self.timer_queue.pop();
            } else {
                break;
            }
        }
    }
    pub fn add(&mut self, timer_item: TimerItem) {
        self.timer_queue.push(timer_item);
    }
    pub fn remove(&mut self, time: u64) -> CResult<()> {
        self.timer_queue.remove(TimerItem {
            time,
            callback: None,
        })
    }
}

static STATIC_TIMER: LazySpinlock<Timer> = LazySpinlock::uninit();

const TICK_MS: u64 = 10;

#[cfg(SHYOS_BACKEND_QEMU_VIRT)]
use shyos_trap::irq::{IRQ_MASK_TIMER, enable_irq};
#[cfg(SHYOS_BACKEND_QEMU_VIRT)]
use shyos_trap::{TRAP_IRQ_CAUSE_TIMER, TrapContext, trap_set_interrupt_handler};

#[cfg(SHYOS_BACKEND_QEMU_VIRT)]
unsafe extern "C" {
    #[link_name = "set_timer"]
    fn c_set_timer(ms: u64);
}

#[cfg(SHYOS_BACKEND_LINUX_USER)]
unsafe extern "C" {
    fn timer_linux_user_init();
}

#[cfg(SHYOS_BACKEND_QEMU_VIRT)]
extern "C" fn timer_trap_handler(_ctx: *mut TrapContext) {
    unsafe { c_set_timer(TICK_MS) };
    timer_tick();
}

/// 构造 STATIC_TIMER 并按 backend 注册中断。
/// 由 `_shy_os_init`（init.c）在 SHYOS_05 下调用，必须先于其余 timer 接口。
#[unsafe(no_mangle)]
pub extern "C" fn timer_init() {
    STATIC_TIMER.init(Timer::new());
    #[cfg(SHYOS_BACKEND_QEMU_VIRT)]
    {
        trap_set_interrupt_handler(TRAP_IRQ_CAUSE_TIMER, Some(timer_trap_handler));
        unsafe { c_set_timer(TICK_MS) };
        enable_irq(IRQ_MASK_TIMER);
    }
    #[cfg(SHYOS_BACKEND_LINUX_USER)]
    unsafe {
        timer_linux_user_init()
    };
}

/// 执行全部到期回调；QEMU 由时钟中断调用，linux_user 由 SIGALRM 调用。
#[unsafe(no_mangle)]
pub extern "C" fn timer_tick() {
    unsafe { STATIC_TIMER.lock() }.tick();
}

#[unsafe(no_mangle)]
pub extern "C" fn timer_add(time: u64, callback: unsafe extern "C" fn()) {
    let item = TimerItem::new(time, callback);
    unsafe { STATIC_TIMER.lock() }.add(item);
}

/// 删除 time 匹配的第一个成员；找到返回 0，未找到返回 -1。
#[unsafe(no_mangle)]
pub extern "C" fn timer_remove(time: u64) -> i32 {
    match unsafe { STATIC_TIMER.lock() }.remove(time) {
        CResult::Ok(()) => 0,
        CResult::Err(_) => -1,
    }
}
