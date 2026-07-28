# Top-level project dispatcher. Final images own their shyos.mk files and
# recursively build the selected ShyOS layer closure.

SHYOS_ROOT := $(abspath .)
APP_NAME ?= 2048
APP_DIR ?= $(SHYOS_ROOT)/app/$(APP_NAME)
APP_CONFIG := $(APP_DIR)/shyos.mk
KERN_DIR ?= $(SHYOS_ROOT)/kernel
KERN_CONFIG := $(KERN_DIR)/shyos.mk
LAYER_TOOL := $(SHYOS_ROOT)/tools/layer_graph.py
NEW_APP_TOOL := $(SHYOS_ROOT)/tools/new_app.py
IMAGE_LAYERS := $(shell python3 $(LAYER_TOOL) --root $(SHYOS_ROOT) list --image)

.NOTPARALLEL: all

.PHONY: all app kernel lib clean run-app run-kernel run-app-gdb run-kernel-gdb print-config \
	print-kernel-config check-layers dag dag-html analyze host-test \
	image-test run-image-test qemu-image-test vscode vscode-settings \
	vscode-compile-commands new-app $(addprefix layer-,$(IMAGE_LAYERS))

all: app kernel

app:
	$(MAKE) -C $(APP_DIR) build

kernel:
	$(MAKE) -C $(KERN_DIR) build

run-app:
	$(MAKE) -C $(APP_DIR) run

run-kernel:
	$(MAKE) -C $(KERN_DIR) run

run-app-gdb:
	$(MAKE) -C $(APP_DIR) run CONFIG=GDB SHYOS_GDB=1

run-kernel-gdb:
	$(MAKE) -C $(KERN_DIR) run CONFIG=GDB SHYOS_GDB=1

print-config:
	$(MAKE) -C $(APP_DIR) print-config

print-kernel-config:
	$(MAKE) -C $(KERN_DIR) print-config

check-layers:
	python3 $(LAYER_TOOL) --root $(SHYOS_ROOT) check

dag: check-layers
	@python3 $(LAYER_TOOL) --root $(SHYOS_ROOT) module-graph

dag-html:
	$(MAKE) -C $(APP_DIR) dag-html

define layer_rule
layer-$(1): check-layers
	$(MAKE) -C $(SHYOS_ROOT)/lib/$(1) build \
		SHYOS_CONFIG=$(APP_CONFIG)
endef
$(foreach layer,$(IMAGE_LAYERS),$(eval $(call layer_rule,$(layer))))

lib:
	$(MAKE) -C $(APP_DIR) layer

vscode:
	$(MAKE) -C $(APP_DIR) vscode

vscode-settings:
	$(MAKE) -C $(APP_DIR) vscode-settings

vscode-compile-commands:
	$(MAKE) -C $(APP_DIR) vscode-compile-commands

analyze:
	$(MAKE) -C $(APP_DIR) analyze

new-app:
	@test -n "$(APP)" || { \
		echo "usage: make new-app APP=app/<name>"; \
		exit 1; \
	}
	python3 $(NEW_APP_TOOL) --root $(SHYOS_ROOT) $(APP)

host-test:
	$(MAKE) -C $(SHYOS_ROOT)/lib/02core/string_no_alloc host-test \
		SHYOS_CONFIG=$(SHYOS_ROOT)/test/test08_simple_alloc_c/shyos.mk
	$(MAKE) -C $(SHYOS_ROOT)/test/test08_simple_alloc_c run

image-test:
	$(MAKE) -C $(SHYOS_ROOT)/test build-host

run-image-test:
	$(MAKE) -C $(SHYOS_ROOT)/test run-host

qemu-image-test:
	$(MAKE) -C $(SHYOS_ROOT)/test run-qemu

clean:
	$(MAKE) -C $(APP_DIR) clean
	$(MAKE) -C $(KERN_DIR) clean
	$(MAKE) -C $(SHYOS_ROOT)/test clean
	@for layer in $(IMAGE_LAYERS); do \
		$(MAKE) -C $(SHYOS_ROOT)/lib/$$layer clean \
			SHYOS_CONFIG=$(APP_CONFIG) || exit $$?; \
	done
	rm -rf $(SHYOS_ROOT)/target
