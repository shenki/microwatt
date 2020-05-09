ARCH = $(shell uname -m)
ifneq ("$(ARCH)", "ppc64")
ifneq ("$(ARCH)", "ppc64le")
	CROSS_COMPILE ?= powerpc64le-linux-gnu-
endif
endif

PYTHON3 ?= python3
MW_DEBUG ?= mw_debug
BRAM_ADDRESS ?= 0xf0000000

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

CFLAGS = -Os -g -Wall -std=c99 -msoft-float -mno-string -mno-multiple \
	 -mno-vsx -mno-altivec -mlittle-endian -fno-stack-protector \
	 -mstrict-align -ffreestanding -fdata-sections -ffunction-sections \
	 -I../include
ASFLAGS = $(CFLAGS)
LDFLAGS = -T powerpc.lds --gc-sections

all: loader.hex

load: loader.bin
	$(MW_DEBUG) -b jtag load $^ $(BRAM_ADDRESS)

loader.elf: loader.o head.o console.o load.o printf.o
	$(LD) $(LDFLAGS) -o $@ $^

loader.bin: loader.elf
	$(OBJCOPY) -O binary $^ $@

loader.hex: loader.bin
	$(PYTHON3) ../scripts/bin2hex.py $^ > $@

clean:
	@rm -f *.o loader.elf loader.bin loader.hex
