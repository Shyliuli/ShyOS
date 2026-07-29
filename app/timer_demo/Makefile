SHYOS_ROOT ?= $(abspath ../..)
SHYOS_CONFIG ?= $(CURDIR)/shyos.mk
include $(SHYOS_ROOT)/mk/config.mk

OUT_DIR := target
APP_C_OBJ := $(OUT_DIR)/main.o

$(OUT_DIR):
	mkdir -p $@

$(APP_C_OBJ): main.c $(SHYOS_CONFIG_STATE) | $(OUT_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

IMAGE_NAME := timer_demo
IMAGE_INPUTS := $(APP_C_OBJ)

include $(SHYOS_ROOT)/mk/app.mk
