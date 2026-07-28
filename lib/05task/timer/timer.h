/* 软件定时器。
 * set_timer / 到期驱动不在 00arch/time：
 *   QEMU virt : 注册 timer_tick 到中断分发，底层再用 arch set_timer(SBI)
 *   linux_user: 可用 epoll/timerfd 等宿主机制驱动 timer_tick
 */
#ifndef SHYOS_TIMER_H
#define SHYOS_TIMER_H

#include "obj.h"
#include "shy_type.h"
#include "spinlock.h"

typedef struct Timer Timer;
typedef struct TimerItem TimerItem;

struct Timer {
    BinaryHeap timer_queue;
};

struct TimerItem {
    u64 time;
    void (*callback)(void);
};

#IMPL_SHY_SPINLOCK(Timer)
#endif /* SHYOS_TIMER_H */
