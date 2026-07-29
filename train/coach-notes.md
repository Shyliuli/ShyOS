# 教练私密记录（学员勿读 · 供定向错误注入使用）

## 2026-07-28 spec-01 整理时怀疑的学员误解 / 未检验区

1. 曾把 uffd 的 monitor 语境理解为"必须借用 fork"——线程 / 进程 /
   缺页服务上下文三者的关系可能模糊（已当场澄清，理解程度未验证）。
2. 对"持锁期间信号到达"的并发问题尚未建立模型：QEMU 侧靠关中断获得
   的隐含安全前提，迁移到信号语境时可能直接平移假设。
3. 认为 setitimer+SIGALRM "简单"——可能未意识到裸 rt_sigaction 需要
   restorer 汇编这一成本项（已被提醒，是否吸收未知）。
4. spec 中 IRQ_MASK ↔ 信号的映射留了空白，学员对 enable/disable/set_irq
   的返回值语义（旧掩码）可能未注意。

## 2026-07-28 spec-01 实现：注入清单（N=6，零注入=否）

参数：S≈450 E=5 A=0.5 T=是 | λ≈8（钳制前）| 取样 N=6

| # | 位置 | 类别 | 内容 | 预期症状 |
|---|------|------|------|----------|
| 1 | lib/05task/timer/timer.c | 算法/逻辑 | signal_trap_init 与 register 顺序颠倒，表被清空 | 什么都不触发 a=0 b=0 c=0 |
| 2 | lib/05task/timer/timer.c | 赋值/遗漏 | setitimer 只设 it_value，it_interval 为 0 | 单次触发，b=1 后无下文 |
| 3 | app/timer_demo/main.c | 测试代码/边界 | 期望总数 2004（实际 2003） | FAIL: count total=2003 |
| 4 | lib/05task/timer/src/lib.rs | 定向（误解#2） | timer_add 无信号屏蔽（remove 有） | phase2 死锁 |
| 5 | lib/00arch/linux_user_signal/linux_user_signal.c | 接口/内核ABI | sigsetsize 传 128（内核要求 8）→ EINVAL 静默 | 修 #4 后仍死锁；strace 可见 |
| 6 | lib/05task/timer/src/lib.rs | 接口误用/定向（误解#4） | timer_remove 用 signal_enable(saved) 当 set_irq 用 | 修 #5 后回归：全部不触发 |

## 级联验证记录（已逐步临时修复-验证-还原）

1. 原始 → FAIL singles a=0 b=0 c=0（bug1）
2. 修 1 → FAIL singles a=0 b=1 c=0（bug2）
3. 修 2 → FAIL count total=2003（bug3）
4. 修 3 → phase1 ok 后 phase2 挂死（bug4+5）
5. 修 4 → 仍挂死（bug5）
6. 修 5 → 回归 FAIL singles a=0 b=0 c=0（bug6）
7. 修 6 → PASS

## 实现期发现（学员不知道，结案时可聊）

- loongarch 内核固定使用 vDSO restorer，rt_sigaction 的 sa_restorer 被忽略
  （break 0 实验证实）——restorer 汇编在 loongarch 上是死代码，
  x86_64 上才是必需品。spec 的"类似 trap 实现汇编"决策在此架构无实际效果。
- 最初埋的 restorer 寄存器 bug 因此不可触发，已修正为正确代码并换埋 bug1。
- workspace glob `app/*` 迫使 C-only app 进 exclude 列表（已加）。
