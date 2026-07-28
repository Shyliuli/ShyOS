# 04data_structure

依赖 `03basic`，提供需要 allocator 和对象所有权语义的数据结构：

- `rawbuf`：只管理分配、元素字节大小和容量，不管理对象生命周期；
- `vec`：C `Vec` 与 Rust `CVec<T>` RAII 包装；
- `deque`：基于 `RawBuf` 的无哨兵槽环形双端队列；
- `ringbuffer`：基于 `Deque` 的覆盖式队列；满时 push 丢弃最老元素；
- `stack` / `queue`：基于 `Deque` 的栈、普通队列与 Rust 包装；
- `string`：直接基于 `RawBuf` 的拥有型 C `String` 与 Rust `CString`；
- `hashtable` / `hashset`：平行 `RawBuf` 开放寻址表及其集合包装；
- `linked_list`：基于 `RawBuf` arena 的双向链表，节点链接和 freelist 均使用 index；
- `binary_heap`：基于 `RawBuf` 的二叉堆；C 侧 `cmp(a,b) > 0` 表示 a 应在 b 之上；Rust 侧 `HeapOrder::{Max,Min}` 选择堆序；
- Rust facade：`shyos-data-structure`，重新导出 `shyos-basic` 与本层容器。

本层内部以 `rawbuf -> deque -> ringbuffer/stack/queue` 为最长依赖链，
深度为 3。`Vec`、`String`、`Hashtable`、`LinkedList` 和 `BinaryHeap` 直接管理各自的逻辑
生命周期，`RawBuf` 不持有 `TypeDesc`。Result 与对象所有权语义由 `03basic` 提供。
