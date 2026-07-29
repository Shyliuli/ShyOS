# spec-01: 用信号模拟中断，让 timer / time 在 linux_user 正确工作

来源：学员讨论材料整理（2026-07-28）。粒度跟随学员粒度；未表达过的细节不补全。

## 目标

linux_user backend 下，用 Linux 信号模拟硬件中断系统，
使 `00arch/time` 与 `05task/timer` 在 linux_user 下正确工作，
行为与 QEMU virt 下一致。

## 学员已定决策

- 信号即中断系统：linux_user 上用 Linux 信号模拟硬件中断。
- 定时器驱动：`setitimer(ITIMER_REAL)` 产生毫秒级周期信号（SIGALRM），
  比 epoll 简单。
- 信号 handler 调用 `timer_tick`，驱动软件定时器堆到期。
- QEMU virt 侧：注册时钟中断到 trap 分发，固定周期 tick；
  不做 arm_if_earliest 之类的动态 deadline 跟踪（过度设计）。
- 两侧只用一个 `timer_init` 入口，backend 差异用简单的条件编译区分，
  不设 qemu_virt 独立模块。
- `timer_init` 做两件事：构造 STATIC_TIMER（LazySpinlock 空初始化 + 显式
  init，程序员保证顺序，语义与 C 一致）+ 注册中断。
- `timer_init` 按 `SHYOS_05` 条件编译写进 init.c（前向声明，HACK 注释，
  与 trap_entry 同模式）。

## 此处未定义

- 各 IRQ_MASK 位与具体信号的对应关系（仅定了 TIMER ↔ SIGALRM 方向）。
//学员注释: 这里不必和Irq完全一致，提供linux_user_signal的接口，然后timer写#[cfg]来条件编译
- 信号 handler 的安装方式（裸 rt_sigaction 涉及 restorer 汇编，未定义）。
- // 类似trap，我们也实现对应的汇编
- irq 的 linux_user 实现落在哪个文件 / 模块。
- // 新建模块，linux_user_signal 和 irq在同级， linux_user_trap 和trap在同级，接口保持类似但是不必完全相同
- 持 spinlock 期间信号到达的并发问题如何处理（QEMU 侧靠关中断，
  linux_user 侧未定义）。
  // 同样的 signal_disable signal_enabgle
- 周期值：QEMU 侧现状 10ms；linux_user 是否一致未定义。
- //一致
- `time`（init_time / get_time）是否需要配合改动，还是维持现状。
- // 是
- timer.c（C 侧）是否需要承载 linux_user 的信号安装逻辑。
  // 是
## 不在本 spec 范围（留待后续 spec）

- seccomp/SIGSYS 的 syscall 模拟
  （`if call pc from kernel { allow } else { return to kernel }`）。
- uffd 缺页与页表模拟；多页表 / 多进程隔离模型。
- fork 模型（guest 独立进程）下的中断 / syscall 接线。

## 后续修订（学员 2026-07-28 提出，本练习结案后实施）

- **spinlock 自动屏蔽信号**：linux_user 下 `spinlock_lock/unlock` 应对齐
  QEMU 语义——持锁期间自动屏蔽信号（类似 disable_irq/恢复），
  而不是由使用方各自手动 signal_disable/signal_enable。
  实现位置未定（spinlock 内转发 vs irq stub 转发 linux_user_signal）。
- **2026-07-29 已实施（方案 B）**：irq 的 linux_user 实现转发到
  linux_user_signal（disable/enable/set/get/global_irq），spinlock 零改动，
  timer 的手动屏蔽已撤除；`signal_enable` 语义为 SIG_SETMASK 恢复现场。

## 实现期发现（2026-07-29 彩蛋）

- loongarch 内核固定使用 vDSO restorer，忽略 rt_sigaction 的 sa_restorer；
  signal_restorer.S 在 loongarch 上是死代码，x86_64 上才是必需品
  （break 0 实验证实）。
