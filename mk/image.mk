# Common final-image build rules. The project Makefile must load mk/config.mk
# first and define IMAGE_NAME, IMAGE_LAYER, and IMAGE_INPUTS.

ifndef IMAGE_NAME
$(error image Makefile must define IMAGE_NAME)
endif
ifndef IMAGE_LAYER
$(error $(SHYOS_CONFIG_FILE) must define IMAGE_LAYER)
endif
ifeq ($(origin IMAGE_INPUTS),undefined)
$(error image Makefile must define IMAGE_INPUTS)
endif

IMAGE_TOOL := $(SHYOS_ROOT)/tools/layer_graph.py
IMAGE_LAYERS := $(shell python3 $(IMAGE_TOOL) --root $(SHYOS_ROOT) list --image)
IMAGE_LAYER_INPUTS := $(shell find $(SHYOS_ROOT)/lib \
	-path '*/target' -prune -o \
	-type f \( \
		-name 'Cargo.toml' -o \
		-name 'Makefile' -o \
		-name 'layer.toml' -o \
		-name '*.c' -o \
		-name '*.h' -o \
		-name '*.rs' -o \
		-name '*.S' -o \
		-name '*.ld' \
	\) -print)

ifeq ($(filter $(IMAGE_LAYER),$(IMAGE_LAYERS)),)
$(error unsupported IMAGE_LAYER=$(IMAGE_LAYER); expected one of: $(IMAGE_LAYERS))
endif

IMAGE_OUT_DIR ?= target
IMAGE_ELF ?= $(IMAGE_OUT_DIR)/$(IMAGE_NAME).elf
IMAGE_BIN ?= $(IMAGE_OUT_DIR)/$(IMAGE_NAME).bin
IMAGE_EMIT_BIN ?= 0
IMAGE_ENTRY_FLAGS ?=
IMAGE_RUST_STATICLIB ?=
ifeq ($(BACKEND),linux_user)
IMAGE_ENTRY_FLAGS += -Wl,-u,_shy_os_init
endif
ifneq ($(strip $(IMAGE_RUST_STATICLIB)),)
IMAGE_LAYER_VARIANT := norust
IMAGE_LAYER_BUILD_TARGET := build-norust
else
IMAGE_LAYER_VARIANT := rust
IMAGE_LAYER_BUILD_TARGET := build
endif
ifeq ($(IMAGE_LAYER_VARIANT),norust)
IMAGE_LAYER_FILE := \
	$(SHYOS_ROOT)/lib/$(IMAGE_LAYER)/target/$(IMAGE_LAYER).norust.layer.ld
else
IMAGE_LAYER_FILE := \
	$(SHYOS_ROOT)/lib/$(IMAGE_LAYER)/target/$(IMAGE_LAYER).layer.ld
endif
IMAGE_LAYER_STAMP := \
	$(SHYOS_ROOT)/lib/$(IMAGE_LAYER)/target/.$(IMAGE_LAYER_VARIANT).image.stamp
QEMU_MEM ?= 256M

.PHONY: build layer run clean print-config dag-html

ifeq ($(IMAGE_EMIT_BIN),1)
build: $(IMAGE_BIN)
else
build: $(IMAGE_ELF)
endif

layer:
	$(MAKE) -C $(SHYOS_ROOT)/lib/$(IMAGE_LAYER) $(IMAGE_LAYER_BUILD_TARGET)

$(IMAGE_LAYER_STAMP): $(IMAGE_LAYER_INPUTS) $(SHYOS_CONFIG_STATE) \
		$(IMAGE_TOOL) $(SHYOS_ROOT)/mk/layer.mk $(SHYOS_ROOT)/mk/config.mk
	$(MAKE) -C $(SHYOS_ROOT)/lib/$(IMAGE_LAYER) $(IMAGE_LAYER_BUILD_TARGET)
	@test -r $(IMAGE_LAYER_FILE)
	touch $@

$(IMAGE_LAYER_FILE): | $(IMAGE_LAYER_STAMP)
	@test -r $@ || $(MAKE) -C $(SHYOS_ROOT)/lib/$(IMAGE_LAYER) $(IMAGE_LAYER_BUILD_TARGET)

$(IMAGE_ELF): $(IMAGE_INPUTS) $(IMAGE_RUST_STATICLIB) \
		$(IMAGE_LAYER_STAMP) $(IMAGE_LAYER_FILE)
	$(CC) $(LDFLAGS) $(IMAGE_ENTRY_FLAGS) -o $@ \
		-Wl,--start-group \
		$(IMAGE_INPUTS) $(IMAGE_RUST_STATICLIB) $(IMAGE_LAYER_FILE) \
		-Wl,--end-group

$(IMAGE_BIN): $(IMAGE_ELF)
	$(OBJCOPY) -O binary $< $@

run: build
ifeq ($(BACKEND),qemu_virt)
	qemu-system-riscv64 -machine virt -m $(QEMU_MEM) -nographic \
		-bios none -kernel $(IMAGE_ELF)
else
	$(IMAGE_ELF)
endif

print-config:
	@echo "SHYOS_CONFIG_FILE=$(SHYOS_CONFIG_FILE)"
	@echo "CONFIG=$(CONFIG)"
	@echo "CARGO_PROFILE=$(CARGO_PROFILE)"
	@echo "IMAGE_NAME=$(IMAGE_NAME)"
	@echo "IMAGE_LAYER=$(IMAGE_LAYER)"
	@echo "IMAGE_RUST_STATICLIB=$(IMAGE_RUST_STATICLIB)"
	@echo "IMAGE_LAYER_FILE=$(IMAGE_LAYER_FILE)"
	@echo "IMAGE_LAYER_STAMP=$(IMAGE_LAYER_STAMP)"
	@echo "IMAGE_LAYER_BUILD_TARGET=$(IMAGE_LAYER_BUILD_TARGET)"
	@echo "CROSS=$(CROSS)"
	@echo "CC=$(CC)"
	@echo "BACKEND=$(BACKEND)"
	@echo "BOARD=$(BOARD)"
	@echo "SHYOS_DEFINES=$(SHYOS_DEFINES)"
	@echo "CFLAGS=$(CFLAGS)"
	@echo "RUST_TARGET=$(RUST_TARGET)"
	@echo "RUSTFLAGS=$(RUSTFLAGS)"
	@echo "LDFLAGS=$(LDFLAGS)"

dag-html:
	python3 $(IMAGE_TOOL) --root $(SHYOS_ROOT) html \
		--defines "$(SHYOS_DEFINES)" \
		--output $(SHYOS_ROOT)/tools/layer_dag.html

clean:
	rm -rf $(IMAGE_OUT_DIR)
