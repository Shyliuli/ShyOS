#include "vec.h"

#include "alloc.h"
#include "string_noalloc.h"

enum { VEC_INIT_CAP = 5 };

static void *vec_ptr_add(const Vec *v, usize i)
{
	return (void *)((char *)v->data + i * v->elem.size);
}

static void vec_drop_item(Vec *v, usize i)
{
	obj_drop(v->elem, vec_ptr_add(v, i));
}

ResultVec vec_new(TypeDesc elem)
{
	if (elem.size == 0) {
		return VecErr(ERROR_INVALID_ARGUMENT);
	}
	return VecOk((Vec){
		.data = NULL,
		.cap = 0,
		.size = 0,
		.elem = elem,
	});
}

Vec vec_from_raw_parts(void *data, usize size, usize cap, TypeDesc elem)
{
	return (Vec){
		.data = data,
		.cap = cap,
		.size = size,
		.elem = elem,
	};
}

usize vec_len(const Vec *v)
{
	return v->size;
}

usize vec_capacity(const Vec *v)
{
	return v->cap;
}

bool vec_is_empty(const Vec *v)
{
	return v->size == 0;
}

i32 vec_reserve(Vec *v, usize additional)
{
	usize required;
	usize new_cap;
	void *new_data;

	if (additional > (usize)-1 - v->size) {
		return -1;
	}
	required = v->size + additional;
	if (required <= v->cap) {
		return 0;
	}
	new_cap = v->cap == 0 ? (usize)VEC_INIT_CAP : v->cap;
	while (new_cap < required) {
		usize growth = new_cap / 2;
		if (growth == 0) {
			growth = 1;
		}
		if (new_cap > (usize)-1 - growth) {
			return -1;
		}
		new_cap += growth;
	}
	if (new_cap > (usize)-1 / v->elem.size) {
		return -1;
	}
	new_data = malloc(new_cap * v->elem.size);
	if (new_data == NULL) {
		return -1;
	}
	if (v->size != 0) {
		memcpy(new_data, v->data, v->size * v->elem.size);
	}
	if (v->data != NULL) {
		free(v->data);
	}
	v->data = new_data;
	v->cap = new_cap;
	return 0;
}

static i32 vec_insert_impl(Vec *v, usize index, const void *data, bool clear_source)
{
	void *slot;

	if (data == NULL || index > v->size) {
		return -1;
	}
	if (vec_reserve(v, 1) != 0) {
		return -1;
	}
	memmove(vec_ptr_add(v, index + 1), vec_ptr_add(v, index),
		(v->size - index) * v->elem.size);
	slot = vec_ptr_add(v, index);
	if (clear_source) {
		obj_move(v->elem, slot, (void *)data);
	} else {
		memcpy(slot, data, v->elem.size);
	}
	v->size++;
	return 0;
}

i32 vec_push(Vec *v, void *data)
{
	return vec_insert_impl(v, v->size, data, true);
}

i32 vec_insert(Vec *v, usize index, void *data)
{
	return vec_insert_impl(v, index, data, true);
}

i32 vec_push_raw(Vec *v, const void *data)
{
	return vec_insert_impl(v, v->size, data, false);
}

i32 vec_insert_raw(Vec *v, usize index, const void *data)
{
	return vec_insert_impl(v, index, data, false);
}

void *vec_get(const Vec *v, usize i)
{
	return vec_ptr_add(v, i);
}

i32 vec_pop(Vec *v, void *out)
{
	if (v->size == 0 || out == NULL) {
		return -1;
	}
	v->size--;
	obj_move(v->elem, out, vec_ptr_add(v, v->size));
	return 0;
}

i32 vec_remove(Vec *v, usize index, void *out)
{
	if (index >= v->size) {
		return -1;
	}
	if (out != NULL) {
		obj_move(v->elem, out, vec_ptr_add(v, index));
	} else {
		vec_drop_item(v, index);
	}
	memmove(vec_ptr_add(v, index), vec_ptr_add(v, index + 1),
		(v->size - index - 1) * v->elem.size);
	v->size--;
	return 0;
}

i32 vec_swap_remove(Vec *v, usize index, void *out)
{
	usize last;

	if (index >= v->size) {
		return -1;
	}
	last = v->size - 1;
	if (out != NULL) {
		obj_move(v->elem, out, vec_ptr_add(v, index));
	} else {
		vec_drop_item(v, index);
	}
	if (index != last) {
		memcpy(vec_ptr_add(v, index), vec_ptr_add(v, last),
			v->elem.size);
	}
	v->size = last;
	return 0;
}

void vec_truncate(Vec *v, usize len)
{
	if (len >= v->size) {
		return;
	}
	while (v->size > len) {
		v->size--;
		vec_drop_item(v, v->size);
	}
}

void vec_clear(Vec *v)
{
	vec_truncate(v, 0);
}

void vec_drop(void *self)
{
	Vec *v = self;

	vec_clear(v);
	if (v->data != NULL) {
		free(v->data);
	}
	memset(v, 0, sizeof(*v));
}

void *vec_clone_obj(const void *source_ptr)
{
	const Vec *source = source_ptr;
	Vec *copy;
	ResultVec result;
	usize i;

	copy = malloc(sizeof(*copy));
	if (copy == NULL) {
		return NULL;
	}
	result = vec_new(source->elem);
	if (Vec_is_err(&result)) {
		free(copy);
		return NULL;
	}
	*copy = Vec_unwrap(&result);
	if (vec_reserve(copy, source->size) != 0) {
		vec_drop(copy);
		free(copy);
		return NULL;
	}
	if (is_copy(source->elem)) {
		if (source->size != 0) {
			memcpy(copy->data, source->data,
				source->size * source->elem.size);
		}
		copy->size = source->size;
		return copy;
	}
	for (i = 0; i < source->size; ++i) {
		void *element = obj_clone(source->elem, vec_ptr_add(source, i));
		if (element == NULL) {
			vec_drop(copy);
			free(copy);
			return NULL;
		}
		obj_move(source->elem, vec_ptr_add(copy, copy->size), element);
		copy->size++;
		free(element);
	}
	return copy;
}

ResultVec vec_clone(const Vec *source)
{
	Vec *copy = vec_clone_obj(source);
	Vec value;

	if (copy == NULL) {
		return VecErr(ERROR_OUT_OF_MEMORY);
	}
	value = *copy;
	memset(copy, 0, sizeof(*copy));
	free(copy);
	return VecOk(value);
}
