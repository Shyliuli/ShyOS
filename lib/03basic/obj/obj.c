#include "obj.h"

#include "alloc.h"
#include "string_noalloc.h"

void obj_move(TypeDesc type, void *dst, void *src)
{
	memcpy(dst, src, type.size);
	if (is_owned(type)) {
		memset(src, 0, type.size);
	}
}

void *obj_clone(TypeDesc type, const void *src)
{
	void *copy;

	if (is_owned(type)) {
		if (type.clone == NULL) {
			return NULL;
		}
		return type.clone(src);
	}
	copy = malloc(type.size);
	if (copy == NULL) {
		return NULL;
	}
	memcpy(copy, src, type.size);
	return copy;
}

void obj_drop(TypeDesc type, void *value)
{
	if (type.drop != NULL) {
		type.drop(value);
	}
}
