#include "linux_user_trap.h"

#include "board.h"

#if !defined(SHYOS_BACKEND_LINUX_USER)
#error "linux_user_trap.c requires SHYOS_BACKEND_LINUX_USER"
#endif

#define SIGNAL_TRAP_MAX 64

/* asm-generic 的 SA_RESTORER */
#define SA_RESTORER_FLAG 0x04000000ULL

/* 内核 rt_sigaction 布局（asm-generic） */
struct k_sigaction {
	void (*handler)(int);
	u64 flags;
	void (*restorer)(void);
	u64 mask;
};

/* signal_restorer.S 提供：handler 返回后的 rt_sigreturn trampoline */
extern void shyos_signal_restorer(void);

static signal_trap_handler_t signal_trap_table[SIGNAL_TRAP_MAX];

/* 所有信号共用的入口，按 signo 查表分发 */
static void signal_trap_entry(int signo)
{
	signal_trap_handler_t handler;

	if (signo <= 0 || signo >= SIGNAL_TRAP_MAX) {
		return;
	}
	handler = signal_trap_table[signo];
	if (handler != 0) {
		handler(signo);
	}
}

void signal_trap_init(void)
{
	/*DO NOTHING!*/
	/*signal的初始化由signal_trap_register完成*/
}

i32 signal_trap_register(i32 signo, signal_trap_handler_t handler)
{
	struct k_sigaction act;

	if (signo <= 0 || signo >= SIGNAL_TRAP_MAX || handler == 0) {
		return -1;
	}
	if (signal_trap_table[signo] != 0) {
		return -1;
	}
	signal_trap_table[signo] = handler;
	act.handler = signal_trap_entry;
	act.flags = SA_RESTORER_FLAG;
	act.restorer = shyos_signal_restorer;
	act.mask = 0;
	if (linux_syscall(__NR_rt_sigaction, signo, (long)&act, 0, 8, 0, 0)
	    != 0) {
		return -1;
	}
	return 0;
}
