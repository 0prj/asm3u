#include "asf.h"

#include "string.h"


#include <stdio.h>
#include "compiler.h"
#include "sysclk.h"
#include "serial.h"
#include "stdio_serial.h"
#include "conf_board.h"
#include "conf_clock.h"

#include "crc16.h"

#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18
#define CTRLZ 0x1A
#define DLY_1S 1
#define MAXRETRANS 25
static int last_error = 0;

extern uint32_t SystemCoreClock;

#define CONF_XMODEM_UART CONF_UART


memcpy_toflash(void *dest, const void *scr, int byte_size)
{
	uint32_t ul_rc;
	ul_rc = flash_write((uint32_t)dest, scr, byte_size, 1);

	if(FLASH_RC_OK == ul_rc)
		return byte_size;
	else
		return 0;
}

void port_outbyte(unsigned char trychar)
{
	usart_serial_putchar(CONF_XMODEM_UART, trychar);
}

unsigned char port_inbyte(unsigned int time_out)
{
    unsigned char ch;
    int i;
    last_error = 0;
	time_out *=(SystemCoreClock>>5);
	time_out++;
	
	while((--time_out) && (!usart_serial_is_rx_ready(CONF_XMODEM_UART)))
	{
	}
	
	if(time_out==0)
	{
		last_error = 1;
		return ch;
	}
	usart_serial_getchar(CONF_XMODEM_UART, &ch);
    return ch;
}

static int check(int crc, const unsigned char *buf, int sz)
{
    if (crc)
    {
        unsigned short crc = crc16_ccitt(buf, sz);
        unsigned short tcrc = (buf[sz] << 8) + buf[sz + 1];
        if (crc == tcrc)
            return 1;
    }
    else
    {
        int i;
        unsigned char cks = 0;
        for (i = 0; i < sz; ++i)
        {
            cks += buf[i];
        }
        if (cks == buf[sz])
            return 1;
    }
    return 0;
}

static void flushinput(void)
{
	if(usart_serial_is_rx_ready(CONF_XMODEM_UART))
		port_inbyte(((DLY_1S) * 3) >> 1);
		
//    while (port_inbyte(((DLY_1S) * 3) >> 1) >= 0)
//        ;
}

int xmodemReceive(unsigned char *dest, int destsz)
{
    unsigned char xbuff[1030];
	unsigned char page_buf[256];
    unsigned char *p;
    int bufsz, crc = 0;
    unsigned char trychar = 'C';
    unsigned char packetno = 1;
    int i, c, len = 0;
    int retry, retrans = MAXRETRANS;
    for(;;)
    {
        for( retry = 0; retry < 16; ++retry)
        {
            if (trychar)
                port_outbyte(trychar);
            c = port_inbyte((DLY_1S) << 1);
            if (last_error == 0)
            {
                switch (c)
                {
                case SOH:
                    bufsz = 128;
                    goto start_recv;
//                case STX:
//                    bufsz = 1024;
//                    goto start_recv;
                case EOT:
                    flushinput();
                    port_outbyte(ACK);
					//
					if(len & 0x80)
					{
						memcpy_toflash(&dest[len-128], &page_buf[0], 256);
					}
                    return len;
                case CAN:
                    c = port_inbyte(DLY_1S);
                    if (c == CAN)
                    {
                        flushinput();
                        port_outbyte(ACK);
                        return -1;
                    }
                    break;
                default:
                    break;
                }
            }
        }
        if (trychar == 'C')
        {
            trychar = NAK;
            continue;
        }
        flushinput();
        port_outbyte(CAN);
        port_outbyte(CAN);
        port_outbyte(CAN);
        return -2;
		
start_recv:
        if (trychar == 'C') crc = 1;
        trychar = 0;
        p = xbuff;
        *p++ = c;
        for (i = 0;  i < (bufsz + (crc ? 1 : 0) + 3); ++i)
        {
            c = port_inbyte(DLY_1S);
            if (last_error != 0)
                goto reject;
            *p++ = c;
        }
        if (xbuff[1] == (unsigned char)(~xbuff[2]) &&
                (xbuff[1] == packetno || xbuff[1] == (unsigned char)packetno - 1) &&
                check(crc, &xbuff[3], bufsz))
        {
            if (xbuff[1] == packetno)
            {
                int count = destsz - len;
                if (count > bufsz)
                    count = bufsz;
                if (count > 0)
                {
                    //memcpy (&dest[len], &xbuff[3], count);
					memcpy (&page_buf[len&0xFF], &xbuff[3], count);
                    if((len&0x80))
                    {
						memcpy_toflash(&dest[len-128], &page_buf[0], count*2);
					}

					len += count;
                }
                ++packetno;
                retrans = MAXRETRANS + 1;
            }
            if (--retrans <= 0)
            {
                flushinput();
                port_outbyte(CAN);
                port_outbyte(CAN);
                port_outbyte(CAN);
                return -3;
            }
            port_outbyte(ACK);
            continue;
        }
reject:
        flushinput();
        port_outbyte(NAK);
    }
}

