#include "panic.h"
#include "shy_type.h"
#include "early_stdio.h"
#include "stdio.h"

i32 main(void)
{
	char buffer[32];
	volatile bool fail = false;

	if (snprintf(buffer, sizeof(buffer), "%s:%d", "log", 2) != 5)
		return 1;
	if (buffer[0] != 'l' || buffer[4] != '2')
		return 2;

	if (early_printf("test02 %s\n", buffer) <= 0)
		return 3;

	assert(!fail);
	return 0;
}
