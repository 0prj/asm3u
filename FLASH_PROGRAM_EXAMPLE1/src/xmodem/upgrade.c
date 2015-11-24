/*
 * upgrade.c
 *
 * all function is in the RAM at run time
 * Created: 2015/7/29 9:21:31
 *  Author: root
 */ 

#include "asf.h"
#include "sam3u4e.h"
#include "crc16.h"
#include "upgrade.h"

/*
1 port selection

2 protocol

3 firmware upgrade

4 reset system
*/
#include "upgrade.h"


__no_inline
RAMFUNC
void upgrade_reset(void)
{
	NVIC_SystemReset();
}

__no_inline
RAMFUNC
int upgrade_is_package(void * package_addr)
{
	uint16_t *p;
	p = (uint16_t *)package_addr;
	printf("\nmagic:%X", *p);
	
	if(*p == PACKAGE_MAGIC)
		return 0;
	return 1;
}

__no_inline
RAMFUNC
void upgrade_flashcpy(void * dest, const void *scr, int byte_size)
{
	irqflags_t flags;

	Efc *p_efc;
	uint32_t ul_fws_temp;
	uint16_t us_page;
	uint32_t ul_error;
	uint32_t ul_idx;
	uint32_t *p_aligned_dest;
	uint32_t *p_aligned_scr;

	uint32_t ul_addr;
	uint32_t ul_size;
	uint32_t ul_page_size;
	
	ul_addr = (uint32_t)dest;	

	if (ul_addr >= IFLASH1_ADDR) {
		p_efc = EFC1;
		us_page = (ul_addr - IFLASH1_ADDR) / IFLASH1_PAGE_SIZE;
		ul_page_size = IFLASH1_PAGE_SIZE;
	} else {
		p_efc = EFC0;
		us_page = (ul_addr - IFLASH0_ADDR) / IFLASH0_PAGE_SIZE;
		ul_page_size = IFLASH0_PAGE_SIZE;
	}

	ul_size = byte_size;
	ul_size +=256;
	ul_size>>=8;
	ul_size<<=8;
	
	p_aligned_dest = (uint32_t *)dest;
	p_aligned_scr =  (uint32_t *)scr;
	
	ul_fws_temp = efc_get_wait_state(p_efc);
	efc_set_wait_state(p_efc, 6);
	printf("copy from:0x%08x to:0x%08x size:%d", (uint32_t)scr, (uint32_t)dest, byte_size);
	// The following code must be in RAM
	
	flags = cpu_irq_save();
	while (ul_size > 0) {
		
		for (ul_idx = 0; ul_idx < (ul_page_size>>2);	++ul_idx) {
			*p_aligned_dest++ = *p_aligned_scr++;
		}
		
		ul_error = efc_perform_command(p_efc, EFC_FCMD_EWP,	us_page);

		/* Progression */
		ul_size -= ul_page_size;
		us_page++;
	}	
	efc_set_wait_state(p_efc, ul_fws_temp);
	// to reset the cotex-M3
	upgrade_reset();
	while(1);
}

int upgrade_check_package(void * package_addr)
{
	char * pdata;
	PACKAGE_HEAD_INFO_T * phead = (PACKAGE_HEAD_INFO_T *)package_addr;
	pdata = (char *)package_addr+ sizeof(PACKAGE_HEAD_INFO_T);

	printf("\npackage head: %p",package_addr);	
	printf("\nimagic:0x%X head_crc:0x%X",phead->magic, phead->head_crc);	
	printf("\ndata_type:0x%X data_crc:0x%X",phead->data_type, phead->data_crc);	
	printf("\ndata_size:%d data_load_addr:0x%X",phead->data_size, phead->data_load_addr);	

	printf("\nhead_crc(%p):0x%X", &phead->data_type, crc16_ccitt((char *)&phead->data_type, 12)); 
	printf("\ndata_crc(%p):0x%X", pdata, crc16_ccitt(pdata, phead->data_size)); 

	if(!(phead->head_crc == crc16_ccitt((char *)&phead->data_type, 12)))
	{
		printf("\n!!!head_crc error:0x%X", crc16_ccitt((char *)&phead->data_type, 12));	
//		return 1;
	}
	if(!(phead->data_crc == crc16_ccitt(pdata, phead->data_size)))
	{
		printf("\n!!!data_crc error(%p):0x%X", pdata, crc16_ccitt(pdata, phead->data_size));	
//		return 2;
	}
	return 0;
}

unsigned long upgrade_get_data_offset(void * package_addr)
{
	PACKAGE_HEAD_INFO_T * phead = (PACKAGE_HEAD_INFO_T *)package_addr;
	return phead->data_size;
}

unsigned long upgrade_get_data_size(void * package_addr)
{
	PACKAGE_HEAD_INFO_T * phead = (PACKAGE_HEAD_INFO_T *)package_addr;
	return phead->data_size;
}

