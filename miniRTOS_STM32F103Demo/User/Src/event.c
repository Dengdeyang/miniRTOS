/********************************************************************************
  * @file    event.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   事件标志组用于多任务或多事件间的同步
  * @atteration   
  ********************************************************************************/
#include "event.h"   
#include "semaphore.h"
#include "soft_timer.h"

//-------------------------------------------------------------------------//
/**
 * @brief  任务等待事件标志组，可设置阻塞时间或非阻塞。
 * @param  
			event：      事件标志组变量地址
			care_bit：   任务需要关注事件标志组的bit(最大32bit)
			relate_flag：关心的bit之间是“与”关系，还是“或”关系
			action_flag：等待到事件发生后，保持事件标志，还是清除事件标志
			delay_ms：   任务读取事件标志组，事件未发生，任务需要阻塞的延时时间，若为0，不等待，若为MAX_DELAY，一直等待。
 * @retval 等待该事件标志组成功或失败标志                       
 */
char xEventGroupWaitBits(Event_Handle * event, u32 care_bit,Relate_Type relate_flag,Action_Type action_flag,u32 delay_ms,MCU_mode mode) 
{
	char flag = Fail;
	
	Enter_Critical();
	flag = (char) ((relate_flag == and_type) ? ((event->Event & care_bit) == care_bit):((event->Event & care_bit) != 0));
	
	if(flag != Success) 
	{
		if(mode != Handle_mode)
		{
			if(delay_ms == 0)  flag = Fail;
			else
			{
				event->task_care_bit_list[current_task_id] = care_bit;
				event->task_relate_type_list[current_task_id] = relate_flag;
				
				event->task_block_list[current_task_id] = Semaphore_Block;
				Exit_Critical();
				RTOS_Delay(delay_ms);
				Enter_Critical();
				event->task_block_list[current_task_id] = Semaphore_Unblock;

				flag = (char) ((relate_flag == and_type) ? ((event->Event & care_bit) == care_bit):((event->Event & care_bit) != 0));
			}
		}
		else flag = Fail;
	}
	
	if((action_flag == release_type) && (flag == Success)) event->Event &= ~care_bit;
	Exit_Critical();
	return flag;
}

//-----------------------------置位事件标志组对应bit----------------------------------------//
void Set_Event_Bit(Event_Handle * event,u8 bit)
{
	u16 i;
	u32 care_bit;
	Relate_Type relate_flag;
	char flag = Fail;

	Enter_Critical();
	event->Event |= (1<<bit);

	for(i=0;i<TASK_NUM;i++)
	{
		if((event->task_block_list[i]==Semaphore_Block)&&(*(Task_List[i].task_id_point) != timer_guard_task_id))  
		{
			care_bit = event->task_care_bit_list[i];
			relate_flag = (Relate_Type) event->task_relate_type_list[i];
			
			flag = (char)((relate_flag == and_type) ? ((event->Event & care_bit) == care_bit):((event->Event & care_bit) != 0));
			
			if(flag == Success)
			{
				event->task_block_list[i] = Semaphore_Unblock;
				Task_List[i].task_state = TASK_READY;
				Task_List[i].task_pend_state = TASK_READY;
				List_insert(priority_list,&Task_List[i],order_insert);
			}
		}
	}
	Launch_Task_Schedule();
	Exit_Critical();
}

//-----------------------------清除事件标志组对应bit----------------------------------------//
void Reset_Event_Bit(Event_Handle * event,u8 bit)
{
	u16 i;
	u32 care_bit;
	Relate_Type relate_flag;
	char flag = Fail;
	
	Enter_Critical();
	event->Event &= ~(1<<bit); 

	for(i=0;i<TASK_NUM;i++)
	{
		if((event->task_block_list[i]==Semaphore_Block)&&(*(Task_List[i].task_id_point) != timer_guard_task_id))  
		{
			care_bit = event->task_care_bit_list[i];
			relate_flag = (Relate_Type) event->task_relate_type_list[i];
			
			flag = (char)((relate_flag == and_type) ? ((event->Event & care_bit) == care_bit):((event->Event & care_bit) != 0));
			
			if(flag == Success)
			{
				event->task_block_list[i] = Semaphore_Unblock;
				Task_List[i].task_state = TASK_READY;
				Task_List[i].task_pend_state = TASK_READY;
				List_insert(priority_list,&Task_List[i],order_insert);
			}
		}
	}
	Launch_Task_Schedule();
	Exit_Critical();
}
						
