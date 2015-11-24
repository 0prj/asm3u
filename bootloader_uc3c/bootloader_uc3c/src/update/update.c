/*
 * update.c
 * the serial port protocol for update AVR at32uc3c0512c FW
 * Created: 2015/7/29 9:21:31
 *  Author: Li Dawen
 */ 

#include "asf.h"
#include "crc16.h"
#include "update.h"
#include "string.h"
#include "../trace/trace.h"

/*
Host is PC
Client is AVR Boot
Port is serial port 115200,n,8,1


Host send frame
+------+-------+-------+-------+-------+-------+------+-----------+-----+-----+
| HEAD | INDEX | LEN L | LEN H |DATA(0)|DATA(1)|  ..  |DATA(LEN-1)|CRC L|CRC H|
+------+-------+-------+-------+-------+-------+------+-----------+-----+-----+
HEAD:
'H' is Head
'D' is Data

INDEX:
 0~255, the 255 next is 0, the head frame index is 0, the first data frame index is 1.

LEN:
the frame data lenght in byte
0~4096,normal value is 1024, max is 4096

CRC:
includes from HEAD to DATA(Len-1), CRC16-CCITT(0x1021)

Client ACK
+-----+-----+-----+
| 'O' | 'K' | '\n'|
+-----+-----+-----+

Client NAK
+-----+-----+-----+-----+-----+
| 'E' | 'R' | 'R' |  N  | '\n'|
+-----+-----+-----+-----+-----+
N:
0: ERROR NOT
1: PACKGE TYPE ERROR		[first char is not 'H' 'D' 'Q']
2: PACKGE DATA TIME OUT		[recv frame too short,is 2~(packge lenght+6)]
3: PACKGE DATA CRC ERROR	[frame CRC error]
4: PACKGE INDEX ERROR   	[index is not exp]
5: FLASH  ERROR				[flash write fail]
6: BIN CRC ERROR			[recv all frame, but bin file CRC error]
7: PACKGE INFO VALUE ERROR	[head packge info value is igellt]
8: PACKGE DATA LEN ERROR	[packge lenght > 4096]
9: PACKGE TRANS CANCEL		[recv 'Q']

Client process END
+-----+-----+-----+-----+
| 'E' | 'N' | 'D' | '\n'|
+-----+-----+-----+-----+


	PACKAGE_HEAD_INFO
    0      1      2     3
+------+------+------+------+  0
| head | index|    size     |
+======+======+======+======+  4
|packge_number|   bin_crc   |
+------+------+------+------+  8
|          bin_size         |
+------+------+------+------+  C
|       bin_begin_addr      |
+------+------+------+------+ 10
|       bin_start_addr      |
+------+------+------+------+ 14
|         reserved1         |
+------+------+------+------+ 18
|         reserved2         |
+------+------+------+------+ 1C
|         reserved3         |
+------+------+------+------+ 20
|      |      |      |      |
+-------bin_info[256]-------+ 
|      |      |      |      |
+======+======+======+======+ 20+100
|   head crc  |
+------+------+ 20+100+2

*/

#define DEBUG_INFO_EN    0

bool g_flash_erased = false;

int update_erase_flash(void)
{
	uint8_t flash_cmd_attempts = 0x00; 						  
	//skip 32KB
	uint32_t start_page = 0x00000040; //((int32_t)DestAddress / AVR32_FLASHC_PAGE_SIZE);
	uint32_t end_page	 = 0x000001FE;
	
	for(uint32_t i = start_page; i <= end_page; i++)
	{
		while(!flashc_erase_page(i, true))
		{
			if(++flash_cmd_attempts >= 5)			//Check FlashCmdAttempts bounds
			{
				return false;						// return error!!!
			} 
			delay_ms(3);							// In bounds, wait, try again 		  
		}
	}
	g_flash_erased = true;

	return true;
}

void update_clean_flag(void)
{
	flashc_erase_user_page(true);
	Byte flag = 0x00;

	flashc_memcpy((void*)FW_UPDATE_FLAG_ADDRESS, (void*)&flag, 1, 1);
}

void update_set_flag(void)
{
	flashc_erase_user_page(true);
	Byte flag = 0xFF;

	flashc_memcpy((void*)FW_UPDATE_FLAG_ADDRESS, (void*)&flag, 1, 1);
}

