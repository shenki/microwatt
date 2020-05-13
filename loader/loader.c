#include <stdint.h>
#include <stdbool.h>
#include <io.h>
#include <microwatt_soc.h>

#include "console.h"

extern void load_linux(unsigned long kernel, unsigned long dtb);
extern void invalidate_icache(void);

extern int simple_printf(char *fmt, ...);
#define printf(...) simple_printf(__VA_ARGS__)

#define DTB_ADDR	0x01000000L

#define DTBIMAGE_ADDR 	0x500000L

int main(void)
{
	int i;
	potato_uart_init();
	unsigned long kernel, dtb;

	printf("\r\nMicrowatt Loader (%s %s)\r\n\r\n", __DATE__, __TIME__);

	writeq(SYS_REG_CTRL_DRAM_AT_0, SYSCON_BASE + SYS_REG_CTRL);
	invalidate_icache();

	printf("Load binaries into SDRAM and select option to start:\r\n\r\n");
	printf("vmlinux.bin and dtb:\r\n");
	printf(" mw_debug -b jtag stop load vmlinux.bin load microwatt.dtb %x start\r\n",
			DTB_ADDR);
	printf(" press 'l' to start'\r\n\r\n");

	printf("dtbImage.microwatt:\r\n");
	printf(" mw_debug -b jtag stop load dtbImage.microwatt %x start\r\n",
			DTBIMAGE_ADDR);
	printf(" press 'w' to start'\r\n\r\n");

	while (1) {
		switch (getchar()) {
		case 'l':
			kernel = 0;
			dtb = DTB_ADDR;
			goto load;
		case 'w':
			kernel = DTBIMAGE_ADDR;
			dtb = -1;
			goto load;
		default:
			continue;
		}
	}

load:
	printf("Loading Linux at %08x...\r\n", 0x0);
	for (i = 0; i < 80; i++)
		putchar('.');
	printf("\r\n");
	load_linux(kernel, dtb);

	while (1);
}
