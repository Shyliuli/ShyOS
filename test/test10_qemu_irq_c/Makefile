SHYOS_ROOT ?= $(abspath ../..)
SHYOS_CONFIG ?= $(CURDIR)/shyos.mk
TEST_NAME := test10_qemu_irq_c
TEST_LANGUAGE := c
TEST_EXTRA_OBJS := target/trap.o

include ../image.mk

target/trap.o: trap.S $(SHYOS_CONFIG_STATE) | target
	$(AS) $(ASFLAGS) -c -o $@ $<
