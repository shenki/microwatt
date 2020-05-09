SECTIONS
{
	_start = .;
	. = 0xf0000000;
	.head : {
		KEEP(*(.head))
	}
	. = 0xf0001000;
	.text : { *(.text) }
	. = 0xf0002000;
	.data : { *(.data) }
	.bss : { *(.bss) }
}
