//crc16.h
 
#ifndef _CRC16_H_
#define _CRC16_H_
unsigned short crc16_ccitt(const unsigned char *buf, int len);
unsigned short crc16_ccitt_stream(unsigned short crc, const unsigned char *buf, int len);

#endif
