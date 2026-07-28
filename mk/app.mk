# Shared app tooling layered on top of the common final-image rules.

VSCODE_TOOL := $(SHYOS_ROOT)/tools/vscode_config.py
COMPILE_COMMANDS := $(SHYOS_ROOT)/compile_commands.json
BEAR ?= bear
CC_PATH := $(shell command -v $(CC) 2>/dev/null || printf '%s' "$(CC)")
CLANG ?= clang

ANALYZE_OUT_DIR := $(SHYOS_ROOT)/target/analyze/$(notdir $(CURDIR))
APP_ANALYZE_SOURCES ?= main.c
ANALYZE_CFLAGS := $(filter-out -nostartfiles,$(CFLAGS))
ifeq ($(BACKEND),qemu_virt)
ANALYZE_TARGET := --target=riscv64-unknown-elf
endif

.PHONY: vscode vscode-settings vscode-compile-commands analyze

vscode: vscode-settings vscode-compile-commands
	@echo "VS Code config generated for $(BACKEND): $(COMPILE_COMMANDS)"

vscode-settings:
	python3 $(VSCODE_TOOL) \
		--root $(SHYOS_ROOT) \
		--c-compiler "$(CC_PATH)" \
		--rust-target "$(RUST_TARGET)" \
		--rustflags "$(RUSTFLAGS)" \
		--defines "$(SHYOS_DEFINES)" \
		--kernel-elf "kernel/target/kernel.elf" \
		--app-elf "$(shell realpath --relative-to=$(SHYOS_ROOT) $(CURDIR))/target/$(IMAGE_NAME).elf"

vscode-compile-commands:
	@command -v $(BEAR) >/dev/null 2>&1 || { \
		echo "bear not found; install Bear or set BEAR=<path>"; \
		exit 1; \
	}
	$(BEAR) --output $(COMPILE_COMMANDS) -- $(MAKE) -B build

analyze:
	@mkdir -p $(ANALYZE_OUT_DIR)
	@for source in $(APP_ANALYZE_SOURCES); do \
		name=$$(basename "$$source" .c); \
		$(CLANG) --analyze $(ANALYZE_TARGET) $(ANALYZE_CFLAGS) \
			-o "$(ANALYZE_OUT_DIR)/$$name.plist" "$$source" || exit $$?; \
	done

include $(SHYOS_ROOT)/mk/image.mk
