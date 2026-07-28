#![no_std]
use shyos_binary_heap::CBinaryHeap;
/// TimerItem 外部构造
#[repr(C)]
pub struct TimerItem{
    /*开机后的毫秒*/
    pub time:u64,
    /*callback函数,Option是为了处理和C交互时可能传入的NULL*/
    pub callback: Option<unsafe extern "C" fn()>,
}
impl TimerItem {
    pub fn new(time: u64, callback: unsafe extern "C" fn()) -> Self {
        TimerItem { time, Some(callback) }
    }
}
impl Ord for TimerItem {
    fn cmp(&self, other: &Self) -> Ordering {
        self.time.cmp(&other.time)
    }
}
#[repr(transparent)]
struct Timer{
    timer_queue:CBinaryHeap<TimerItem>,
}

impl Timer {
    pub fn new() -> Self {
        Timer { timer_queue: CBinaryHeap::new(HeapOrder::Min) }
    }
    pub fn tick(&mut self){
        let now = shyos_time::get_time();
        while let Some(timer_item) = self.timer_queue.peek() {
            if timer_item.time <= now {
                match timer_item.callback {
                    Some(callback) => callback(),
                    None => (),
                }
                self.timer_queue.pop();
            } else {
                break;
            }
        }
    }
    pub fn add(&mut self, timer_item: TimerItem){
        self.timer_queue.push(timer_item);
    }
    pub fn remove(&mut self, time:u64){
        for item in &mut self.timer_queue{
            if item.time == time{
                self.timer_queue.remove(item);
                break;
            }
        }
    }
}
static STATIC_TIMER: Spinlock<Timer> = Spinlock::new(Timer::new());
unsafe extern "C" fn timer_init(){
   STATIC_TIMER.lock().init();
}
unsafe extern "C" fn timer_tick(t:*mut Timer){
    STATIC_TIMER.lock().tick();
}
unsafe extern "C" fn timer_add(t:*mut Timer, time:u64, callback:unsafe extern "C" fn()){
    STATIC_TIMER.lock().add(TimerItem::new(time, callback));
}
unsafe extern "C" fn timer_remove(t:*mut Timer,time:u64){
    STATIC_TIMER.lock().remove(time);
}
