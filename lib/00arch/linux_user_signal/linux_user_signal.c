#include "linux_user_signal.h"

#include "board.h"

#if !defined(SHYOS_BACKEND_LINUX_USER)
#error "linux_user_signal.c requires SHYOS_BACKEND_LINUX_USER"
#endif

#define SIG_BLOCK_OP 0
#define SIG_UNBLOCK_OP 1
#define SETMASK_OP 2
#define SIGALRM_NR 14


typedef struct {
	u64 word;
} shy_sigset_t;

static void mask_to_sigset(u64 mask, shy_sigset_t *set)
{


	set->word=0;
	if ((mask & SIGNAL_MASK_TIMER) != 0) {
		set->word |= 1ULL << (SIGALRM_NR - 1);
	}
}

static u64 sigset_to_mask(const shy_sigset_t *set)
{
	u64 mask = 0;

	if ((set->word & (1ULL << (SIGALRM_NR - 1))) != 0) {
		mask |= SIGNAL_MASK_TIMER;
	}
	return mask;
}

static u64 signal_procmask(long op, u64 mask)
{
	shy_sigset_t set;
	shy_sigset_t old;

	mask_to_sigset(mask, &set);
	mask_to_sigset(0, &old);
	/* rt_sigprocmask 的 sigsetsize 必须是内核 sigset_t 的大小 */
	(void)linux_syscall(__NR_rt_sigprocmask, op, (long)&set,
		(long)&old, sizeof(shy_sigset_t), 0, 0);
	return sigset_to_mask(&old);
}

u64 signal_disable(u64 mask)
{
	return signal_procmask(SIG_BLOCK_OP, mask);
}

u64 signal_enable(u64 mask)
{
	return signal_procmask(SETMASK_OP, mask);
}

u64 signal_unblock(u64 mask)
{
	return signal_procmask(SIG_UNBLOCK_OP, mask);
}
