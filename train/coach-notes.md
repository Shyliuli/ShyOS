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

## 2026-07-30 spec-02 整理时怀疑的学员误解 / 未检验区

1. 讨论开始时认为"alloc 直接管理了整个可用内存"——实际 HEAP_START/HEAP_SIZE
   是 board 写死的固定区间，区间外 RAM 无人管理（已当场纠正，吸收未验证）。
2. 讨论开始时认为引入 vmm 必须大改 alloc——未分清 heap/pmm/vmm 三个层次
   （讨论后接受"纯增量"，三层模型是否真正建立未验证）。
3. "NONE 下 malloc_page 不可编译"与"无条件 page_memory_init"之间的头文件
   分离（接口可见性由 provider 条件编译控制）不是学员主动意识到的，
   由教练提出后确认——对条件编译控制 ABI 可见性的机制可能不熟。
4. 学员对 pmm 范围与 32M 堆的关系最终澄清正确
   （pmm 管 32M 之外的剩余，自身无 32M 边界）。

## 2026-07-31 spec-02 实现：重新设计注入清单

学员反馈原前两项只是预处理符号和 Make 条件错误，未迫使其推演 PMM
核心机制。取消这两项，恢复正确头文件可见性和 provider 层组装；训练仍按
stage=2，不告知数量与模块。

| # | 位置 | 类别 | 内容 | 预期症状 |
|---|------|------|------|----------|
| 1 | lib/06page_memory/freelist/freelist.c | 算法/不变量 | 归还页的 next 指向旧表头的 next，跳过旧表头；初始化也复用 free_page，因此链最终只保留极少节点 | test15 运行期耗尽计数远小于预期；需推演 intrusive freelist 的插入不变量及初始化复用路径 |
| 2 | lib/06page_memory/freelist/freelist.c | 跨模块/内存切分 | page_memory_init 使用 HEAP_START/HEAP_SIZE，而非 board 定义的 PMM_START/PMM_SIZE | 修 #1 后耗尽计数比预期多 8192 页，需核对 alloc、board、PMM 三者的区间所有权 |

原注入 #1（pmm.h 漏 `!`）与 #2（Makefile provider 条件反向）废弃，不再作为
练习内容。

## 2026-07-31 重新设计后的级联验证

1. 当前训练态：构建、DAG、NONE 可见性检查通过；QEMU 运行
   `FAIL: exhaust count=1 expect=40960`。
2. 临时修正新 #1：QEMU 运行
   `FAIL: exhaust count=49152 expect=40960`，新 #2 独立暴露。
3. 再临时修正新 #2：QEMU 运行
   `test15 pmm ok (40960 pages)`，NONE 可见性检查仍通过。
4. 已恢复新 #1 与新 #2；当前练习从新 #1 的运行期症状开始。

## 2026-07-30 原级联验证记录（已废弃）

原记录仅保留为训练设计复盘，不代表当前练习状态。

## 实现期发现（学员不知道，结案时可聊）

- 层 archive 的 make 依赖不含层 Makefile 与模块头文件：改 Makefile 的
  对象收集条件或改 pmm.h 不会触发 archive/main.o 重打，过期产物会掩盖
  bug——交付前已 clean 重建 NONE 产物并删 test15 target，保证学员首建
  即遇症状 1。这个构建系统 staleness 值得另开议题。
- 前 agent 中断时 test15 的 check-none-compile 只定义了目标没挂进
  run 流程；已挂（test15 Makefile `run: check-none-compile`）。
- timer_demo 的 IMAGE_LAYER 在 05task→05timer 改名中被漏改，make all
  会失败；已修。tools/layer_graph.py 内的 05task 是虚构演示数据，未动。
- bug1 的 NONE 可见性症状依赖 gcc>=14 把 implicit-function-declaration
  当 error；本机 gcc 15.2/15.3 成立。
