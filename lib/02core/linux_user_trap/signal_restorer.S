/* linux_user：rt_sigaction 的 restorer trampoline。
note:对于loongarch，riscv等新架构，内核固定使用vDSO 提供的 restorer
所以loongarch分支是死代码
 */
#if defined(__loongarch64)
	.text
	.globl	shyos_signal_restorer
	.type	shyos_signal_restorer,@function
shyos_signal_restorer:
	li.d	$a7, 139	/* __NR_rt_sigreturn */
	syscall	0
#else
#error "signal restorer not implemented for this architecture"
#endif
