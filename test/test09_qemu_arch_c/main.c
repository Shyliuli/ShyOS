#include "board.h"
#include "irq.h"
#include "shy_type.h"
#include "uart.h"

i32 main(void)
{
	irq_mask_t previous;

	if (UART0_BASE == 0 || SIFIVE_TEST_BASE == 0)
		return 1;

	raw_putc('9');
	raw_putc('\n');

	(void)global_irq(false);
	previous = set_irq(0);
	if ((previous & ~IRQ_MASK_ALL) != 0)
		return 2;
	if (get_irq() != 0)
		return 3;

	return 0;
}
