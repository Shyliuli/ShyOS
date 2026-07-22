#ifndef BOARD_BOARD_H
#define BOARD_BOARD_H

#include "shy_type.h"

#ifdef BOARD_QEMU_VIRT
#include "qemu_virt/qemu.h"
#endif

#if defined(SHYOS_BACKEND_LINUX_USER)
#include "linux_user/linux_user.h"
#endif

u64 randseed(void);

#endif
