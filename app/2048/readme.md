验证层级basic能力
链接basic层
rust侧实现算法逻辑
c侧实现draw
通过Vec<struct state>ffi传状态.

Rust 导出 `index(State *, i, j) -> Resulti32 *` 借用棋盘槽位，C `draw` 检查
Result tag 后读取 `ok` 字段，不复制或转移棋盘元素所有权。