char update_get_flag(void)
{
	return *(unsigned char *)(FW_UPDATE_FLAG_ADDRESS);
}

int memcpy_toflash(void *dest, const void *scr, int byte_size)
{
	if(!g_flash_erased)
	{		
		TRACE(1, "flash is not erased");
		update_erase_flash();
	 	g_flash_erased = true;		  
	}			
	TRACE(2, "flash is erased");
	
	flashc_memcpy((uint32_t)dest, scr, byte_size, false);
	if(flashc_is_lock_error() || flashc_is_programming_error())
		return 0;
	
	TRACE(3, "flash copy is ok");
	return byte_size;
}

//the port_input_no_ready is serial port input error flag
static int 	port_input_no_ready = 0;


void usart_reset_sta(volatile avr32_usart_t *usart)
{
	if (usart->csr & (AVR32_USART_CSR_OVRE_MASK |
					  AVR32_USART_CSR_FRAME_MASK |
					  AVR32_USART_CSR_PARE_MASK))
	{
		TRACE(4, "reset port rx error");
		usart->cr = AVR32_USART_CR_RSTSTA_MASK;		
	}
	
}

static void port_outbyte(unsigned char trychar)
{
	usart_putchar(UPDATE_PORT_UART, trychar);
}

static unsigned char port_inbyte(unsigned int time_out)
{
    int ch=0;
    port_input_no_ready = 0;
	time_out *=(300000);
	time_out++;
	
	while((--time_out) && (!(usart_read_char(UPDATE_PORT_UART, &ch) == USART_SUCCESS)))
	{	
		TRACE(5, "wait port input data loop");		
		usart_reset_sta(UPDATE_PORT_UART);
	}
	
	if(0 == time_out)
	{
		TRACE(6, "wait port input data time out");
		port_input_no_ready = 1;
	}
    return (unsigned char)ch;
}



static void flushinput(void)
{
	int ch;
	while(((usart_read_char(UPDATE_PORT_UART, &ch) == USART_SUCCESS)))
	{
	}
	TRACE(7, "flashinput run");
	usart_reset_sta(UPDATE_PORT_UART);
}

unsigned char packge_buffer[MAX_PACKGE_BUFFER_SIZE];
PACKAGE_HEAD_INFO_T packge_head;

unsigned char g_packge_index=1;
static char g_err_ch = '0';

void update_respons_ok(void)
{
	TRACE(8, "send OK");

	flushinput();
	port_outbyte('O');
	port_outbyte('K');
	port_outbyte('\n');
}

void update_respons_end(void)
{
	TRACE(9, "send 'END'");

	flushinput();
	port_outbyte('E');
	port_outbyte('N');
	port_outbyte('D');
	port_outbyte('\n');
}

void update_respons_err(unsigned char ch)
{
	TRACE(10, "send 'ERRn'");

	flushinput();	
	port_outbyte('E');
	port_outbyte('R');
	port_outbyte('R');
	port_outbyte(ch);
	port_outbyte('\n');
}

void update_respons_bin_info(void)
{
	TRACE(11, "send bin info");

	flushinput();
	port_outbyte('O');
	port_outbyte('K');
	for(int i = 0; i < 256; i++)
	{
		print_dbg_char_hex(*(unsigned char *)(FLASH_BIN_INFO_ADDR+ i));
	}
	port_outbyte('\n');
}

