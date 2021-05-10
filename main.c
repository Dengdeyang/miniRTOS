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

//----------------------RTOS属性变量以及IPC通信中间件定义区---------------------------//
Task_Handle Task1,Task2,Task3,Task4,Task5,Task6,Task7,Task8;          //RTOS任务id

Soft_Timer_Handle soft_timer0,soft_timer1,soft_timer2;                //软件定时器id

Semaphore_Handle Binary_Semaphore1;                                   //信号量定义(二值，计数，互斥)

Queue_Handle Queue_test;                                              //消息队列定义

Event_Handle test_event;                                              //事件标志组定义

//----------------------RTOS软件定时器回调函数和各任务函数声明------------------------//
void Timer0_callback(void);
void Timer1_callback(void);
void Timer2_callback(void);
void task1(void);
void task2(void);
void task3(void);
void task4(void);
void task5(void);
void task6(void);
void task7(void);
void task8(void);

//-------------------------板级支持包硬件外设初始化-----------------------------------//
void BSP_Init(void)
{
	USART1_Config();
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
	if(Queue_test==NULL) printf("Creat Queue Fail\r\n");
	
	//*timer_id,timer_switch_flag,Timer_mode,timer_priority,user_tick_count,*timer_function
	Soft_Timer_Creat(&soft_timer0,stop_timer,repeat_mode,1,500,Timer0_callback);
	Soft_Timer_Creat(&soft_timer1,stop_timer,repeat_mode,2,1000,Timer1_callback);
	Soft_Timer_Creat(&soft_timer2,stop_timer,once_mode,3,2000,Timer2_callback);
	
	//*task_id,*task_name,task_state,task_priority,task_delay_ms,task_stack_size_words,*task_function
	Task_Create(&Task1,"task1",TASK_READY,1,0,100,task1);
	Task_Create(&Task2,"task2",TASK_READY,2,0,100,task2);
	Task_Create(&Task3,"task3",TASK_READY,3,0,100,task3);
	Task_Create(&Task4,"task4",TASK_READY,4,0,100,task4);
	Task_Create(&Task5,"task5",TASK_READY,5,0,100,task5);
	Task_Create(&Task6,"task6",TASK_READY,6,0,100,task6);
	Task_Create(&Task7,"task7",TASK_READY,7,0,100,task7);
	Task_Create(&Task8,"task8",TASK_READY,8,0,100,task8);
}


//------------------------------RTOS 任务函数定义区---------------------------------//

//---------------空闲任务----------------//
void Idle_task(void)
{
	while(1)
	{
		//printf("Idle_task\r\n");
		__WFI();
	}
}

//---------------task1-----------------//
void task1(void)
{
	u32 temp;

	while(1)
	{
		printf("1111111111111111111\r\n");

		if(CMD =='0') 
		{
			CMD = 0;
	        
			if(xEventGroupWaitBits(&test_event, 0x0A,and_type,release_type,3000))      printf("Task1 wait Event OK \r\n");
			else                                                                       printf("Task1 wait Event Fail \r\n");
			printf("Task1 解除阻塞 \r\n");
			printf("Task1 wait event data = %X\r\n",test_event.Event);
			
		}
		
		if(CMD =='1') 
		{
			CMD = 0;
			
			if(QueueReceive(Queue_test,&temp,2000))             printf("Task1 Queue message Receive OK \r\n");
			else                                                printf("Task1 Queue message Receive Fail \r\n");
			printf("Task1 解除阻塞 \r\n");
			printf("Task1 Queue message receive data = %d\r\n",temp);
		}
		RTOS_Delay(1000);
	}
}

//---------------task2-----------------//
void task2(void)
{
	u32 temp;
	while(1)
	{
		printf("2222222222222222222222222222222\r\n");

		if(CMD =='2') 
		{
			CMD = 0;
			
			if(xEventGroupWaitBits(&test_event, 0x0A,and_type,release_type,MAX_DELAY))      printf("Task2 wait Event OK \r\n");
			else                                                                            printf("Task2 wait Event Fail \r\n");
			
			printf("Task2 解除阻塞 \r\n");
			printf("Task2 wait event data = %X\r\n",test_event.Event);
		}
		if(CMD =='3') 
		{
			CMD = 0;
			if(QueueReceive(Queue_test,&temp,MAX_DELAY))             printf("Task2 Queue message Receive OK \r\n");
			else                                                     printf("Task2 Queue message Receive Fail \r\n");
			printf("Task2 解除阻塞 \r\n");
			printf("Task2 Queue message receive data = %d\r\n",temp);
		}
		RTOS_Delay(1000);
	}
}

//---------------task3-----------------//
void task3(void)
{
	while(1)
	{
		printf("333333333333333333333333333333333\r\n");
		
		if(CMD =='4') 
		{
			CMD = 0;
			Set_Event_Bit(&test_event,1);
		}
		if(CMD =='5') 
		{
			CMD = 0;
			QueueSend(Queue_test,65,0);
		}
		
		RTOS_Delay(1000);
	}
}

