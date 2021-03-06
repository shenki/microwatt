#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#include <generated/git.h>

#include "microwatt_soc.h"
#include "io.h"
#include "sdram.h"

/*
 * Core UART functions to implement for a port
 */

static uint64_t potato_uart_base;

#define PROC_FREQ 100000000
#define UART_FREQ 115200

static uint8_t potato_uart_reg_read(int offset)
{
	return readb(potato_uart_base + offset);
}

static void potato_uart_reg_write(int offset, uint8_t val)
{
	writeb(val, potato_uart_base + offset);
}

static bool potato_uart_rx_empty(void)
{
	uint8_t val = potato_uart_reg_read(POTATO_CONSOLE_STATUS);

	return (val & POTATO_CONSOLE_STATUS_RX_EMPTY) != 0;
}

static int potato_uart_tx_full(void)
{
	uint8_t val = potato_uart_reg_read(POTATO_CONSOLE_STATUS);

	return (val & POTATO_CONSOLE_STATUS_TX_FULL) != 0;
}

static char potato_uart_read(void)
{
	return potato_uart_reg_read(POTATO_CONSOLE_RX);
}

static void potato_uart_write(char c)
{
	potato_uart_reg_write(POTATO_CONSOLE_TX, c);
}

static unsigned long potato_uart_divisor(unsigned long proc_freq,
					 unsigned long uart_freq)
{
	return proc_freq / (uart_freq * 16) - 1;
}

void potato_uart_init(void)
{
	potato_uart_base = UART_BASE;

	potato_uart_reg_write(POTATO_CONSOLE_CLOCK_DIV,
			      potato_uart_divisor(PROC_FREQ, UART_FREQ));
}

int getchar(void)
{
	while (potato_uart_rx_empty())
		/* Do nothing */ ;

	return potato_uart_read();
}

int putchar(int c)
{
	while (potato_uart_tx_full())
		/* Do Nothing */;

	potato_uart_write(c);
	return c;
}

void putstr(const char *str, unsigned long len)
{
	for (unsigned long i = 0; i < len; i++) {
		if (str[i] == '\n')
			putchar('\r');
		putchar(str[i]);
	}
}

int _printf(const char *fmt, ...)
{
	int count;
	char buffer[320];
	va_list ap;

	va_start(ap, fmt);
	count = vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);
	putstr(buffer, count);
	return count;
}

void flush_cpu_dcache(void) { }
void flush_cpu_icache(void) { }
void flush_l2_cache(void) { }

void main(void)
{
	unsigned long long ftr, val;
	int i;

	/*
	 * Let things settle ... not sure why but the UART is
	 * not happy otherwise. The PLL might need to settle ?
	 */
	potato_uart_init();
	for (i = 0; i < 100000; i++)
		potato_uart_reg_read(POTATO_CONSOLE_STATUS);
	printf("\n\nWelcome to Microwatt !\n\n");

	/* TODO: Add core version information somewhere in syscon, possibly
	 *       extracted from git
	 */
	printf(" Soc signature: %016llx\n",
	       (unsigned long long)readq(SYSCON_BASE + SYS_REG_SIGNATURE));
	printf("  Soc features: ");
	ftr = readq(SYSCON_BASE + SYS_REG_INFO);
	if (ftr & SYS_REG_INFO_HAS_UART)
		printf("UART ");
	if (ftr & SYS_REG_INFO_HAS_DRAM)
		printf("DRAM ");
	printf("\n");
	val = readq(SYSCON_BASE + SYS_REG_BRAMINFO);
	printf("          BRAM: %lld KB\n", val / 1024);
	if (ftr & SYS_REG_INFO_HAS_DRAM) {
		val = readq(SYSCON_BASE + SYS_REG_DRAMINFO);
		printf("          DRAM: %lld MB\n", val / (1024 * 1024));
	}
	val = readq(SYSCON_BASE + SYS_REG_CLKINFO);
	printf("           CLK: %lld MHz\n", val / 1000000);

	printf("\n");
	if (ftr & SYS_REG_INFO_HAS_DRAM) {
		printf("LiteDRAM built from Migen %s and LiteX %s\n",
		       MIGEN_GIT_SHA1, LITEX_GIT_SHA1);
		sdrinit();
	}
	printf("Booting from BRAM...\n");
}
