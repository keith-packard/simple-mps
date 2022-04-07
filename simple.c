/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2022 Keith Packard
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdint.h>

typedef volatile uint32_t vuint32_t;

struct uart {
	vuint32_t	data;
	vuint32_t	state;
	vuint32_t	control;
	vuint32_t	isr;

	vuint32_t	bauddiv;
};

/* state */
#define UART_STATE_TX_BF	0
#define UART_STATE_RX_BF	1
#define UART_STATE_TX_B_OV	2
#define UART_STATE_RX_B_OV	3

/* control */
#define UART_CONTROL_TX_EN	0
#define UART_CONTROL_RX_EN	1
#define UART_CONTROL_TX_IN_EN	2
#define UART_CONTROL_RX_IN_EN	3
#define UART_CONTROL_TX_OV_EN	4
#define UART_CONTROL_RX_OV_EN	5
#define UART_CONTROL_HS_TM_TX	6

/* isr */
#define UART_ISR_TX_IN		0
#define UART_ISR_RX_IN		1
#define UART_ISR_TX_OV_IN	2
#define UART_ISR_RX_OV_IN	3

#define CLOCK	25000000

extern struct uart uart0;

static void
uart_init(int baud)
{
	/* Set baud rate */
	uart0.bauddiv = CLOCK / baud;

	/* Enable receiver and transmitter */
	uart0.control = ((1 << UART_CONTROL_RX_EN) |
			 (1 << UART_CONTROL_TX_EN));
}

static int
uart_putc(char c, FILE *file)
{
	(void) file;

	/* Translate newline into cr/lf */
	if (c == '\n')
		uart_putc('\r', file);

	/* Wait for transmitter */
	while ((uart0.state & (1 << UART_STATE_TX_BF)) != 0)
		;

	/* Send character */
	uart0.data = c;

	/* Return character */
	return (int) (uint8_t) c;
}

static int
uart_getc(FILE *file)
{
	uint8_t	c;

	/* Wait for receiver */
	while ((uart0.state & (1 << UART_STATE_RX_BF)) == 0)
		;
	/* Read input */
	c = uart0.data;

	/* Translate return into newline */
	if (c == '\r')
		c = '\n';

	/* Echo */
	uart_putc(c, stdout);
	return (int) c;
}

/* Create a single FILE object for stdin and stdout */
static FILE __stdio = FDEV_SETUP_STREAM(uart_putc, uart_getc, NULL, _FDEV_SETUP_RW);

/*
 * Use the picolibc __strong_reference macro to share one variable for
 * stdin/stdout/stderr. This saves space, but prevents the application
 * from updating them independently.
 */

#ifdef __strong_reference
#define STDIO_ALIAS(x) __strong_reference(stdin, x);
#else
#define STDIO_ALIAS(x) FILE *const x = &__stdio;
#endif

FILE *const stdin = &__stdio;
STDIO_ALIAS(stdout);
STDIO_ALIAS(stderr);

int main(void)
{
	static char buf[512];

	/* Initialize the UART */
	uart_init(115200);

	/* Loop forever sending and receiving data */
	for (;;) {
		printf("What is your name? ");
		fgets(buf, sizeof(buf), stdin);
		printf("Good to meet you, %s", buf);
	}
}
