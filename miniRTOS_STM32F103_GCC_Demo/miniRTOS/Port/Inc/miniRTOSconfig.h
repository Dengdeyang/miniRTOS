#ifndef __MINIRTOSCONFIG_H
#define	__MINIRTOSCONFIG_H

//----------------操作系统裁剪--------------------
#define	USER_TASK_NUM        2     //用户指定任务数(不含空闲任务)

#define USER_Softimer_NUM    2     //用户指定软件定时器个数

#define SysTick_Rhythm       1000  //用户指定操作系统心跳时钟周期，单位us

#define miniRTOS_Semaphore   1     //用户指定是否使用信号量和消息队列

#define miniRTOS_Event       1     //用户指定是否使用事件标志组

#define miniRTOS_Softimer    1     //用户指定是否使用软件定时器

#define miniRTOS_Device      1     //用户指定是否使用驱动抽象层

#endif 
