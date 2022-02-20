#ifndef __SEMAPHORE_H
#define	__SEMAPHORE_H

#include "kernel.h"

//信号量状态枚举
typedef enum  
{
	Semaphore_Unblock = 0,
	Semaphore_Block = 1,
}Semaphore_state;

//信号量类型枚举
typedef enum  
{
	Binary_Semaphore = 1,
	Mutex_Semaphore,
	Count_Semaphore,
}Semaphore_Type;


//队列节点结构体
typedef struct Queue_node
{
	uint8* data;
	struct Queue_node *next;
}Queue_point;

//消息队列结构体
typedef struct
{
	Queue_point *head;
	Queue_point *tail;
	uint8 task_block_list[TASK_NUM];
}Queue_message;


//信号量结构体
typedef struct
{
   volatile uint32 Semaphore;
	        uint16 task_id;
			uint16 task_priority;
	volatile uint8 task_block_list[TASK_NUM];
}Semaphore_Handle;


typedef  Queue_message* Queue_Handle; 
//-------------------------------------------------------------------------------//
Queue_Handle Creat_queue(void);
char QueueSend(Queue_message *queue,void *message,uint32 byteNum,uint32 delay_ms,MCU_mode mode);
char QueueReceive(Queue_message *queue,void *message,uint32 byteNum,uint32 delay_ms,MCU_mode mode);
void Semaphore_Creat(Semaphore_Type type,Semaphore_Handle *Semaphore,uint32 init_value);
char Semaphore_Take(Semaphore_Type type,Semaphore_Handle *Semaphore,uint32 delay_ms,MCU_mode mode);
void Semaphore_Give(Semaphore_Type type,Semaphore_Handle *Semaphore);
#endif 
