#include "alloc.h"
#include "obj.h"
#include "result.h"
#include "early_stdio.h"

typedef struct Probe {
	i32 *drops;
} Probe;

static void probe_drop(void *probe_ptr)
{
	Probe *probe = probe_ptr;

	(*probe->drops)++;
	probe->drops = NULL;
}

static void *probe_clone(const void *source)
{
	Probe *copy = malloc(sizeof(*copy));

	if (copy != NULL)
		*copy = *(const Probe *)source;
	return copy;
}

IMPL_SHY_OWNED_RESULT(Probe, probe_drop, probe_clone)

_Static_assert(sizeof(Resulti32) == 8, "Resulti32 ABI size");

i32 main(void)
{
	Resulti32 number = i32Ok(4);
	ResultProbe owned;
	ResultProbe previous;
	i32 drops = 0;

	if (!i32_is_ok(&number) || *i32_unwrap_ref(&number) != 4)
		return 1;
	*i32_unwrap_ref_mut(&number) = 5;
	if (i32_unwrap(&number) != 5 ||
	    !i32_is_none(&number) || i32_error(&number) != ERROR_NO_VALUE)
		return 2;
	if (i32_unwrap_or(&number, 6) != 6)
		return 3;

	owned = ProbeOk((Probe){ .drops = &drops });
	previous = Probe_replace(&owned, (Probe){ .drops = &drops });
	ResultProbe_drop(&previous);
	if (drops != 1)
		return 4;
	ResultProbe_drop(&owned);
	if (drops != 2)
		return 5;

	{
		let(ResultProbe) scoped =
			ProbeOk((Probe){ .drops = &drops });
		if (Probe_is_err(&scoped))
			return 6;
	}
	if (drops != 3)
		return 7;

	early_printf("test06 basic result c\n");
	return 0;
}
