# Layer image projects: each numbered layer is consumed once from C and once
# from Rust.

HOST_PROJECTS := \
	test00_arch_c \
	test01_arch_rust \
	test02_log_c \
	test03_log_rust \
	test04_core_c \
	test05_core_rust \
	test06_basic_c \
	test07_basic_rust \
	test08_simple_alloc_c \
	test11_data_structure_c \
	test12_data_structure_rust \
	test14_spinlock_c

QEMU_PROJECTS := \
	test09_qemu_arch_c \
	test10_qemu_irq_c \
	test13_qemu_trap_uart_c

.PHONY: build run build-host run-host build-qemu run-qemu build-all clean

build: build-host

run: run-host

build-host:
	@for project in $(HOST_PROJECTS); do \
		$(MAKE) -C $$project build || exit $$?; \
	done

run-host:
	@for project in $(HOST_PROJECTS); do \
		$(MAKE) -C $$project run || exit $$?; \
	done

build-qemu:
	@for project in $(QEMU_PROJECTS); do \
		$(MAKE) -C $$project build || exit $$?; \
	done

run-qemu:
	@for project in $(QEMU_PROJECTS); do \
		$(MAKE) -C $$project run || exit $$?; \
	done

build-all: build-host build-qemu

clean:
	@for project in $(HOST_PROJECTS) $(QEMU_PROJECTS); do \
		$(MAKE) -C $$project clean || exit $$?; \
	done
