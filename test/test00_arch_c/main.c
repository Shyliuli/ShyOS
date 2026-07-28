#include "board.h"
#include "irq.h"
#include "rand.h"
#include "shy_type.h"
#include "time.h"
#include "uart.h"

i32 main(void)
{
	irq_id_t claimed = 1;
	u64 t0;
	u64 t1;

	if (HEAP_START != 0 || HEAP_SIZE == 0)
		return 1;

	raw_putc('0');
	raw_putc('\n');

	if (set_trap_entry(0) != IRQ_ERROR_UNSUPPORTED)
		return 2;
	if (get_irq() != 0)
		return 3;
	if (claim_external_irq(&claimed) != IRQ_STATUS_NONE || claimed != 0)
		return 4;

	rand_seed(1);
	u64 first = rand_u64();
	rand_seed(1);
	if (rand_u64() != first)
		return 5;
	for (usize i = 0; i < 32; i++) {
		i32 value = randint(3, 7);
		if (value < 3 || value > 7)
			return 6;
	}

	/* init_time 已在 _shy_os_init 中调用；单调非减即可。 */
	t0 = get_time();
	t1 = get_time();
	if (t1 < t0)
		return 7;

	return 0;
}
