#include <stdint.h>
#include <stdbool.h>
#include <io.h>
#include <microwatt_soc.h>

#include "console.h"

extern void load_linux(long dtb);
extern void invalidate_icache(void);

extern int simple_printf(char *fmt, ...);
#define printf(...) simple_printf(__VA_ARGS__)

#define DTB_ADDR	0x01000000L

int main(void)
{
	int i;
	potato_uart_init();

	printf("\r\nMicrowatt Loader (%s %s)\r\n\r\n", __DATE__, __TIME__);

	writeq(SYS_REG_CTRL_DRAM_AT_0, SYSCON_BASE + SYS_REG_CTRL);
	invalidate_icache();

	printf("Load binaries into SDRAM and press 'l' to start:\r\n\r\n");
	printf(" mw_debug -b jtag stop load vmlinux.bin load microwatt.dtb %x start\r\n\r\n",
			DTB_ADDR);

	while (1) {
		if (getchar() == 'l') {
			printf("Loading Linux...\r\n");
			for (i = 0; i < 80; i++)
				putchar('.');
			printf("\r\n");
			load_linux(DTB_ADDR);
		}
	}
}
