#include "early_stdio.h"
#include "string_noalloc.h"

i32 main(void)
{
	char buffer[16];
	const char *text = "core-string";
	if (strlen(text) != 11 || strcmp(text, "core-string") != 0)
		return 1;

	memset(buffer, 0, sizeof(buffer));
	memcpy(buffer, "core", 5);
	if (memcmp(buffer, "core", 5) != 0)
		return 2;
	if (strlcat(buffer, "-c", sizeof(buffer)) != 6)
		return 3;

	early_printf("test04 %s\n", buffer);
	return 0;
}