//---------------task4-----------------//
void task4(void)
{
	while(1)
	{
		printf("4444444444444444444444444444444444444\r\n");
		
		if(CMD =='6') 
		{
			CMD = 0;
			Set_Event_Bit(&test_event,3);	
		}
		
		if(CMD =='p') 
		{
			CMD = 0;
			QueueSend(Queue_test,78,0);
		}
		
		RTOS_Delay(1000);
	}
}

//---------------task5-----------------//
void task5(void)
{
	while(1)
	{
		printf("555555555555555555555555555555555555555555\r\n");

		if(CMD =='7') 
		{
			CMD = 0;
			if(Semaphore_Take(Binary_Semaphore,&Binary_Semaphore1,5000))            printf("Task5 Semaphore1 Take OK \r\n");
			else                                                      					  printf("Task5 Semaphore1 Take Fail \r\n");
			printf("Task5 解除阻塞 \r\n");
		}
		
		RTOS_Delay(1000);
	}
}
 
//---------------task6-----------------//
void task6(void)
{
	while(1)
	{
		printf("666666666666666666666666666666666666666666666666666\r\n");
		
		if(CMD =='8') 
		{
			CMD = 0;

			Semaphore_Give(Binary_Semaphore,&Binary_Semaphore1);
		}
		
		RTOS_Delay(1000);
	}
}

//---------------task7-----------------//
void task7(void)
{
	while(1)
	{
		printf("7777777777777777777777777777777777777777777777777777777777777777777777\r\n");

		if(CMD =='9') 
		{
			CMD = 0;
			if(Semaphore_Take(Binary_Semaphore,&Binary_Semaphore1,MAX_DELAY))         printf("Task7 Take Semaphore1 OK \r\n");
			else                                                                            printf("Task7 Take Semaphore1 Fail \r\n");
			printf("Task7 解除阻塞 \r\n");
		}
		RTOS_Delay(1000);
	}
}

//---------------task8-----------------//
void task8(void)
{
	Device_Handle led=NULL;
	u8 tx_buffer = 0,rx_buffer = 0;
	
	while(1)
	{
		printf("888888888888888888888888888888888888888888888888888888888888888888888888888888888\r\n");
		switch(CMD)
		{
			case 'a':
			{
				Start_Soft_Timer(soft_timer0);
				
			};break;
			
			case 'b':
			{
				Start_Soft_Timer(soft_timer1);

			};break;
			
			case 'c':
			{
				Start_Soft_Timer(soft_timer2);

			};break;
			
			case 'd':
			{
				Stop_Soft_Timer(soft_timer0);

			};break;
			
			case 'e':
			{
				Stop_Soft_Timer(soft_timer1);

			};break;
			
			case 'f':
			{
				Stop_Soft_Timer(soft_timer2);
			};break;
			
			case 'g':
			{
				led = device_find("LED");
				if((led==NULL)&&(device_init(led))==Fail) printf("LED device is fail \r\n");
				else                                      printf("LED device is OK \r\n");
			};break;
			
			case 'h':
			{
				device_open(led);
				device_control(led,LED_intput,NULL);
				device_read(led,0,&rx_buffer,1);
				device_close(led);
				printf("rx_buffer = %d\r\n",rx_buffer);
			};break;
			
			case 'i':
			{
				tx_buffer = 0;
				device_open(led);
				device_control(led,LED_output,NULL);
				device_write(led,0,&tx_buffer,1);
				device_close(led);
			};break;
			
			case 'j':
			{
				tx_buffer = 1;
				device_open(led);
				device_control(led,LED_output,NULL);
				device_write(led,0,&tx_buffer,1);
				device_close(led);
			};break;
			
			default:break;
		}
		CMD = 0;
		RTOS_Delay(1000);
	}
}


//------------------------------软件定时器回调函数定义区------------------------------//
void Timer0_callback(void)
{
	printf("Timer0_callback is running\r\n");
	GPIO_B12_TOGGLE
}

void Timer1_callback(void)
{
	printf("Timer1_callback is running\r\n");
	GPIO_B13_TOGGLE
}

void Timer2_callback(void)
{
	printf("Timer2_callback is running\r\n");
	GPIO_B14_TOGGLE
}

//---------------------------中断中使用IPC测试--------------------------------//
void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{ 	
		CMD = (char)USART_ReceiveData(USART1);
		printf("CMD = %c\r\n",CMD);
		switch(CMD)
		{
			case 'k':
			{		
				Set_Event_Bit(&test_event,1);
			};break;
			
			case 'l':
			{
				Set_Event_Bit(&test_event,3);	
				
			};break;
			
			case 'm':
			{
				Reset_Event_Bit(&test_event,1);
			};break;
			
			case 'n':
			{
				QueueSend(Queue_test,65,0);
			};break;
			
			case 'o':
			{
				QueueSend(Queue_test,78,0);
			};break;
			
			case 'p':
			{
				Pend_Task(Task1);
			};break;
			
			case 'q':
			{
				Release_Task(Task1);
			};break;
			
			case 'r':
			{
				Semaphore_Give(Mutex_Semaphore,&Binary_Semaphore1);
			};break;
			default:break;
		}
	} 
}
			
