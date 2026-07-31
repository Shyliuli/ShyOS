# spec-02: pmm（物理页管理）与内核堆的内存切分

来源：学员讨论材料整理（2026-07-30）。粒度跟随学员粒度；未表达过的细节不补全。

## 背景（讨论结论）

- `lib/alloc` 不是物理内存管理，只是"给定一段地址区间做 malloc/free"
  的堆分配器，对物理/虚拟无感；区间之外的 RAM 现状是无人管理。
- 内核保持直接映射，内核堆不走虚拟内存；vmm 将来只服务用户地址空间。
- 因此引入 pmm 是纯增量，不改 malloc/free 接口。

## 学员已定决策

- 假设板子内存足够大。可分配内存（去掉内核镜像等占用）的**开头 32M**
  划给 alloc 后端作为内核堆；所有内核 malloc/free 走这里。
  32M 是拍的一个数，将来不够用直接改这个数字。
  头文件写死：堆区间 = `[START, START + 32*1024*1024)`。
- 可分配内存起点维持现状：头文件写死（沿用 HEAP_START 模式），
  不用链接脚本符号。
- 新层 `06page_memory`，pmm 模块在其中。
- pmm 接口：`malloc_page()` 和 `free_page()`，freelist 管理 4K page。
- pmm 抽象成 capability（类似 alloc 的模式），可选 provider。
- init 可以 hack：init.c **无条件**调用 `page_memory_init()`；
  若选择了 NONE provider，则为空实现。
- 剩余内存归属：
  - 选择了 pmm provider → 堆区间之外的剩余全部交给 pmm；
  - NONE / 无 pmm → alloc 后端接管全部（总内存大小维持现状写死）。
- `SHYOS_BACKEND_LINUX_USER` 下不存在 vmm/页的概念 → 选 NONE provider，
  alloc 接管全部（维持现状的 brk/sbrk 向宿主申请）。
  linux_user 将来做进程时直接交给 host OS 处理。
- 堆的 32M 截断由 init.c 按 provider **条件编译**完成：保留 heap_init，
  有 pmm 传 32M，否则传全部；`page_memory_init()` 维持无条件调用、
  NONE 空实现。
- `malloc_page` / `free_page`：只支持单页，不支持连续多页；
  失败返回 NULL。

## 测试 spec（学员 2026-07-30）

1. NONE provider 下，包含 malloc_page/free_page 调用的文件**无法编译**。
2. 分配返回的地址必须是 4K 对齐的物理地址。
3. 多次 free-malloc 之后仍能正确分配（free 的页正确回到 freelist）。

学员补充澄清：pmm 自身没有 32M 边界问题——pmm 管理的不是 32M，
而是 32M 堆之外的全部剩余内存。

头文件结构（学员确认）：`page_memory_init` 始终声明，NONE provider
下补一个空实现；`malloc_page`/`free_page` 只在选择了实体 provider
时才声明。

## 此处未定义

- capability 模块的落位：alloc 的现成模式是无编号共享模块（lib/alloc）
  + 层内 provider（03basic/simple_alloc）；pmm 是否照搬
  （lib/pmm + 06page_memory 内 provider）学员未明确。
- provider define 的命名（类似 SHYOS_UART_* 的 SHYOS_PMM_*？）。
- pmm freed page 的元数据放哪（freelist 通常直接利用空闲页自身，
  学员未明确）。
- Rust 侧接口的包装形状。

## 不在本 spec 范围（留待后续 spec）

- vmm / 页表 / 用户地址空间。
- task、fork。
- 内核堆从 pmm 按需增长的动态后端。
- linux_user 下的进程模型（交 host OS 处理）。
