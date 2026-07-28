/* 00arch 原子操作接口；由编译器原子内建实现，两个 backend 通用。 */
#ifndef SHYOS_ATOMIC_H
#define SHYOS_ATOMIC_H

#include "shy_type.h"

/* 原子地把 *ptr 换成 new_value，返回 *ptr 的旧值。 */
i32 xchg(i32 *ptr, i32 new_value);

#endif /* SHYOS_ATOMIC_H */
