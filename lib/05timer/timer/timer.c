#include "timer.h"

#if defined(SHYOS_BACKEND_LINUX_USER)
/* linux_user：setitimer 产生毫秒级周期 SIGALRM，handler 调 timer_tick。
 * 与 QEMU virt 的固定周期 tick 对齐，周期同为 10ms。
 */
#include "board.h"
#include "linux_user_signal.h"
#include "linux_user_trap.h"
#include "time.h"

#define TIMER_TICK_MS 10
#define SIGALRM_NR 14
#define ITIMER_REAL 0

struct shy_timeval {
	i64 tv_sec;
	i64 tv_usec;
};

struct shy_itimerval {
	struct shy_timeval it_interval;
	struct shy_timeval it_value;
};

static void timer_sigalrm_handler(i32 signo)
{
	(void)signo;
	timer_tick();
}

void timer_linux_user_init(void)
{
	struct shy_itimerval it = {0};

	signal_trap_register(SIGALRM_NR, timer_sigalrm_handler);
	signal_trap_init();
	// 第一次的时间
	it.it_value.tv_usec = TIMER_TICK_MS * 1000;
	// 周期时间！
	it.it_interval.tv_usec=  TIMER_TICK_MS * 1000;
	(void)linux_syscall(__NR_setitimer, ITIMER_REAL, (long)&it, 0, 0, 0, 0);
}
#endif
