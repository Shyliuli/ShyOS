# vec C 对象

SHYOS_ROOT ?= $(abspath ../../..)
include $(SHYOS_ROOT)/mk/config.mk

OUT_DIR := target
VEC_C_OBJ := $(OUT_DIR)/vec_c.o

.PHONY: build clean

build: $(VEC_C_OBJ)

$(OUT_DIR):
	mkdir -p $@

$(VEC_C_OBJ): vec.c vec.h $(SHYOS_ROOT)/lib/03basic/obj/obj.h \
		$(SHYOS_ROOT)/lib/03basic/result/result.h \
		$(SHYOS_ROOT)/lib/04data_structure/rawbuf/rawbuf.h \
		$(SHYOS_CONFIG_STATE) | $(OUT_DIR)
	$(CC) $(CFLAGS) -c -o $@ vec.c

clean:
	rm -rf $(OUT_DIR)
