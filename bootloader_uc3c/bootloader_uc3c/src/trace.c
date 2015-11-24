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
	

char test_log_buf[TRACE_LOG_BUF_SIZE];
int test_log_buf_count = 0;
unsigned char test_trace_buf[TRACK_NUMBER];
int test_log_on_flag = 0;
void test_log_init(void)
{
	int i;
	test_log_buf_count = 0;
	for(i = 0; i < TRACE_LOG_BUF_SIZE; i++)
		test_log_buf[i] = 0;
	
	for(i = 0; i < TRACK_NUMBER; i++)
		test_trace_buf[i] = 0;
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
	if( n >= TRACK_NUMBER )
	{
		test_trace_buf[0] = 1;
		return;
	}

	if((0 == test_trace_buf[n]) && (test_log_on_flag) )
	{
		test_trace_buf[n] = 1;
		test_log_put('#');
		test_log_num(n);
		test_log_put(':');
		test_log_puts(s);
		test_log_put('#');
	}
}

void test_trace_report(void)
{
	int i;
	int cover_count = 0;
	for(i = 0; i < TRACK_NUMBER; i++)
	{
		if(test_trace_buf[i])
			cover_count++;
		
		test_log_put('[');
		test_log_num(test_trace_buf[i]);
		test_log_put(']');
		test_log_put('#');
		test_log_num(i);
		test_log_put('#');
	}
	test_port_puts(test_log_buf);
}

#endif


