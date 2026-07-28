#include "atomic.h"

i32 xchg(i32 *ptr, i32 new_value)
{
	return __atomic_exchange_n(ptr, new_value, __ATOMIC_ACQ_REL);
}
