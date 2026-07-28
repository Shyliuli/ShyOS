SHYOS_ROOT ?= $(abspath ../../..)
include $(SHYOS_ROOT)/mk/config.mk

OUT_DIR := $(SHYOS_TARGET_DIR)
SPINLOCK_C_OBJ := $(OUT_DIR)/spinlock_c.o

.PHONY: build clean cargo-check

build: $(SPINLOCK_C_OBJ)

$(OUT_DIR):
	mkdir -p $@

$(SPINLOCK_C_OBJ): spinlock.c spinlock.h \
		$(SHYOS_ROOT)/lib/00arch/atomic/atomic.h \
		$(SHYOS_ROOT)/lib/00arch/irq/irq.h \
		$(SHYOS_CONFIG_STATE) | $(OUT_DIR)
	$(CC) $(CFLAGS) -c -o $@ spinlock.c

cargo-check:
	$(CARGO) check --manifest-path Cargo.toml --target $(RUST_TARGET)

clean:
	rm -rf target
