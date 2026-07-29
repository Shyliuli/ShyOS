/* timer_demo：linux_user 下软件定时器的功能验证。
 * 阶段一：定时顺序、remove、批量到期；
 * 阶段二：持续 add/remove 压测到 400ms。
 */
#include "early_stdio.h"
#include "time.h"
#include "timer.h"

#define HAMMER_N 2000

static volatile int c_a;
static volatile int c_b;
static volatile int c_c;
static volatile int c_removed;
static volatile int c_hammer;

static void cb_a(void) { c_a++; }
static void cb_b(void) { c_b++; }
static void cb_c(void) { c_c++; }
static void cb_removed(void) { c_removed++; }
static void cb_hammer(void) { c_hammer++; }

int main(void)
{
	u64 start = get_time();
	u64 now;
	int i;

	early_printf("timer demo start\n");
	/*test:实际运行到这里还是0ms*/
	/*问题:cb_a没有被运行到!*/
	timer_add(start + 30, cb_a);
	timer_add(start + 10, cb_b);
	timer_add(start + 20, cb_c);
	timer_add(start + 15, cb_removed);
	if (timer_remove(start + 15) != 0) {
		early_printf("FAIL: remove returned nonzero\n");
		return 1;
	}

	for (i = 0; i < HAMMER_N; ++i) {
		timer_add(start + 100 + (u64)(i % 40), cb_hammer);
	}

	while (get_time() - start < 200) {
	}

	if (c_a != 1 || c_b != 1 || c_c != 1 || c_removed != 0) {
		early_printf("FAIL: singles a=%d b=%d c=%d removed=%d\n",
			c_a, c_b, c_c, c_removed);
		return 1;
	}
	/* 单次 3 个 + 批量 2000 个 */
	if (c_a + c_b + c_c + c_hammer != 2003) {
		early_printf("FAIL: count total=%d\n",
			c_a + c_b + c_c + c_hammer);
		return 1;
	}
	early_printf("phase1 ok\n");

	while ((now = get_time()) - start < 400) {
		timer_add(now + 20, cb_hammer);
		(void)timer_remove(now + 20);
	}
	early_printf("PASS\n");
	return 0;
}
