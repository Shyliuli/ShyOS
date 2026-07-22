#include "board.h"
#include "irq.h"
#include "shy_type.h"

extern void test10_trap_entry(void);

volatile i32 test10_result;

static usize read_scause(void)
{
	usize value;

	__asm__ volatile("csrr %0, scause" : "=r"(value));
	return value;
}

static usize read_cycle(void)
{
	usize value;

	__asm__ volatile("rdcycle %0" : "=r"(value));
	return value;
}

void test10_handle(void)
{
	const usize interrupt_bit = (usize)1 << (sizeof(usize) * 8 - 1);
	irq_id_t irq = 0;

	if ((read_scause() & ~interrupt_bit) != 9) {
		test10_result = -1;
		return;
	}
	if (claim_external_irq(&irq) != IRQ_STATUS_CLAIMED ||
	    irq != IRQ_ID_EXTERNAL(UART0_IRQ)) {
		test10_result = -2;
		return;
	}

	*(volatile u8 *)(usize)(UART0_BASE + UART_IER) = 0;
	(void)*(volatile u8 *)(usize)(UART0_BASE + UART_IIR);
	if (complete_external_irq(irq) != IRQ_STATUS_OK) {
		test10_result = -3;
		return;
	}
	test10_result = 1;
}

i32 main(void)
{
	const irq_id_t uart_irq = IRQ_ID_EXTERNAL(UART0_IRQ);
	usize start;

	(void)global_irq(false);
	(void)set_irq(0);
	if (set_trap_entry((usize)test10_trap_entry) != IRQ_STATUS_OK)
		return 1;
	if (enable_irq_source(uart_irq) != IRQ_STATUS_OK)
		return 2;
	(void)enable_irq(IRQ_MASK_EXTERNAL);

	*(volatile u8 *)(usize)(UART0_BASE + UART_IER) = 1U << 1;
	(void)global_irq(true);

	start = read_cycle();
	while (test10_result == 0 && read_cycle() - start < 10000000UL)
		__asm__ volatile("nop");

	(void)global_irq(false);
	*(volatile u8 *)(usize)(UART0_BASE + UART_IER) = 0;
	(void)disable_irq(IRQ_MASK_EXTERNAL);
	(void)disable_irq_source(uart_irq);

	return test10_result == 1 ? 0 : 3;
}
