SHYOS_ROOT ?= $(abspath ../../..)
include $(SHYOS_ROOT)/mk/config.mk

OUT_DIR := $(SHYOS_TARGET_DIR)
TIMER_C_OBJ := $(OUT_DIR)/timer_c.o

.PHONY: build clean cargo-check

build: $(TIMER_C_OBJ)

$(OUT_DIR):
	mkdir -p $@

$(TIMER_C_OBJ): timer.c timer.h \
		$(SHYOS_ROOT)/lib/00arch/linux_user_signal/linux_user_signal.h \
		$(SHYOS_ROOT)/lib/02core/linux_user_trap/linux_user_trap.h \
		$(SHYOS_ROOT)/lib/00arch/time/time.h \
		$(SHYOS_ROOT)/lib/board/board.h \
		$(SHYOS_CONFIG_STATE) | $(OUT_DIR)
	$(CC) $(CFLAGS) -c -o $@ timer.c

cargo-check:
	$(CARGO) check --manifest-path Cargo.toml --target $(RUST_TARGET)

clean:
	rm -rf target
