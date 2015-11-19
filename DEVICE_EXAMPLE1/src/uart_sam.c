/**
 * \file
 *
 * \brief UART functions
 *
 * Copyright (c) 2012-2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#include <asf.h>
#include "conf_example.h"
#include "uart_sam.h"
#include "main.h"
#include "ui.h"
#if (SAMG55)
#include "flexcom.h"
#endif

#if SAM4L
#   define USART_PERIPH_CLK_ENABLE() sysclk_enable_peripheral_clock(USART_BASE)
#elif SAMG55
#   define USART_PERIPH_CLK_ENABLE() flexcom_enable(BOARD_FLEXCOM);      \
	                                                        flexcom_set_opmode(BOARD_FLEXCOM, FLEXCOM_USART);
#else
#   define USART_PERIPH_CLK_ENABLE() sysclk_enable_peripheral_clock(USART_ID)
#endif

static sam_usart_opt_t usart_options;
#define PRINTK_BUFFER_SIZE	(1024*8)
int printk_buffer_count = 0;
int printk_buffer_in_index = 0; 
int printk_buffer_out_index = 0;

int rx_ready_flag = 0;
char printk_buff[PRINTK_BUFFER_SIZE];

int printk_outbuf_putc(Usart *p_usart, unsigned char ch)
{
	if(printk_buffer_count >= PRINTK_BUFFER_SIZE)
		return 1;
	printk_buff[printk_buffer_out_index] = ch;
	printk_buffer_count++;
	printk_buffer_count &= PRINTK_BUFFER_SIZE-1;
	printk_buffer_out_index++;
	printk_buffer_out_index &= PRINTK_BUFFER_SIZE-1;

	// Enable UART TX interrupt to send a new value
//	usart_enable_tx(p_usart);
	usart_enable_interrupt(p_usart, US_IER_TXRDY);
	return 1;
}

int printk_outbuf_getc(int *ch)
{
	if(0 == printk_buffer_count)	{
		return 0;
	}
	else	{		
		*ch = printk_buff[printk_buffer_in_index];	
		printk_buffer_in_index++;
		printk_buffer_in_index &= PRINTK_BUFFER_SIZE-1;
		printk_buffer_count--;
		return 1;
	}
}

ISR(USART_HANDLER)
{
	uint32_t sr = usart_get_status(USART_BASE);
#if 1		
	if (sr & US_CSR_TXRDY) {
		int c;
		if(printk_outbuf_getc(&c) > 0)	{
			usart_write(USART_BASE, c);
		}
		else {
//			usart_disable_tx(USART_BASE);
			usart_disable_interrupt(USART_BASE, US_IDR_TXRDY);
		}
	}
#else		
	if (sr & US_CSR_RXRDY) {
		// Data received
		ui_com_tx_start();
		uint32_t value;
		bool b_error = usart_read(USART_BASE, &value) ||
			(sr & (US_CSR_FRAME | US_CSR_TIMEOUT | US_CSR_PARE));
		if (b_error) {
			usart_reset_rx(USART_BASE);
			usart_enable_rx(USART_BASE);
			udi_cdc_signal_framing_error();
			ui_com_error();
		}
		// Transfer UART RX fifo to CDC TX
		if (!udi_cdc_is_tx_ready()) {
			// Fifo full
			udi_cdc_signal_overrun();
			ui_com_overflow();
		} else {
			udi_cdc_putc(value);
		}
		ui_com_tx_stop();
		return;
	}

	if (sr & US_CSR_TXRDY) {
		// Data send
		if (udi_cdc_is_rx_ready()) {
			// Transmit next data
			ui_com_rx_start();
			int c = udi_cdc_getc();
			usart_write(USART_BASE, c);
			
		} else {
			// Fifo empty then Stop UART transmission
			usart_disable_tx(USART_BASE);
			usart_disable_interrupt(USART_BASE, US_IDR_TXRDY);
			ui_com_rx_stop();
		}
	}
#endif			
}

void uart_clean_rx_ready(uint8_t port)
{
	UNUSED(port);
	rx_ready_flag= 0;
}

int uart_rx_is_ready()
{
	int rt = rx_ready_flag;
	rx_ready_flag = rt;
	return rt;
}

void uart_rx_notify(uint8_t port)
{
	UNUSED(port);
	rx_ready_flag = 1;
	return;
	// If UART is open
	if (usart_get_interrupt_mask(USART_BASE)
		& US_IMR_RXRDY) {
		// Enable UART TX interrupt to send a new value
		usart_enable_tx(USART_BASE);
		usart_enable_interrupt(USART_BASE, US_IER_TXRDY);
	}
}


void uart_config(uint8_t port, usb_cdc_line_coding_t * cfg)
{
	uint32_t stopbits, parity, databits;
	uint32_t imr;
	UNUSED(port);

	switch (cfg->bCharFormat) {
	case CDC_STOP_BITS_2:
		stopbits = US_MR_NBSTOP_2_BIT;
		break;
	case CDC_STOP_BITS_1_5:
		stopbits = US_MR_NBSTOP_1_5_BIT;
		break;
	case CDC_STOP_BITS_1:
	default:
		// Default stop bit = 1 stop bit
		stopbits = US_MR_NBSTOP_1_BIT;
		break;
	}

	switch (cfg->bParityType) {
	case CDC_PAR_EVEN:
		parity = US_MR_PAR_EVEN;
		break;
	case CDC_PAR_ODD:
		parity = US_MR_PAR_ODD;
		break;
	case CDC_PAR_MARK:
		parity = US_MR_PAR_MARK;
		break;
	case CDC_PAR_SPACE:
		parity = US_MR_PAR_SPACE;
		break;
	default:
	case CDC_PAR_NONE:
		parity = US_MR_PAR_NO;
		break;
	}
	
	switch(cfg->bDataBits) {
	case 5: case 6: case 7:
		databits = cfg->bDataBits - 5;
		break;
	default:
	case 8:
		databits = US_MR_CHRL_8_BIT;
		break;
	}

	// Options for USART.
	usart_options.baudrate = LE32_TO_CPU(cfg->dwDTERate);
	usart_options.char_length = databits;
	usart_options.parity_type = parity;
	usart_options.stop_bits = stopbits;
	usart_options.channel_mode = US_MR_CHMODE_NORMAL;
	imr = usart_get_interrupt_mask(USART_BASE);
	usart_disable_interrupt(USART_BASE, 0xFFFFFFFF);
	usart_init_rs232(USART_BASE, &usart_options,
			sysclk_get_peripheral_bus_hz(USART_BASE));
	// Restore both RX and TX
	usart_enable_tx(USART_BASE);
	usart_enable_rx(USART_BASE);
	usart_enable_interrupt(USART_BASE, imr);
}

void uart_open(uint8_t port)
{
	UNUSED(port);

	// IO is initialized in board init
	// Enable interrupt with priority higher than USB
	NVIC_SetPriority(USART_INT_IRQn, USART_INT_LEVEL);
	NVIC_EnableIRQ(USART_INT_IRQn);

	// Initialize it in RS232 mode.
	USART_PERIPH_CLK_ENABLE();
	if (usart_init_rs232(USART_BASE, &usart_options,
			sysclk_get_peripheral_bus_hz(USART_BASE))) {
		return;
	}
	// Enable USART
	USART_ENABLE();

	// Enable both RX and TX
	usart_enable_tx(USART_BASE);
//	usart_enable_rx(USART_BASE);
	// Enable interrupts
//	usart_enable_interrupt(USART_BASE, US_IER_RXRDY | US_IER_TXRDY);
	usart_enable_interrupt(USART_BASE, US_IER_TXRDY);

}

void uart_close(uint8_t port)
{
	UNUSED(port);
	// Disable interrupts
	usart_disable_interrupt(USART_BASE, 0xFFFFFFFF);
	// Close RS232 communication
	USART_DISABLE();
}
