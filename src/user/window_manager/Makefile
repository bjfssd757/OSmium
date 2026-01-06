SRC  := $(shell find src -path src/user -prune -o -name '*.c' -print)
LINKER := linker.ld
PROG := window_manager

ifeq ($(wildcard $(SRC)),)
$(error main.c not found in this directory)
endif
ifeq ($(wildcard $(LINKER)),)
$(error linker.ld not found in this directory)
endif

CC := gcc
LD := ld
OBJCOPY := objcopy
XXD := xxd

CFLAGS := -m64 -c -ffreestanding -fno-builtin -nostdlib
LDFLAGS := -m elf_x86_64 -T $(LINKER)

.PHONY: all clean

all: $(PROG).elf

$(PROG).o: $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

$(PROG).elf: $(PROG).o $(LINKER)
	$(LD) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(PROG).o $(PROG).elf