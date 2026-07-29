# 训练日志

模式：标准训练循环 | 难度阶段：2（告知有 bug，不告知数量与模块）

```
2026-07-28 | ShyOS / irq(linux_user)+timer+time | 模式：合成
spec: train/spec-01-signal-irq-timer.md（学员讨论材料整理，学员已批注修订）
参数：S≈450 E=5 A=0.5 T=是 | λ≈8 | 实际注入 N=6（零注入：否）
各类别实际注入数：定向 2 / 接口 2 / 边界(测试) 1 / 算法 1 / 赋值 1
命中 6/6（逐个定位并修复，gdb watchpoint + strace 为主要仪器）| 提示未动用阶梯（学员自主完成）
暴露的误解：信号屏蔽语义（pending 不丢失）、UNBLOCK vs SETMASK、
  kernel_sigset_t ≠ glibc sigset_t、测试自身可作为 bug 源、ptrace 改变时序
后续动作：学员反馈首次练习 6 bug 剂量过大且发现时已即时解释，
  将自行修订协议（结案报告制度）
```

## 豁免记录

- 2026-07-28：学员以"首次使用、6 bug 记不住、发现时已即时解释"为由跳过
  结案三问报告。理由成立，准予本次豁免；学员表示将自行修订协议相关条款。
- 2026-07-29：spinlock 自动屏蔽（spec 修订，方案 B：irq linux_user 转发
  linux_user_signal）按学员明确要求不埋雷，正常实现。已验证：
  app/timer_demo PASS、05task 层 QEMU 配置构建通过、test11/test05 无回归。
