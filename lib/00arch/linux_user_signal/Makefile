SHYOS_ROOT ?= $(abspath ../../..)
include $(SHYOS_ROOT)/mk/config.mk

OUT_DIR := $(SHYOS_TARGET_DIR)
SIGNAL_C_OBJ := $(OUT_DIR)/linux_user_signal_c.o

.PHONY: build clean

ifeq ($(BACKEND),linux_user)
build: $(SIGNAL_C_OBJ)
else
build:
endif

$(OUT_DIR):
	mkdir -p $@

$(SIGNAL_C_OBJ): linux_user_signal.c linux_user_signal.h \
		$(SHYOS_ROOT)/lib/board/board.h \
		$(SHYOS_ROOT)/lib/board/linux_user/syscall.h \
		$(SHYOS_CONFIG_STATE) | $(OUT_DIR)
	$(CC) $(CFLAGS) -c -o $@ linux_user_signal.c

clean:
	rm -rf target
