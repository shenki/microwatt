#include <microwatt_soc.h>

#define BASE_ADDR	BRAM_BASE

SECTIONS
{
	_start = .;
	. = BASE_ADDR;
	.head : {
		KEEP(*(.head))
	}
	. = (BASE_ADDR + 0x1000);
	.text : { *(.text) }
	. = (BASE_ADDR + 0x2000);
	.data : { *(.data) }
	.bss : { *(.bss) }
}
