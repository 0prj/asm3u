OS: Windows 7 32bit
IDE: Atmel Studio 7.0

A: 
run Atmel Studio 7.0
New GCC C ASF Board project 

Board Select:
MCU is AT32UC3C0512C,
Board is UC3C-EK-AT32UC3C0512C


B:
ASF added:
*1 GPIO-General-Purpose Input/Output (driver)
*2 Generic board support(driver)
3 Delay routines (service)
4 PM-Power Manager(driver)
5 USART Debug strings(service)
* board default select

C:
Add New folder 'update' under src\
Added files bootmain.c crc16.c update.c update.h crc16.h to 'src\update\'

modify the following files:

src\main.c
#include <asf.h>
#include "update/update.h"
int main (void)
{
	/* Insert system clock initialization code here (sysclk_init()). */
	boot_init();
	/* Insert application code here, after the board has been initialized. */
	boot_main();
}

ASF\avr32\utils\debug\print_funcs.h
#elif BOARD == UC3C_EK
#  define DBG_USART               (&AVR32_USART2)
#  define DBG_USART_RX_PIN        AVR32_USART2_RXD_0_1_PIN
#  define DBG_USART_RX_FUNCTION   AVR32_USART2_RXD_0_1_FUNCTION
#  define DBG_USART_TX_PIN        AVR32_USART2_TXD_0_1_PIN
#  define DBG_USART_TX_FUNCTION   AVR32_USART2_TXD_0_1_FUNCTION
#  define DBG_USART_IRQ           AVR32_USART2_IRQ
#  define DBG_USART_BAUDRATE      115200         