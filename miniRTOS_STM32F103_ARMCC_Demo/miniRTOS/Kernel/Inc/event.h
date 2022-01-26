#ifndef __EVENT_H
#define	__EVENT_H

#include "kernel.h"
 
//---------------------事件标志组结构体--------------------------//
typedef struct
{
	volatile uint32 Event;
	uint32 task_care_bit_list[TASK_NUM];
	uint8  task_relate_type_list[TASK_NUM];
	volatile Task_Handle task_block_list[TASK_NUM];
}Event_Handle;

//---------------------二选一状态枚举量--------------------------/
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


char xEventGroupWaitBits(Event_Handle * event, uint32 care_bit,Relate_Type relate_flag,Action_Type action_flag,uint32 delay_ms,MCU_mode mode);
void Set_Event_Bit(Event_Handle * event,uint8 bit);
void Reset_Event_Bit(Event_Handle * event,uint8 bit);
#endif 
