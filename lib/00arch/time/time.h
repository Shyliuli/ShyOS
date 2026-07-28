#include "shy_type.h"
#if !defined(SHYOS_BACKEND_LINUX_USER)
void set_timer(u64 ms);
u64 get_time();
void init_time();

#endif