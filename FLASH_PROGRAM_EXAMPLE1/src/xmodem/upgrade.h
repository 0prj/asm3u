/*
 * upgrade.h
 *
 * Created: 2015/7/29 9:24:43
 *  Author: root
 */ 


#ifndef _UPGRAND_H_
#define _UPGRAND_H_


#define PACKAGE_MAGIC 		0x5448   //THOR
#define DATA_TYPE_BIN  		0x0B
#define DATA_TYPE_DATA  	0x0D
#define DATA_TYPE_FACTORY 	0x0F

//VID PID PACKAGE SIZE
typedef struct PACKAGE_HEAD_INFO {
	unsigned short magic;
	unsigned short head_crc;
	
	unsigned short data_type;    
	unsigned short data_crc;
	
	unsigned long data_size;
	unsigned long data_load_addr;
}PACKAGE_HEAD_INFO_T;

void upgrade_reset(void);

void upgrade_flashcpy(void * dest, const void *scr, int byte_size);

int upgrade_check_package(void * package_addr);

int upgrade_is_package(void * package_addr);

unsigned long upgrade_get_data_offset(void * package_addr);

unsigned long upgrade_get_data_size(void * package_addr);

void upgrade_flashcpy(void * dest, const void *scr, int byte_size);

int xmodemReceive(unsigned char *dest, int destsz);


#endif /* INCFILE1_H_ */
