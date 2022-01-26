#ifndef __MINIRTOSPORT_H
#define	__MINIRTOSPORT_H

#include "mini_libc.h"
#include "kernel.h" 

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
#define NULL ((void *)0)

#define stop_cpu          while(1)

//断言
#define vAssertCalled(char,int) mini_printf("Error:%s,%d\r\n",char,int)
#define configASSERT(x) if((x)==0) vAssertCalled(__FILE__,__LINE__)

void task_debug(void);
void Exit_Critical(uint32 irq_flag);
uint32 Enter_Critical(void);
void SVC_Handler(void);
void SVC_Handler_C(uint32 *svc_args);
void PendSV_Handler(void);
void RTOS_Tick_IRQ(void);
void Launch_Task_Schedule(void);
void miniRTOS_Init(void);
void Launch_Task_Schedule(void);
#endif 

