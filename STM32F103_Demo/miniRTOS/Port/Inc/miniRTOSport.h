#ifndef __MINIRTOSPORT_H
#define	__MINIRTOSPORT_H

#include "mini_libc.h"
#include "kernel.h" 

#define stop_cpu          __breakpoint(0)

//断言
#define vAssertCalled(char,int) mini_printf("Error:%s,%d\r\n",char,int)
#define configASSERT(x) if((x)==0) vAssertCalled(__FILE__,__LINE__)

void task_debug(void);
void Exit_Critical(unsigned int irq_flag);
unsigned int Enter_Critical(void);
void SVC_Handler(void);
void SVC_Handler_C(unsigned int *svc_args);
void PendSV_Handler(void);
void RTOS_Tick_IRQ(void);
void Launch_Task_Schedule(void);
void miniRTOS_Init(void);
void Launch_Task_Schedule(void);
#endif 

