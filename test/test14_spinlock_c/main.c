#include "early_stdio.h"
#include "spinlock.h"

i32 main(void)
{
	struct spinlock raw;
	Spinlocki32 typed;

	spinlock_init(&raw);
	spinlock_lock(&raw);
	if (raw.locked != 1)
		return 1;
	spinlock_unlock(&raw);
	if (raw.locked != 0)
		return 2;

	spinlock_init(&typed);
	spinlock_lock(&typed);
	*i32_spinlock_get(&typed) = 42;
	if (typed.value != 42)
		return 3;
	if (typed.lock.locked != 1)
		return 4;
	spinlock_unlock(&typed);
	if (typed.lock.locked != 0)
		return 5;

	early_printf("test14 spinlock ok\n");
	return 0;
}
