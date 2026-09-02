ARCH ?= i386
.DEFAULT_GOAL := all
BUILD_DIR := build/$(ARCH)
ISO_DIR := iso
KERNEL := $(BUILD_DIR)/nanos.elf
ISO := build/nanos-$(ARCH).iso
MILO_BUILD_DIR := build/M.I.L.O
MILO_STAGE1_OBJECT := $(MILO_BUILD_DIR)/stage1.o
MILO_STAGE1 := $(MILO_BUILD_DIR)/stage1.bin
MILO_STAGE2_OBJECT := $(MILO_BUILD_DIR)/stage2.o
MILO_STAGE2 := $(MILO_BUILD_DIR)/stage2.bin
MILO_STAGE2_MAX_BYTES := 8192
MILO_KERNEL_OBJECT := $(MILO_BUILD_DIR)/kernel.o
MILO_KERNEL := $(MILO_BUILD_DIR)/kernel.bin
MILO_KERNEL_MAX_BYTES := 49152
MILO_SPLASH_SOURCE := boot/milo/assets/milo-splash.ans
MILO_SPLASH_TOOL := tools/build_milo_splash.py
MILO_SPLASH := $(MILO_BUILD_DIR)/splash.bin
MILO_FILES_DIR := boot/milo/files
MILO_SAMPLE_IMAGE_TOOL := tools/build_milo_m16.py
MILO_SAMPLE_IMAGE := $(MILO_FILES_DIR)/MILO.M16
MILO_FILES := $(wildcard $(MILO_FILES_DIR)/*)
MILO_FAT12_TOOL := tools/build_milo_fat12.py
MILO_FAT12_ARGS := --place=FARTEST.TXT=130
MILO_ROUTING_TEST := tools/test_milo_routing_contract.py
MILO_STORAGE_TEST := tools/test_milo_storage_contract.py
MILO_SHELL_TEST := tools/test_milo_shell_contract.py
MILO_VERSION ?= 0.30.2
MILO_FLOPPY := build/M.I.L.O-floppy-V$(MILO_VERSION).img
PYTHON ?= python3
INITRD_DIR := initrd
INITRD := $(BUILD_DIR)/initrd.tar
INITRD_FILES := $(shell find $(INITRD_DIR) -type f 2>/dev/null)
INITRD_STAGE := $(BUILD_DIR)/initrd-root
USERSPACE_DIR := userspace
USERSPACE_SOURCES := $(wildcard $(USERSPACE_DIR)/*.S)
USERSPACE_OBJECTS := $(patsubst $(USERSPACE_DIR)/%.S,$(BUILD_DIR)/userspace/%.o,$(USERSPACE_SOURCES))
USERSPACE_APPS := $(patsubst $(USERSPACE_DIR)/%.S,$(BUILD_DIR)/userspace/%.nx,$(USERSPACE_SOURCES))
NANOS_BOOT_SELFTESTS ?= 0

CC ?= gcc
LD ?= ld
OBJCOPY ?= objcopy
NM ?= nm
QEMU_I386 ?= qemu-system-i386
QEMU_X86_64 ?= qemu-system-x86_64
GRUB_MKRESCUE ?= grub-mkrescue
GRUB_MKSTANDALONE ?= grub-mkstandalone
XORRISO ?= xorriso
TAR ?= tar

CFLAGS_COMMON := -std=c89 -Wall -Wextra -Werror -ffreestanding -fno-builtin \
	-fno-stack-protector -fno-pic -fno-pie -fno-asynchronous-unwind-tables \
	-fno-unwind-tables -nostdinc \
	-DNANOS_BOOT_SELFTESTS=$(NANOS_BOOT_SELFTESTS) \
	-Iabi/include -Ikernel/include -Iarch/x86/include

ifeq ($(ARCH),i386)
CFLAGS_ARCH := -m32 -march=i386
LDFLAGS_ARCH := -m elf_i386
QEMU := $(QEMU_I386)
else ifeq ($(ARCH),x86_64)
$(error x86_64 is planned but not bootable yet; build ARCH=i386 for now)
else
$(error Unsupported ARCH=$(ARCH))
endif

CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_ARCH)
ASFLAGS := $(CFLAGS_ARCH) -ffreestanding -fno-asynchronous-unwind-tables \
	-fno-unwind-tables -Iabi/include -Ikernel/include -Iarch/x86/include \
	-Iuserspace/include
DEPFLAGS := -MMD -MP
LDFLAGS := $(LDFLAGS_ARCH) -T boot/linker-$(ARCH).ld -nostdlib

C_SOURCES := \
	arch/x86/src/cpu.c \
	arch/x86/src/gdt.c \
	arch/x86/src/idt.c \
	arch/x86/src/isr.c \
	arch/x86/src/keyboard.c \
	arch/x86/src/mouse.c \
	arch/x86/src/paging.c \
	arch/x86/src/platform.c \
	arch/x86/src/pic.c \
	arch/x86/src/pit.c \
	arch/x86/src/ports.c \
	arch/x86/src/scheduler.c \
	arch/x86/src/serial.c \
	arch/x86/src/syscall.c \
	arch/x86/src/user.c \
	arch/x86/src/vga_text.c \
	kernel/src/block.c \
	kernel/src/cursor.c \
	kernel/src/desktop/desktop.c \
	kernel/src/desktop/event_router.c \
	kernel/src/desktop/move_resize.c \
	kernel/src/fb_console.c \
	kernel/src/framebuffer.c \
	kernel/src/gui/app_button.c \
	kernel/src/gui/compositor.c \
	kernel/src/gui/font.c \
	kernel/src/gui/visibility.c \
	kernel/src/gui/window.c \
	kernel/src/heap.c \
	kernel/src/init.c \
	kernel/src/input.c \
	kernel/src/kmain.c \
	kernel/src/memory.c \
	kernel/src/nx_loader.c \
	kernel/src/debug/monitor.c \
	kernel/src/debug/monitor_commands.c \
	kernel/src/debug/process.c \
	kernel/src/debug/selftest.c \
	kernel/src/panic.c \
	kernel/src/pmm.c \
	kernel/src/print.c \
	kernel/src/process.c \
	kernel/src/process/events.c \
	kernel/src/process/scheduler.c \
	kernel/src/ramdisk.c \
	kernel/src/shell/panel.c \
	kernel/src/string.c \
	kernel/src/syscall.c \
	kernel/src/syscall/app.c \
	kernel/src/syscall/event.c \
	kernel/src/syscall/file.c \
	kernel/src/syscall/gui.c \
	kernel/src/syscall/ipc.c \
	kernel/src/syscall/util.c \
	kernel/src/tarfs.c \
	kernel/src/time.c \
	kernel/src/vfs.c

ASM_SOURCES := \
	arch/x86/src/gdt_flush.S \
	arch/x86/src/interrupts.S \
	arch/x86/src/user_switch.S \
	boot/$(ARCH)/start.S
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES)) \
	$(patsubst %.S,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
DEPS := $(OBJECTS:.o=.d) $(USERSPACE_OBJECTS:.o=.d)

-include $(DEPS)

.PHONY: all clean iso run run64 bochs check-tools check-iso-tools verify \
	milo-boot milo-floppy milo-install-hooks verify-milo-boot \
	verify-milo-routing FORCE

all: $(KERNEL)

iso: $(ISO)

verify: $(KERNEL)
	grub-file --is-x86-multiboot2 $(KERNEL)
	readelf -h $(KERNEL) | sed -n '1,16p'

milo-boot: $(MILO_STAGE1) $(MILO_STAGE2) $(MILO_KERNEL)

milo-floppy: $(MILO_FLOPPY)

milo-install-hooks:
	git config core.hooksPath .githooks

verify-milo-boot: $(MILO_STAGE1) $(MILO_STAGE2) $(MILO_KERNEL) $(MILO_FLOPPY)
	test "$$(wc -c < $(MILO_STAGE1))" -eq 512
	test "$$(od -An -tx1 -j510 -N2 $(MILO_STAGE1) | tr -d ' \n')" = "55aa"
	test "$$(wc -c < $(MILO_STAGE2))" -le $(MILO_STAGE2_MAX_BYTES)
	test "$$(wc -c < $(MILO_KERNEL))" -le $(MILO_KERNEL_MAX_BYTES)
	test -z "$$($(NM) -u $(MILO_KERNEL_OBJECT))"
	$(PYTHON) $(MILO_ROUTING_TEST)
	$(PYTHON) $(MILO_STORAGE_TEST) $(MILO_FLOPPY) $(MILO_FILES_DIR) $(MILO_KERNEL) \
		boot/milo/stage2.S
	$(PYTHON) $(MILO_SHELL_TEST) boot/milo/kernel.S boot/milo/shell.inc \
		boot/milo/mouse.inc $(MILO_KERNEL)

verify-milo-routing:
	$(PYTHON) $(MILO_ROUTING_TEST)

run: $(ISO)
	$(QEMU) -cdrom $(ISO) -serial stdio -m 128M

run64: $(ISO)
	$(QEMU_X86_64) -cdrom $(ISO) -serial stdio -m 128M

bochs: $(ISO)
	bochs -q -f bochsrc

check-tools:
	@command -v $(CC) >/dev/null
	@command -v $(LD) >/dev/null
	@command -v $(OBJCOPY) >/dev/null

check-iso-tools: check-tools
	@command -v $(GRUB_MKSTANDALONE) >/dev/null
	@command -v $(XORRISO) >/dev/null
	@command -v $(TAR) >/dev/null

$(MILO_STAGE1): boot/milo/stage1.S
	@mkdir -p $(@D)
	$(CC) -m32 -ffreestanding -c $< -o $(MILO_STAGE1_OBJECT)
	$(OBJCOPY) -O binary -j .text $(MILO_STAGE1_OBJECT) $@
	@test "$$(wc -c < $@)" -eq 512

$(MILO_SPLASH): $(MILO_SPLASH_SOURCE) $(MILO_SPLASH_TOOL)
	@mkdir -p $(@D)
	$(PYTHON) $(MILO_SPLASH_TOOL) $(MILO_SPLASH_SOURCE) $@

$(MILO_SAMPLE_IMAGE): $(MILO_SAMPLE_IMAGE_TOOL)
	$(PYTHON) $(MILO_SAMPLE_IMAGE_TOOL) $@

$(MILO_STAGE2): boot/milo/stage2.S $(MILO_SPLASH)
	@mkdir -p $(@D)
	$(CC) -m32 -ffreestanding -c $< -o $(MILO_STAGE2_OBJECT)
	$(OBJCOPY) -O binary -j .text $(MILO_STAGE2_OBJECT) $@
	@test "$$(wc -c < $@)" -le $(MILO_STAGE2_MAX_BYTES)

$(MILO_KERNEL): boot/milo/kernel.S boot/milo/mouse.inc boot/milo/shell.inc
	@mkdir -p $(@D)
	$(CC) -m32 -ffreestanding -c $< -o $(MILO_KERNEL_OBJECT)
	$(OBJCOPY) -O binary -j .text $(MILO_KERNEL_OBJECT) $@
	@test "$$(wc -c < $@)" -le $(MILO_KERNEL_MAX_BYTES)

$(MILO_FLOPPY): $(MILO_STAGE1) $(MILO_STAGE2) $(MILO_KERNEL) \
		$(MILO_FAT12_TOOL) $(MILO_FILES)
	@mkdir -p $(@D)
	dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	dd if=$(MILO_STAGE1) of=$@ conv=notrunc status=none
	dd if=$(MILO_STAGE2) of=$@ bs=512 seek=1 conv=notrunc status=none
	dd if=$(MILO_KERNEL) of=$@ bs=512 seek=18 conv=notrunc status=none
	$(PYTHON) $(MILO_FAT12_TOOL) $@ $(MILO_FILES_DIR) $(MILO_FAT12_ARGS)

$(ISO): $(KERNEL) grub/standalone.cfg $(INITRD) | check-iso-tools
	@rm -rf $(BUILD_DIR)/standalone
	@mkdir -p $(BUILD_DIR)/standalone/boot/grub/i386-pc build
	$(GRUB_MKSTANDALONE) -O i386-pc-eltorito \
		--compress=xz --core-compress=xz --fonts= --locales= --themes= \
		--install-modules='normal multiboot2 relocator boot configfile memdisk tar' \
		-o $(BUILD_DIR)/standalone/boot/grub/i386-pc/eltorito.img \
		"/boot/grub/grub.cfg=grub/standalone.cfg" \
		"/boot/initrd.bin=$(INITRD)" \
		"/boot/nanos.elf=$(KERNEL)"
	$(XORRISO) -as mkisofs -R \
		-b boot/grub/i386-pc/eltorito.img \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		-o $(ISO) $(BUILD_DIR)/standalone

$(INITRD): $(INITRD_FILES) $(USERSPACE_APPS) FORCE
	@rm -rf $(INITRD_STAGE)
	@mkdir -p $(INITRD_STAGE)
	cp -R $(INITRD_DIR)/. $(INITRD_STAGE)/
	@mkdir -p $(INITRD_STAGE)/bin
	cp $(USERSPACE_APPS) $(INITRD_STAGE)/bin/
	@mkdir -p $(@D)
	$(TAR) --format=ustar --owner=0 --group=0 --numeric-owner -C $(INITRD_STAGE) -cf $@ .

$(BUILD_DIR)/userspace/%.o: $(USERSPACE_DIR)/%.S
	@mkdir -p $(@D)
	$(CC) $(ASFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/userspace/%.nx: $(BUILD_DIR)/userspace/%.o
	$(OBJCOPY) -O binary -j .text $< $@

$(KERNEL): $(OBJECTS) boot/linker-$(ARCH).ld
	@mkdir -p $(@D)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(ASFLAGS) $(DEPFLAGS) -c $< -o $@

clean:
	rm -rf build iso/boot/nanos.elf iso/boot/grub/grub.cfg

FORCE:
