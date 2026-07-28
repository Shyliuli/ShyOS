#![no_std]

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
#[repr(transparent)]
struct Timer{
    //最小堆，需要去实现！
    //好吧 可能应该选择红黑树?考虑到要删除
    timer_queue:CMinHeap<TimerItem>,
}

