SHYOS_ROOT ?= $(abspath ../../..)
include $(SHYOS_ROOT)/mk/config.mk

OUT_DIR := $(SHYOS_TARGET_DIR)
ATOMIC_C_OBJ := $(OUT_DIR)/atomic_c.o

.PHONY: build clean cargo-check

build: $(ATOMIC_C_OBJ)

$(OUT_DIR):
	mkdir -p $@

$(ATOMIC_C_OBJ): atomic.c atomic.h $(SHYOS_CONFIG_STATE) | $(OUT_DIR)
	$(CC) $(CFLAGS) -c -o $@ atomic.c

cargo-check:
	$(CARGO) check --manifest-path Cargo.toml --target $(RUST_TARGET)

clean:
	rm -rf target
