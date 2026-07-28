SHYOS_ROOT ?= $(abspath ../..)
include $(SHYOS_ROOT)/mk/config.mk

LAYER := 05task
OUT_DIR := $(SHYOS_TARGET_DIR)

TIMER_DIR := timer
TIMER_C_OBJ := $(TIMER_DIR)/$(SHYOS_TARGET_DIR)/timer_c.o

LAYER_ARCHIVE := $(OUT_DIR)/libshyos_$(LAYER).a
LAYER_RUST_LIB := shyos_$(LAYER)_rust
LAYER_OBJS := \
	$(TIMER_C_OBJ) \

.PHONY: clean

$(OUT_DIR):
	mkdir -p $@

$(LAYER_ARCHIVE): $(LAYER_OBJS) $(SHYOS_CONFIG_STATE) | $(OUT_DIR)
	rm -f $@
	$(AR) rcs $@ $(LAYER_OBJS)

clean:
	rm -rf target
	$(MAKE) -C $$(TIMER_DIR) clean
	
include $(SHYOS_ROOT)/mk/layer.mk
