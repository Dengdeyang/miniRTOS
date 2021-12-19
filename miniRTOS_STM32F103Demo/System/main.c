/********************************************************************************
  * @file    main.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   main文件完成硬件初始化，RTOS属性变量，任务函数，以及IPC通信中间件定义和初始化
  * @atteration   
  ********************************************************************************/
#include "stm32f10x.h"
#include "led.h"
#include "uart.h"
#include "kernel.h"
#include "semaphore.h" 
#include "event.h"    
#include "soft_timer.h"   
#include "device.h"
#include "mini_libc.h"
#include "heap.h"

char CMD=0;
//----------------------RTOS属性变量以及IPC通信中间件定义区---------------------------//
Task_Handle Task1,Task2;          

Soft_Timer_Handle soft_timer0,soft_timer1;                //软件定时器id

Semaphore_Handle Binary_Semaphore1;                       //信号量定义(二值，计数，互斥)

Queue_Handle Queue_test;                                  //消息队列定义

Event_Handle test_event;                                  //事件标志组定义

//----------------------RTOS软件定时器回调函数和各任务函数声明------------------------//
void Timer0_callback(void);
void Timer1_callback(void);
 
void task1(void);
void task2(void);

//-------------------------板级支持包硬件外设初始化-----------------------------------//
void BSP_Init(void)
{
	USART1_Config();
	HeapInit();
	//queue_init(&printf_buf,printf_buf_size);
    LED_Device_Register();	
}

//-------------------------------------main主函数-----------------------------------//
int main(void)
{	
    BSP_Init();
	RTOS_Init();
	while(1)
	{
		stop_cpu;
	}
}

//-------------------------RTOS 消息队列、软件定时器、任务创建初始化-----------------------------------//
void Task_Creat_Init(void)
{
	Queue_test = Creat_queue();
	if(Queue_test==NULL) mini_printf("Creat Queue Fail\r\n");
	
	//*timer_id,timer_switch_flag,Timer_mode,timer_priority,user_tick_count,*timer_function
	Soft_Timer_Creat(&soft_timer0,stop_timer,repeat_mode,1,500,Timer0_callback);
	Soft_Timer_Creat(&soft_timer1,stop_timer,repeat_mode,2,1000,Timer1_callback);
	
	//*task_id,*task_name,task_state,task_priority,task_delay_ms,task_tick_ms,task_stack_size_words,*task_function
	Task_Create(&Task1,"task1",TASK_READY,1,0,1000, 40,task1);
	Task_Create(&Task2,"task2",TASK_READY,1,0,1000,100,task2);
}


//------------------------------RTOS 任务函数定义区---------------------------------//

//---------------空闲任务----------------//
void Idle_task(void)
{
	while(1)
	{
		//mini_printf("Idle_task\r\n");
		//__WFI();
	}
}

void fault_test_by_div0(void) 
{
    volatile int * SCB_CCR = (volatile int *) 0xE000ED14; // SCB->CCR
    int x, y, z;

    *SCB_CCR |= (1 << 4); /* bit4: DIV_0_TRP. */

    x = 10;
    y = 0;
    z = x / y;
    mini_printf("z:%d\n", z);
}

//---------------task1-----------------//
void task1(void)
{
	u8 tx_buffer = 0,rx_buffer = 0;
	Device_Handle led=NULL;
	float x2;
	while(1)
	{
		mini_printf("1111111111111111111111111111111\r\n");
		switch(CMD)
		{
			case '1':
			{
				fault_test_by_div0();
				Start_Soft_Timer(soft_timer0);
				Start_Soft_Timer(soft_timer1);
				mini_printf("Start_Soft_Timer\r\n");
				CMD = 0;
			};break;
			
			case '2':
			{
				Stop_Soft_Timer(soft_timer0);
				Stop_Soft_Timer(soft_timer1);
				CMD = 0;
			};break;
			
			case '3':
			{
				led = device_find("LED");
				if((led==NULL)&&(device_init(led))==Fail) mini_printf("LED device is fail \r\n");
				else                                      mini_printf("LED device is OK \r\n");
				CMD = 0;
			};break;
			
			case '4':
			{
				device_open(led);
				device_control(led,LED_intput,NULL);
				device_read(led,0,&rx_buffer,1);
				device_close(led);
				mini_printf("rx_buffer = %d\r\n",rx_buffer);
				CMD = 0;
			};break;
			
			case '5':
			{
				tx_buffer = 0;
				device_open(led);
				device_control(led,LED_output,NULL);
				device_write(led,0,&tx_buffer,1);
				device_close(led);
				CMD = 0;
			};break;
			
			case '6':
			{
				tx_buffer = 1;
				device_open(led);
				device_control(led,LED_output,NULL);
				device_write(led,0,&tx_buffer,1);
				device_close(led);
				CMD = 0;
			};break;
			
			case '7':
			{
				if(xEventGroupWaitBits(&test_event, 0x0A,or_type,release_type,3000,User_mode))      mini_printf("Task1 wait Event OK \r\n");
				else                                                                       mini_printf("Task1 wait Event Fail \r\n");
				mini_printf("Task1 解除阻塞 \r\n");
				mini_printf("Task1 wait event data = %X\r\n",test_event.Event);
				CMD = 0;
			};break;
			
			case '8':
			{
				if(QueueReceive(Queue_test,&x2,sizeof(x2),5000,User_mode))    mini_printf("Task1 Queue message Receive OK \r\n");
				else                                                mini_printf("Task1 Queue message Receive Fail \r\n");
				mini_printf("Task1 解除阻塞 \r\n");
				mini_printf("Task1 Queue message receive data = %f\r\n",x2);
				CMD = 0;
			};break;
			
			case '9':
			{
				if(Semaphore_Take(Binary_Semaphore,&Binary_Semaphore1,5000,User_mode))            mini_printf("Task5 Semaphore1 Take OK \r\n");
				else                                                      					  mini_printf("Task5 Semaphore1 Take Fail \r\n");
				mini_printf("Task5 解除阻塞 \r\n");
				CMD = 0;
			};break;
			
			case 'a':
			{
				Pend_Task(Task1);
				CMD = 0;
			};break;
			
			default:break;
		}
		RTOS_Delay(1000);
	}
}

//---------------task2-----------------//
void task2(void)
{
	float x1 = 3.14159;
	while(1)
	{
		mini_printf("2222222222222222222222222222222\r\n");
		switch(CMD)
		{
			case 'b':
			{
				Set_Event_Bit(&test_event,1);
				CMD = 0;
			};break;
			
			case 'c':
			{
				mini_printf("QueueSend %f\r\n",x1);
				QueueSend(Queue_test,&x1,sizeof(x1),0,User_mode);
				CMD = 0;
			};break;
			
			case 'd':
			{
				Semaphore_Give(Binary_Semaphore,&Binary_Semaphore1);
				CMD = 0;
			};break;
			
			case 'e':
			{
				Release_Task(Task1);
				CMD = 0;
			};break;
			default:break;
		}
		
		RTOS_Delay(1000);
	}
}


//------------------------------软件定时器回调函数定义区------------------------------//
void Timer0_callback(void)
{
	mini_printf("Timer0_callback is running\r\n");
}

void Timer1_callback(void)
{
	mini_printf("Timer1_callback is running\r\n");
}
			
//---------------------------中断中使用IPC测试--------------------------------//
void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{ 	
		CMD = (char)USART_ReceiveData(USART1);
		mini_printf("CMD = %c\r\n",CMD);
	}
}
