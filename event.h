#ifndef __EVENT_H
#define	__EVENT_H

#include "kernel.h"
 
//---------------------�¼���־��ṹ��--------------------------//
typedef struct
{
	volatile u32 Event;
	u32 task_care_bit_list[TASK_NUM];
	u8  task_relate_type_list[TASK_NUM];
	volatile Task_Handle task_block_list[TASK_NUM];
}Event_Handle;

//---------------------��ѡһ״̬ö����--------------------------/
typedef enum  
{
	and_type = 0,
	or_type,
}Relate_Type;

typedef enum  
{
	hold_type = 0,
	release_type,
}Action_Type;


char xEventGroupWaitBits(Event_Handle * event, u32 care_bit,Relate_Type relate_flag,Action_Type action_flag,u32 delay_ms);
void Set_Event_Bit(Event_Handle * event,u8 bit);
void Reset_Event_Bit(Event_Handle * event,u8 bit);
#endif 