#if 0

int xmodemTransmit(unsigned char *src, int srcsz)
{
    unsigned char xbuff[1030];
    int bufsz, crc = -1;
    unsigned char packetno = 1;
    int i, c, len = 0;
    int retry;
    for(;;)
    {
        for( retry = 0; retry < 16; ++retry)
        {
            c = port_inbyte((DLY_1S) << 1);
            if (last_error == 0)
            {
                switch (c)
                {
                case 'C':
                    crc = 1;
                    goto start_trans;
                case NAK:
                    crc = 0;
                    goto start_trans;
                case CAN:
                    c = port_inbyte(DLY_1S);
                    if (c == CAN)
                    {
                        port_outbyte(ACK);
                        flushinput();
                        return -1;
                    }
                    break;
                default:
                    break;
                }
            }
        }
        port_outbyte(CAN);
        port_outbyte(CAN);
        port_outbyte(CAN);
        flushinput();
        return -2;
        for(;;)
        {
start_trans:
            xbuff[0] = SOH;
            bufsz = 128;
            xbuff[1] = packetno;
            xbuff[2] = ~packetno;
            c = srcsz - len;
            if (c > bufsz) c = bufsz;
            if (c >= 0)
            {
                memset (&xbuff[3], 0, bufsz);
                if (c == 0)
                {
                    xbuff[3] = CTRLZ;
                }
                else
                {
                    memcpy (&xbuff[3], &src[len], c);
                    if (c < bufsz) xbuff[3 + c] = CTRLZ;
                }
                if (crc)
                {
                    unsigned short ccrc = crc16_ccitt(&xbuff[3], bufsz);
                    xbuff[bufsz + 3] = (ccrc >> 8) & 0xFF;
                    xbuff[bufsz + 4] = ccrc & 0xFF;
                }
                else
                {
                    unsigned char ccks = 0;
                    for (i = 3; i < bufsz + 3; ++i)
                    {
                        ccks += xbuff[i];
                    }
                    xbuff[bufsz + 3] = ccks;
                }
                for (retry = 0; retry < MAXRETRANS; ++retry)
                {
                    for (i = 0; i < bufsz + 4 + (crc ? 1 : 0); ++i)
                    {
                        port_outbyte(xbuff[i]);
                    }
                    c = port_inbyte(DLY_1S);
                    if (last_error == 0 )
                    {
                        switch (c)
                        {
                        case ACK:
                            ++packetno;
                            len += bufsz;
                            goto start_trans;
                        case CAN:
                            c = port_inbyte(DLY_1S);
                            if ( c == CAN)
                            {
                                port_outbyte(ACK);
                                flushinput();
                                return -1;
                            }
                            break;
                        case NAK:
                        default:
                            break;
                        }
                    }
                }
                port_outbyte(CAN);
                port_outbyte(CAN);
                port_outbyte(CAN);
                flushinput();
                return -4;
            }
            else
            {
                for (retry = 0; retry < 10; ++retry)
                {
                    port_outbyte(EOT);
                    c = port_inbyte((DLY_1S) << 1);
                    if (c == ACK) break;
                }
                flushinput();
                return (c == ACK) ? len : -5;
            }
        }
    }
}
#endif


