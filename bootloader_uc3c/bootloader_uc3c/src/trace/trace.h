#ifndef _TRACE_H_
#define _TRACE_H_


#define TRACE_EN     0
#define TRACE_LOG_EN     0

#define TRACE_LOG_OUT_PORT_MEM	0	
#define TRACE_LOG_OUT_PORT_UART	1
#define TRACE_REPORT_OUT_PORT_MEM

#if TRACE_EN
#define TRACE(n, s) 	test_trace((n), (s))
#define LOG(s) 			test_log_puts(s)
#define TRACE_INIT() 	test_init()
#define TRACE_REPORT() 	test_trace_report()
#else
#define TRACE(n, s) 
#define LOG(s) 
#define TRACE_INIT()
#define TRACE_REPORT() 
#endif

#define TRACE_LOG_OUT_PORT
#define TRACE_LOG_BUF_SIZE   (1024*8)
#define TRACE_NUMBER  (62+1)

void test_init(void);
void test_log_put(char ch);
void test_log_puts(char *ch);
void test_log_report_puts(char *ch);
void test_log_report(void);
void test_trace(unsigned int n, char *s);
void test_trace_report(void);

#endif


