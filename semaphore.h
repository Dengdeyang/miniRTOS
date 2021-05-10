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
struct Queue_node
{
	volatile u32 data;
	struct Queue_node *next;
};
typedef struct Queue_node Queue_point;

//���нṹ��
typedef struct
{
	Queue_point *head;
	Queue_point *tail;
	volatile u32    queue_size;
	volatile u8 task_block_list[TASK_NUM];
}Queue;
typedef Queue * Queue_Handle;

//�ź����ṹ��
typedef struct
{
   volatile u32 Semaphore;
	        u16 task_id;
			u16 task_priority;
	volatile u8 task_block_list[TASK_NUM];
}Semaphore_Handle;

//-------------------------------------------------------------------------------//
Queue_Handle Creat_queue(void);
char QueueSend(Queue *queue_data,u32 push_data,u32 delay_ms);
char QueueReceive(Queue *queue_data,u32 *pull_data,u32 delay_ms);
char Semaphore_Take(Semaphore_Type type,Semaphore_Handle *Semaphore,u32 delay_ms);
void Semaphore_Give(Semaphore_Type type,Semaphore_Handle *Semaphore);
#endif 
