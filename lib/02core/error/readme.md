# Error

`errno.h` 提供 Linux 通用 errno 数值，并为每个短名称提供可读的
`ERROR_*` 别名。`ENONE` / `ERROR_NO_VALUE` 是 ShyOS 扩展，用于
`CResult::none()`，不表示一个独立的 Result 状态。
