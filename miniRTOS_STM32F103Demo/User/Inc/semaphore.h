#ifndef __SEMAPHORE_H
#define	__SEMAPHORE_H

#include "kernel.h"

//�ź���״̬ö��
typedef enum  
{
	Semaphore_Unblock = 0,
	Semaphore_Block = 1,
}Semaphore_state;

//�ź�������ö��
typedef enum  
{
	Binary_Semaphore = 1,
	Mutex_Semaphore,
	Count_Semaphore,
}Semaphore_Type;


//���нڵ�ṹ��
typedef struct Queue_node
{
	unsigned char* data;
	struct Queue_node *next;
}Queue_point;

//��Ϣ���нṹ��
typedef struct
{
	Queue_point *head;
	Queue_point *tail;
	u8 task_block_list[TASK_NUM];
}Queue_message;


//�ź����ṹ��
typedef struct
{
   volatile u32 Semaphore;
	        u16 task_id;
			u16 task_priority;
	volatile u8 task_block_list[TASK_NUM];
}Semaphore_Handle;


typedef  Queue_message* Queue_Handle; 
//-------------------------------------------------------------------------------//
Queue_Handle Creat_queue(void);
char QueueSend(Queue_message *queue,void *message,unsigned int byteNum,u32 delay_ms,MCU_mode mode);
char QueueReceive(Queue_message *queue,void *message,unsigned int byteNum,u32 delay_ms,MCU_mode mode);
char Semaphore_Take(Semaphore_Type type,Semaphore_Handle *Semaphore,u32 delay_ms,MCU_mode mode);
void Semaphore_Give(Semaphore_Type type,Semaphore_Handle *Semaphore);
#endif 
