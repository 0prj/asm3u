#include "asf.h"

#include "trace.h"
#include "../update/update.h"
#if TRACE_EN 
/*
if(x)
{
	T_(1,"the recv exp char");   //
}
else
{
	T_(2,"recv time out");  // 
}

switch(y)
{
	case 1:
		T_(3, "is head packge"); 
		break;
	
	case 2:
		T_(4, "is data packge");
		break;
}
*/
	

int test_log_on_flag = 0;
int test_log_buf_count = 0;
int test_trace_num_over = 0;

int test_trace_report_update = 1;
unsigned char test_trace_buf[TRACE_NUMBER+10];
char test_log_buf[TRACE_LOG_BUF_SIZE];


void test_log_init(void)
{
	int i;
	test_trace_report_update = 1;
	test_log_buf_count = 0;
	for(i = 0; i < TRACE_LOG_BUF_SIZE; i++)
		test_log_buf[i] = 0;
}
void test_trace_init(void)
{
	int i;
	test_trace_num_over = 0;
	test_trace_report_update = 1;
	for(i = 0; i < TRACE_NUMBER; i++)
		test_trace_buf[i] = 0;
}
void test_init(void)
{
	test_log_init();
	test_trace_init();
	TRACE(0, "trace init");
}


void test_log_put(char ch)
{
	if(test_log_buf_count < TRACE_LOG_BUF_SIZE)
	{
		test_log_buf[test_log_buf_count++] = ch;
	}
}

void test_log_num(unsigned int n)
{
	char tmp[11];
	int i = sizeof(tmp) - 1;
	
	// Convert the given number to an ASCII decimal representation.
	tmp[i] = '\0';
	do
	{
	  tmp[--i] = '0' + n % 10;
	  n /= 10;
	} while (n);
	
	// Transmit the resulting string with the given buffer.
	test_log_puts(tmp + i);
}

void test_port_put(char *ch)
{
	usart_putchar(UPDATE_PORT_UART, ch);
}

void test_port_puts(char *ch)
{
	while(*ch)
	{
		test_port_put(*ch++);
	}
}

void test_log_puts(char *ch)
{
	while(*ch)
	{
		test_log_put(*ch++);
	}
}

void test_log_report_puts(char *ch)
{

}

void test_log_report(void)
{
	test_log_buf[TRACE_LOG_BUF_SIZE-1] = 0;
	test_log_report_puts(test_log_buf);
}

/*
 test_track(101, "packge type is error")
 
 #101:packge type is error#
*/
void test_trace(unsigned int n, char *s)
{
	if( n >= TRACE_NUMBER )
	{
		test_trace_num_over = n;
		return;
	}
	if(0 == test_trace_buf[n])
	{
		test_trace_buf[n] = 1;
		
#if TRACE_LOG_EN
		if(test_log_on_flag)
		{
			test_log_put('#');
			test_log_num(n);
			test_log_put(':');
			test_log_puts(s);
			test_log_put('#');
			test_log_put('\n');
		}
#endif	
	}
	test_trace_report_update = 1;	
}

void test_trace_report(void)
{
	int i;
	int cover_count = 0;
	if(1 == test_trace_report_update)
	{
		int cover_count = 0;

		test_log_init();
		test_log_puts("\n\n####  TRACE REPORT  ####");

		for(i = 0; i < TRACE_NUMBER; i++)
		{
			if(test_trace_buf[i])
				cover_count++;
			test_log_puts("\n[ ");
			test_log_num(test_trace_buf[i]);
			test_log_puts(" ]:");
			test_log_num(i);
			test_log_put('#');
		}

		test_log_puts("\nCOVER:");
		test_log_num(cover_count);
		test_log_puts("\nTRACE:");
		test_log_num(TRACE_NUMBER);
		
		if(test_trace_num_over)
		{
			test_log_puts("\nCOVER TEST:OVER:");
			test_log_num(test_trace_num_over);
		}
		
		if(TRACE_NUMBER == cover_count)
		{
			test_log_puts("\nCOVER TEST:PASS\n");
		}
		else
		{
			test_log_puts("\nCOVER TEST:FAIL\n");
		}
		test_log_puts("\n\n####  TRACE REPORT END ####");
		test_trace_report_update  = 0;

	}
	
	test_port_puts(test_log_buf);
}

#endif


