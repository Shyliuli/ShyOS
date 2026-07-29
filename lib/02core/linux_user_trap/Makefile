SHYOS_ROOT ?= $(abspath ../../..)
include $(SHYOS_ROOT)/mk/config.mk

OUT_DIR := $(SHYOS_TARGET_DIR)
TRAP_C_OBJ := $(OUT_DIR)/linux_user_trap_c.o
TRAP_S_OBJ := $(OUT_DIR)/signal_restorer.o

.PHONY: build clean

ifeq ($(BACKEND),linux_user)
build: $(TRAP_C_OBJ) $(TRAP_S_OBJ)
else
build:
endif

$(OUT_DIR):
	mkdir -p $@

$(TRAP_C_OBJ): linux_user_trap.c linux_user_trap.h \
		$(SHYOS_ROOT)/lib/board/board.h \
		$(SHYOS_ROOT)/lib/board/linux_user/syscall.h \
		$(SHYOS_CONFIG_STATE) | $(OUT_DIR)
	$(CC) $(CFLAGS) -c -o $@ linux_user_trap.c

$(TRAP_S_OBJ): signal_restorer.S $(SHYOS_CONFIG_STATE) | $(OUT_DIR)
	$(AS) $(ASFLAGS) -c -o $@ signal_restorer.S

clean:
	rm -rf target