static int packge_recv_frame(unsigned char *buf, int len)
{
    int i;
	int data_len = 0;
	unsigned short packge_crc=0;
	
	TRACE(12, "function enter");

	g_err_ch = PACKGE_ERR_NOT;
	do
	{
		g_err_ch = PACKGE_ERR_PACKGE_TYPE_ERROR;
		//	HEAD first date byte is filled

		// 1:index 2:3: packge length
		for(i = 1; i < 4; i++)
		{
			buf[i] = port_inbyte(PACK_RECV_TIMEOUT_1S);
			if(port_input_no_ready) 
			{
				TRACE(13, "recv packge head info time out");
				break;
			}
			TRACE(14, "recv packe head info loop");
		}
		if(port_input_no_ready) 
		{
			TRACE(15, "see 13");
			break;
		}
		TRACE(16, "head info is ready");
		g_err_ch= PACKGE_ERR_PACKGE_DATA_LEN_ERROR;

		// DATA
		data_len = (buf[3]<<8) + buf[2];
		if(data_len > len)
		{
			TRACE(17, "packge lenght error");
			break;
		}
		TRACE(18, "17 else");
		
		g_err_ch= PACKGE_ERR_PACKGE_DATA_TIME_OUT;
		for(i = 0; i < (data_len+2); i++)
		{
			buf[i+4] = port_inbyte(PACK_RECV_TIMEOUT_1S);
			if(port_input_no_ready)
			{
				TRACE(19, "recv data time out");
				break;
			}
			TRACE(20, "recv data loop");
		}
		if(port_input_no_ready)
		{
			TRACE(21, "see 20");
			break;
		}
		TRACE(22, "packge data is ready");
		packge_crc = crc16_ccitt(&buf[0], data_len+6);
		g_err_ch= PACKGE_ERR_PACKGE_DATA_CRC_ERROR;
		if(0 == packge_crc)
		{
			TRACE(23, "data crc error");
			g_err_ch= PACKGE_ERR_NOT;
			break;
		}
		TRACE(24, "23 else");
	}while(0);

	if(PACKGE_ERR_NOT == g_err_ch)
	{
		TRACE(25, "packge frame recv is ok");
		return data_len;
	}
	else
	{
		TRACE(26, "packge fram recv is error");
		return -1;
	}
}

int packge_head_process(PACKAGE_HEAD_INFO_T *packge_head, unsigned char *packge_buffer)
{
	packge_head->size = (packge_buffer[3]<<8) + packge_buffer[2];
	packge_head->packge_number =  (packge_buffer[5]<<8) + packge_buffer[4];;
	packge_head->bin_crc = (packge_buffer[7]<<8) + packge_buffer[6];
	packge_head->bin_size = (packge_buffer[11]<<24) + (packge_buffer[10]<<16) + (packge_buffer[9]<<8) + packge_buffer[8];
	packge_head->bin_begin_addr = (packge_buffer[15]<<24) + (packge_buffer[14]<<16) + (packge_buffer[13]<<8) + packge_buffer[12];
	packge_head->bin_start_addr = (packge_buffer[19]<<24) + (packge_buffer[18]<<16) + (packge_buffer[17]<<8) + packge_buffer[16];

	if ( ( ((packge_head->bin_start_addr>>8)<<8) != FLASH_BIN_START_ADDR)  \
		|| ( packge_head->bin_start_addr < packge_head->bin_begin_addr ) \
		|| ( packge_head->bin_size > FLASH_BIN_MAX_SIZE ) )
	{
		TRACE(27, "head data value is illegal");
		return 1;
	}
	TRACE(28, "data ok");
	return 0;
}

int update_wait_head_ch(unsigned char *packge_buffer)
{
	unsigned char ch; 
	while(1)		
	{
		ch = port_inbyte(PACK_RECV_TIMEOUT_1S);
		if(0 == port_input_no_ready) 
		{
			TRACE(29, "recv head ch time");
			packge_buffer[0] = ch;
			break;
		}
		TRACE(30, "wait loop");
	}
	if( (PACKGE_TYPE_HEAD == ch) || (PACKGE_TYPE_DATA == ch) )
	{
		//NULL
		TRACE(31, "is 'H' or 'D'");
	}else if(PACKGE_TYPE_QUIT == packge_buffer[0])
	{
		TRACE(32, "is 'Q'");
		g_err_ch = PACKGE_ERR_QUIT;
		return -2;
	}else
	{
		TRACE(33, "other char is error");
		// TYPE NOT 'H','D','Q', is ERROR
		g_err_ch = PACKGE_ERR_PACKGE_TYPE_ERROR;
		return -1;
	}
	return 0;
}

int update_wait_handshake(unsigned char *packge_buffer)
{
	packge_buffer[0] = 0;
	while(1)
	{
		flushinput();
		port_outbyte('U');
		packge_buffer[0] = port_inbyte(PACK_RECV_TIMEOUT_1S);
		if(port_input_no_ready) 
		{
			TRACE(34, "wait handshake time out");
			continue;		
		}
		TRACE(35, "34 else");

		if(PACKGE_TYPE_HEAD == packge_buffer[0])
		{
			TRACE(36, "is 'H'");
			break;
		}
		else if(PACKGE_TYPE_QUIT == packge_buffer[0])
		{
			TRACE(37, "is 'Q'");
			g_err_ch =  PACKGE_ERR_QUIT;
			update_respons_err(g_err_ch);
			return -2;
		} 
		else 
		{
			TRACE(38, "other char is error");
			g_err_ch =  PACKGE_ERR_PACKGE_TYPE_ERROR;
			update_respons_err(g_err_ch);
			return -1;
		}
	}
	return 0;
}

