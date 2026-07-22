# ShyOS C 对象所有权原语

SHYOS_ROOT ?= $(abspath ../../..)
include $(SHYOS_ROOT)/mk/config.mk

OUT_DIR := target
OBJ_C_OBJ := $(OUT_DIR)/obj_c.o

.PHONY: build clean

build: $(OBJ_C_OBJ)

$(OUT_DIR):
	mkdir -p $@

$(OBJ_C_OBJ): obj.c obj.h $(SHYOS_CONFIG_STATE) | $(OUT_DIR)
	$(CC) $(CFLAGS) -c -o $@ obj.c

clean:
	rm -rf $(OUT_DIR)
