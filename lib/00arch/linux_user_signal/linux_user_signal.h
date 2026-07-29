/* linux_user：用信号掩码模拟 CPU 中断屏蔽。
 * 与 00arch/irq 同级，接口类似但不必相同；
 * irq 的 linux_user stub 维持无硬件语义，需要屏蔽语义的模块
 * （如 05task/timer）改用本接口。
 */
#ifndef SHYOS_LINUX_USER_SIGNAL_H
#define SHYOS_LINUX_USER_SIGNAL_H

#include "shy_type.h"

/* 位序与 irq.h 的 IRQ_MASK_* 对齐，便于上层共用语义 */
#define SIGNAL_MASK_SOFTWARE (1ULL << 0)
#define SIGNAL_MASK_TIMER    (1ULL << 1) /* SIGALRM */
#define SIGNAL_MASK_EXTERNAL (1ULL << 2)
#define SIGNAL_MASK_ALL \
	(SIGNAL_MASK_SOFTWARE | SIGNAL_MASK_TIMER | SIGNAL_MASK_EXTERNAL)

/* 屏蔽 mask 对应的信号，返回之前的掩码（保存现场） */
u64 signal_disable(u64 mask);
/* 恢复信号掩码为 mask（SIG_SETMASK），返回之前的掩码；
 * 与 signal_disable 配对做恢复现场 */
u64 signal_enable(u64 mask);
/* 解除 mask 对应信号的屏蔽（SIG_UNBLOCK），返回之前的掩码 */
u64 signal_unblock(u64 mask);

#endif /* SHYOS_LINUX_USER_SIGNAL_H */