int packge_recv(unsigned char *dest, int destsz)
{
	unsigned char packge_index=1;
	unsigned char *pdest;
	int len = 0;
	int data_len;
	int flash_offset=0;
	int recv_data_size;
	unsigned short bin_crc = 0;
	pdest = dest;
	g_err_ch = PACKGE_ERR_NOT;

	for (int i=0; i < MAX_PACKGE_BUFFER_SIZE; i++)
		packge_buffer[i]=0;
	
	if (0 > update_wait_handshake(&packge_buffer[0]))
	{
		TRACE(39, "handshake ERR");
		return 0;
	}
	TRACE(40, "39 else");

	while(1)
	{		
		if((PACKGE_TYPE_DATA == packge_buffer[0]))
		{
			TRACE(41, "is 'D'");
			data_len = packge_recv_frame(&packge_buffer[0], MAX_PACKGE_DATA_SIZE);
			if(PACKGE_ERR_NOT != g_err_ch)
			{
				TRACE(42, "recv frame error");
				break;	
			}
			TRACE(43, "42 else");
			g_err_ch = PACKGE_ERR_PACKGE_INDEX_ERROR;
			if((packge_index != packge_buffer[1]))
			{
				TRACE(44, "index error");
				break;
			}
			TRACE(45, "44 else");
			
			bin_crc = crc16_ccitt_stream(bin_crc, &packge_buffer[4], data_len);
			if(len >= flash_offset)
			{
				TRACE(46, "need copy to flash");
				//need write to flash
				g_err_ch = PACKGE_ERR_FLASH_ERROR;
				if(0 == memcpy_toflash(&dest[len-flash_offset], &packge_buffer[4], data_len))
				{
					TRACE(47, "copy error");
					break;
				}
				TRACE(48, "47 else");
			}
			else
			{
				//NULL
				TRACE(49, "48 else");
			}
			len += data_len;
			packge_index++;
			
			update_respons_ok();

			if((len >= packge_head.bin_size))
			{
				TRACE(50, "data is end");
				//recv bin file end
				g_err_ch = PACKGE_ERR_BIN_CRC_ERROR;
				if(bin_crc == packge_head.bin_crc)
				{
					TRACE(51, "bin crc is ok");
					g_err_ch = PACKGE_ERR_NOT;
					update_clean_flag();					
				}
				else
				{
					//NULL
					TRACE(52, "51 else");
				}
				break;
			}
		}
		else if(PACKGE_TYPE_HEAD == packge_buffer[0])
		{			
			TRACE(53, "is 'H'");
			data_len = packge_recv_frame(&packge_buffer[0], MAX_PACKGE_DATA_SIZE);
			if(PACKGE_ERR_NOT != g_err_ch)
			{
				TRACE(54, "recv frame error");
				break;
			}
			TRACE(55, "53 else");
			recv_data_size = data_len;
			packge_index = 1;

			if(packge_head_process(&packge_head, &packge_buffer[0]))
			{
				TRACE(56, "head process error");
				g_err_ch = PACKGE_ERR_PACKGE_INFO_VALUE_ERROR;
				break;
			}
			TRACE(57, "55 else");
			flash_offset = ((packge_head.bin_start_addr>>8)<<8) - ((packge_head.bin_begin_addr>>8)<<8);
			g_flash_erased = false;	

			update_respons_bin_info();
		}
		else
		{
		// NULL
		 	TRACE(58, "other char can't trace");
		}
		TRACE(58, "58 hold");
		
		g_err_ch= PACKGE_ERR_PACKGE_TYPE_ERROR;
		if(update_wait_head_ch(&packge_buffer[0]) < 0)
		{
			TRACE(59, "wait head char error ");
			break;
		}
		TRACE(60, "59 else");
	}
	
	if(PACKGE_ERR_NOT == g_err_ch)
	{
		TRACE(61, "is ok");
//clean the update flag
//		update_clean_flag();
		update_respons_end();
		return len;
	}
	else
	{
		TRACE(62, "is error");
		update_respons_err(g_err_ch);
		return -1;
	}
}


