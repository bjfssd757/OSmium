CC      := x86_64-elf-gcc
LD      := x86_64-elf-ld
AS      := nasm
QEMU    := qemu-system-x86_64

LIMINE_DIR := limine

BASE_CFLAGS := -m64 -ffreestanding -Wall -Wextra -Isrc -nostdlib -g -mno-red-zone -mcmodel=kernel
DEBUG_CFLAGS := -O0 -DDEBUG

LDFLAGS  := -m elf_x86_64 -T link.ld -z noexecstack -static -z max-page-size=0x1000

ASMFLAGS       := -f elf64
ASMFLAGS_DEBUG := -g -F dwarf

SRCS_AS := $(shell find . -path "./limine" -prune -o -name "*.asm" -print)
SRCS_C  := $(shell find src -path src/user -prune -o -name '*.c' -print)

ASM_OBJS := $(patsubst %.asm,build/%.asm.o,$(SRCS_AS))
C_OBJS   := $(patsubst %.c,build/%.c.o,$(SRCS_C))

OBJECTS  := $(ASM_OBJS) $(C_OBJS)

BUILD_KERNEL := build/kernel.elf
IMAGE_ISO    := build/myos.iso
QEMU_OPTS    := -serial stdio -m 2G

.PHONY: all clean builddir run debug limine_setup

all: builddir $(IMAGE_ISO)

builddir:
	@mkdir -p build iso_root

build/%.asm.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASMFLAGS) $< -o $@

build/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(BASE_CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@

$(BUILD_KERNEL): $(OBJECTS) link.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

$(IMAGE_ISO): $(BUILD_KERNEL) limine.conf
	@mkdir -p iso_root
	cp $(BUILD_KERNEL) limine.conf iso_root/
	cp $(LIMINE_DIR)/limine-bios.sys \
	   $(LIMINE_DIR)/limine-bios-cd.bin \
	   $(LIMINE_DIR)/limine-uefi-cd.bin iso_root/
	
	xorriso -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_ISO)

	./$(LIMINE_DIR)/limine bios-install $(IMAGE_ISO)


fs: all
	$(QEMU) -cdrom $(IMAGE_ISO) $(QEMU_OPTS) -drive file=disk.img,format=raw

run: all
	$(QEMU) -cdrom $(IMAGE_ISO) $(QEMU_OPTS)

kvm: all
	$(QEMU) -cdrom $(IMAGE_ISO) $(QEMU_OPTS) -enable-kvm

debug: EXTRA_CFLAGS += $(DEBUG_CFLAGS)
debug: ASMFLAGS += $(ASMFLAGS_DEBUG)
debug: all
	$(QEMU) -cdrom $(IMAGE_ISO) $(QEMU_OPTS) -d guest_errors,int,in_asm,exec -D qemu.log -no-reboot -action panic=pause

gdb: all
	$(QEMU) -cdrom $(IMAGE_ISO) -s -S $(QEMU_OPTS) -d guest_errors,int,in_asm,exec -D qemu.log -no-reboot

log: all
	$(QEMU) -cdrom $(IMAGE_ISO) $(QEMU_OPTS) -d in_asm,exec,cpu,guest_errors,int,mmu -D qemu.log

dump: all
	$(QEMU) -cdrom $(IMAGE_ISO) $(QEMU_OPTS) -d int,cpu_reset,guest_errors -mem-verify -D qemu.log

clean:
	rm -rf build iso_root $(IMAGE_ISO)