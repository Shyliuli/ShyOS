# Hashtable C object.

SHYOS_ROOT ?= $(abspath ../../..)
include $(SHYOS_ROOT)/mk/config.mk

OUT_DIR := target
HASHTABLE_C_OBJ := $(OUT_DIR)/hashtable_c.o
HASHSET_C_OBJ := $(OUT_DIR)/hashset_c.o

.PHONY: build clean

build: $(HASHTABLE_C_OBJ) $(HASHSET_C_OBJ)

$(OUT_DIR):
	mkdir -p $@

$(HASHTABLE_C_OBJ): hashtable.c hashtable.h \
		$(SHYOS_ROOT)/lib/03basic/obj/obj.h \
		$(SHYOS_ROOT)/lib/03basic/result/result.h \
		$(SHYOS_ROOT)/lib/04data_structure/rawbuf/rawbuf.h \
		$(SHYOS_CONFIG_STATE) | $(OUT_DIR)
	$(CC) $(CFLAGS) -c -o $@ hashtable.c

$(HASHSET_C_OBJ): hashset.c hashset.h hashtable.h \
		$(SHYOS_ROOT)/lib/03basic/obj/obj.h \
		$(SHYOS_ROOT)/lib/03basic/result/result.h \
		$(SHYOS_ROOT)/lib/04data_structure/rawbuf/rawbuf.h \
		$(SHYOS_CONFIG_STATE) | $(OUT_DIR)
	$(CC) $(CFLAGS) -c -o $@ hashset.c

clean:
	rm -rf $(OUT_DIR)
