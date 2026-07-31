/* 软件定时器。
 * set_timer / 到期驱动不在 00arch/time：
 *   QEMU virt : 注册 timer_tick 到中断分发，底层再用 arch set_timer(SBI)
 *   linux_user: 可用 epoll/timerfd 等宿主机制驱动 timer_tick
 *
 * timer_init 由 _shy_os_init（init.c）在 SHYOS_05 下调用，
 * 必须先于其余 timer 接口。
 */
#ifndef SHYOS_TIMER_H
#define SHYOS_TIMER_H

#include "binary_heap.h"
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

IMPL_SHY_SPINLOCK(Timer)

void timer_init(void);
void timer_tick(void);
void timer_add(u64 time, void (*callback)(void));
/* 删除 time 匹配的第一个成员；找到返回 0，未找到返回 -1 */
i32 timer_remove(u64 time);

#endif /* SHYOS_TIMER_H */
