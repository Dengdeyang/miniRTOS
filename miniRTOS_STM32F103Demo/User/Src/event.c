/********************************************************************************
  * @file    event.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   �¼���־�����ڶ��������¼����ͬ��
  * @atteration   
  ********************************************************************************/
#include "event.h"   
#include "semaphore.h"
#include "soft_timer.h"

//-------------------------------------------------------------------------//
/**
 * @brief  ����ȴ��¼���־�飬����������ʱ����������
 * @param  
			event��      �¼���־�������ַ
			care_bit��   ������Ҫ��ע�¼���־���bit(���32bit)
			relate_flag�����ĵ�bit֮���ǡ��롱��ϵ�����ǡ��򡱹�ϵ
			action_flag���ȴ����¼������󣬱����¼���־����������¼���־
			delay_ms��   �����ȡ�¼���־�飬�¼�δ������������Ҫ��������ʱʱ�䣬��Ϊ0�����ȴ�����ΪMAX_DELAY��һֱ�ȴ���
 * @retval �ȴ����¼���־��ɹ���ʧ�ܱ�־                       
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

//-----------------------------��λ�¼���־���Ӧbit----------------------------------------//
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

//-----------------------------����¼���־���Ӧbit----------------------------------------//
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
						
