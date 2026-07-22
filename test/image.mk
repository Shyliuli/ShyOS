# Common final-image rules used by the independent projects under test/.

ifndef TEST_NAME
$(error project Makefile must define TEST_NAME)
endif
ifndef TEST_LANGUAGE
$(error project Makefile must define TEST_LANGUAGE)
endif

include $(SHYOS_ROOT)/mk/config.mk

OUT_DIR := target
TEST_ELF := $(OUT_DIR)/$(TEST_NAME).elf
TEST_EXTRA_OBJS ?=

$(OUT_DIR):
	mkdir -p $@

ifeq ($(TEST_LANGUAGE),c)
TEST_OBJ := $(OUT_DIR)/main.o

$(TEST_OBJ): main.c $(SHYOS_CONFIG_STATE) | $(OUT_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

IMAGE_INPUTS := $(TEST_OBJ) $(TEST_EXTRA_OBJS)
else ifeq ($(TEST_LANGUAGE),rust)
ifndef RUST_LIB_NAME
$(error Rust project Makefile must define RUST_LIB_NAME)
endif

RUST_STATICLIB := \
	$(CARGO_TARGET_DIR)/$(RUST_TARGET)/$(CARGO_PROFILE)/lib$(RUST_LIB_NAME).a

$(RUST_STATICLIB): Cargo.toml src/lib.rs $(SHYOS_RUST_INPUTS) \
		$(SHYOS_CONFIG_STATE) | $(OUT_DIR)
	$(CARGO) build \
			--manifest-path Cargo.toml \
			--target $(RUST_TARGET) \
			$(CARGO_BUILD_STD_FLAGS) \
			$(CARGO_PROFILE_ARGS)
	touch $@

IMAGE_ENTRY_FLAGS += -Wl,-u,main
IMAGE_INPUTS :=
IMAGE_RUST_STATICLIB := $(RUST_STATICLIB)
else
$(error unsupported TEST_LANGUAGE=$(TEST_LANGUAGE))
endif

IMAGE_NAME := $(TEST_NAME)
IMAGE_ELF := $(TEST_ELF)

include $(SHYOS_ROOT)/mk/image.mk
