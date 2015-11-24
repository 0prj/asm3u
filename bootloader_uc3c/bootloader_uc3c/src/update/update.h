/*
 * update.h
 *
 * Created: 2015/7/29 9:24:43
 * Author: Li Dawen
 */ 


#ifndef _UPDATE_H_
#define _UPDATE_H_

#define COVER_TEST_PRINT(branched_num) {print_dbg("#COVER:"_##branched_num_##"#");}



#define FW_UPDATE_FLAG_LENGTH               0x01
#define FW_UPDATE_FLAG_ADDRESS              ((AVR32_FLASH_ADDRESS + AVR32_FLASHC_FLASH_SIZE) - FW_UPDATE_FLAG_LENGTH)

#define FLASH_BIN_ADDR  		0x80008000
#define FLASH_BIN_BEGIN_ADDR  	0x80000000
#define FLASH_BIN_START_ADDR  	0x80008000
#define FLASH_BIN_INFO_OFFSET 	0x100

#define FLASH_BOOT_SIZE			(1024*32)

#define FLASH_BIN_MAX_SIZE  	(AVR32_FLASHC_FLASH_SIZE - FLASH_BOOT_SIZE - 2*AVR32_FLASHC_PAGE_SIZE)
#define FLASH_BIN_INFO_ADDR		(FLASH_BIN_START_ADDR + FLASH_BIN_INFO_OFFSET)



#define FGLED_GPIO   		AVR32_PIN_PB04
#define FRLED_GPIO   		AVR32_PIN_PB05
#define Reset_FT232R		AVR32_PIN_PB18
#define GPIO_PUSH_Bandwidth	AVR32_PIN_PD28	//SW1, SW2,C2
#define TP_32				AVR32_PIN_PB16


#define GPIO_E1				AVR32_PIN_PA08	//@Kurios E1_INT

#define GPIO_BOOT_MODE_BUTTON GPIO_E1


#define UPDATE_PORT_UART	DBG_USART

#define PACKGE_TYPE_HEAD	'H'
#define PACKGE_TYPE_DATA	'D'
#define PACKGE_TYPE_QUIT	'Q'

#define MAX_PACKGE_DATA_SIZE	(4096)
#define MAX_PACKGE_BUFFER_SIZE	(MAX_PACKGE_DATA_SIZE+8)
#define PACK_RECV_TIMEOUT_1S	2



#define PACKGE_ERR_NOT						'0'
#define PACKGE_ERR_PACKGE_TYPE_ERROR		'1'
#define PACKGE_ERR_PACKGE_DATA_TIME_OUT		'2'
#define PACKGE_ERR_PACKGE_DATA_CRC_ERROR	'3'
#define PACKGE_ERR_PACKGE_INDEX_ERROR		'4'
#define PACKGE_ERR_FLASH_ERROR				'5'
#define PACKGE_ERR_BIN_CRC_ERROR			'6'
#define PACKGE_ERR_PACKGE_INFO_VALUE_ERROR	'7'
#define PACKGE_ERR_PACKGE_DATA_LEN_ERROR	'8'
#define PACKGE_ERR_QUIT						'9'



typedef struct PACKAGE_HEAD_INFO {
	unsigned char head;
	unsigned char index;
	unsigned short size;
	
	unsigned short packge_number;
	unsigned short bin_crc;
	
	unsigned long bin_size;
	unsigned long bin_begin_addr;
	unsigned long bin_start_addr;
	unsigned long reserved1;
	unsigned long reserved2;
	unsigned long reserved3;
	unsigned char bin_info[256];
	unsigned short crc;
}PACKAGE_HEAD_INFO_T;


void update_clean_flag(void);
char update_get_flag(void);
int  update_erase_flash(void);
void update_set_flag(void);
void usart_reset_sta(volatile avr32_usart_t *usart);

int packge_recv(unsigned char *dest, int destsz);
int memcpy_toflash(void *dest, const void *scr, int byte_size);

char update_key_is_press(void);
void update_console(void);
void update_respons_ok(void);
void update_respons_err(unsigned char ch);

void boot_main(void);
void boot_init(void);

#endif /* INCFILE1_H_ */
