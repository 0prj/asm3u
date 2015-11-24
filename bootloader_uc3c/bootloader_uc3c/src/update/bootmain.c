/*
 * bootmain.c
 *
 * Created: 2015/7/29 9:21:31
 *  Author: Li Dawen
 */ 


#include "asf.h"

#include "crc16.h"
#include "update.h"
#include "../trace/trace.h"

char update_key_is_press(void)
{
	int key_count = 0;
	char i = 0;
	
	for(i = 0; i < 10; i++)
	{
		// get boot mode button status
		if (gpio_get_pin_value(GPIO_BOOT_MODE_BUTTON) == 0)
			key_count++;
		else
			key_count = 0;
		
		delay_ms(5);	
	}
	if(key_count)
		return 1;					
	else
		return 0;
}

void update_console(void)
{
	int rt;
	int uc_key;
	unsigned short cc;
	
	while(1)
	{
		rt = usart_read_char(UPDATE_PORT_UART, &uc_key);
		if(USART_SUCCESS != rt)
		{
			usart_reset_sta(UPDATE_PORT_UART);
			continue;
		}	
		switch(uc_key)
		{
			case 'a':
			case 'A':
				rt = packge_recv((unsigned char *)FLASH_BIN_START_ADDR, FLASH_BIN_MAX_SIZE);
				break;
				
			case 'g':
			case 'G':
				update_respons_ok();
				return;
				break;

			case 'y':
			case 'Y':
				usart_write_char(UPDATE_PORT_UART, 'Y');
				break;				
#if TRACE_EN
			case 'i':
			case 'I':
				TRACE_INIT();
				update_respons_ok();				
				break;
			case 'r':
			case 'R':
				TRACE_REPORT();
				update_respons_ok();
				break;					
			case 'v':
				flashc_lock_page_region((FLASH_BIN_START_ADDR-AVR32_FLASH_ADDRESS)/ AVR32_FLASHC_PAGE_SIZE,true);
				update_respons_err('5');
				break;
			case 't':
				if(flashc_is_page_region_locked((FLASH_BIN_START_ADDR-AVR32_FLASH_ADDRESS)/ AVR32_FLASHC_PAGE_SIZE))
					update_respons_err('5');
				else
					update_respons_ok();
				break;
			case 'x':
				flashc_lock_page_region((FLASH_BIN_START_ADDR-AVR32_FLASH_ADDRESS)/ AVR32_FLASHC_PAGE_SIZE,false);
				update_respons_ok();
				break;
#endif				
			default:
				break;
		}
	}
}

void boot_init(void)
{
	TRACE_INIT();

	/* Insert system clock initialization code here (sysclk_init()). */
	pcl_switch_to_osc(PCL_OSC0, FOSC0, OSC0_STARTUP);

	// Initialize the system clock
	sysclk_init();

	// Initialize the debug USART module.
	init_dbg_rs232(sysclk_get_main_hz());
	
	//board_init();
	gpio_set_gpio_pin(GPIO_PUSH_Bandwidth);	
	gpio_set_pin_low(GPIO_PUSH_Bandwidth);
	
	gpio_configure_pin(GPIO_BOOT_MODE_BUTTON, GPIO_DIR_INPUT);
}

void boot_main(void)
{
	void (*boot)(void)=(void*)FLASH_BIN_START_ADDR;		
	int rc;
	
	if(update_key_is_press()||update_get_flag())
	{
		rc = packge_recv((unsigned char *)FLASH_BIN_START_ADDR, FLASH_BIN_MAX_SIZE);
		update_console();
	}
	boot();    
}


