/* linux_user：信号驱动的 trap 分发。
 * 与 02core/trap 同级，接口类似但不必相同；
 * 表驱动：signo -> handler，restorer 汇编负责从 handler 返回。
 */
#ifndef SHYOS_LINUX_USER_TRAP_H
#define SHYOS_LINUX_USER_TRAP_H

#include "shy_type.h"

typedef void (*signal_trap_handler_t)(i32 signo);

/* 清空分发表 */
void signal_trap_init(void);
/* 注册 signo 的 handler 并安装信号动作；参数非法或重复注册返回 -1 */
i32 signal_trap_register(i32 signo, signal_trap_handler_t handler);

#endif /* SHYOS_LINUX_USER_TRAP_H */
