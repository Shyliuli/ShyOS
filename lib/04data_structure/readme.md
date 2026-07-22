# 04data_structure

依赖 `03basic`，提供需要 allocator 和对象所有权语义的数据结构：

- `vec`：C `Vec` 与 Rust `CVec<T>` RAII 包装；
- `string`：拥有型 C `String` 与 Rust `CString` 包装；
- Rust facade：`shyos-data-structure`，重新导出 `shyos-basic` 与本层容器。

本层内部依赖为 `vec -> string`，最长链深度为 2。Result 与对象所有权语义由
`03basic` 提供。
