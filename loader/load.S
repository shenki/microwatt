/*
 * Set up system for jumping to Linux.
 *
 * load_linux(kernel, dtb)
 */
.global load_linux
load_linux:
	/* ABI:
	 *  r5: zero (no openfirmware)
	 *  r3: device tree
	 */
	addi	%r5,0,0
	addi	%r12,%r3,0
	addi	%r3,%r4,0
	mtctr	%r12
	bctrl
	b .

/*
 * Invalidate the icache.
 */
.global invalidate_icache
invalidate_icache:
        icbi    0,0
	isync
	blr
