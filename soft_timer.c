/********************************************************************************
  * @file    soft_timer.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   �����ʱ�����������ڴ����������¸�����Ҫ���壬ͻ��Ӳ����ʱ������������
  * @atteration   
  ********************************************************************************/
#include "soft_timer.h"   
#include "list.h" 

//--------------------------------�����ʱ���б���-----------------------------------//
Softimer_Unit Softimer_List[Softimer_NUM]={0};
Softimer_Unit *Softimer_list_head;
Task_Handle timer_guard_task_id;
Soft_Timer_Handle idle_softimer;


//--------------------------------�����ʱ���ػ�����-----------------------------------//
/**
 * @brief   ������RTOS�����У������ʱ���ػ���������ȼ���ߣ�RTOS�����ж����жϵ��������ʱ��ready�ᴥ��������
            �ڸ������н��е��������ʱ����Ӧ�Ļص�����
 * @param   NO
 * @retval  NO                      
 */
void soft_timer_guard_task(void)
{
	u16 i;
	while(1)
	{
		for(i=0;i<Softimer_NUM;i++)
		{
			if((Softimer_List[i].timer_state == run_timer)&&(Softimer_List[i].timer_runflag == run_timer))
			{
				(*Softimer_List[i].callback_function)();
				Softimer_List[i].timer_state = stop_timer;
				
				if(Softimer_List[i].timer_mode == repeat_mode)
				{
					Softimer_List[i].timer_count = SysTick_count + Softimer_List[i].timer_reload;
					Softimer_list_insert(&Softimer_List[i]);
				}
				else
				{
					Softimer_List[i].timer_runflag = stop_timer;
				}
			}
		}
		Pend_Task(timer_guard_task_id);
	}
}

//-------------------------------������Ӧ�����ʱ��-----------------------------------//
void Start_Soft_Timer(Soft_Timer_Handle timer_id)
{
	taskENTER_CRITICAL();
	Softimer_List[timer_id].timer_count = SysTick_count + Softimer_List[timer_id].timer_reload;
	Softimer_list_insert(&Softimer_List[timer_id]);
	Softimer_List[timer_id].timer_state = stop_timer;
	Softimer_List[timer_id].timer_runflag = run_timer;
	taskEXIT_CRITICAL();
}

//-------------------------------ֹͣ��Ӧ�����ʱ��------------------------------------//
void Stop_Soft_Timer(Soft_Timer_Handle timer_id)
{
	taskENTER_CRITICAL();
	Softimer_list_remove_node(&Softimer_List[timer_id]);
	Softimer_List[timer_id].timer_state = stop_timer;
	Softimer_List[timer_id].timer_runflag = stop_timer;
	taskEXIT_CRITICAL();
}

//---------------------���ö�Ӧ�����ʱ���Ķ�ʱֵ(RTOS����������)------------------------//
void Set_Soft_Timer(Soft_Timer_Handle timer_id,u32 tick_count)
{
	taskENTER_CRITICAL();
	Softimer_List[timer_id].timer_reload = tick_count;
	Softimer_List[timer_id].timer_count = SysTick_count + tick_count;
    Softimer_list_insert(&Softimer_List[timer_id]);
	Softimer_List[timer_id].timer_state = stop_timer;
	Softimer_List[timer_id].timer_runflag = run_timer;
	taskEXIT_CRITICAL();
}

//-------------------------------�����ʱ���ĳ�ʼ��------------------------------------//
/**
 * @brief   ������RTOS�����У������ʱ���ػ���������ȼ���ߣ�RTOS�����ж����жϵ��������ʱ��ready�ᴥ��������
            �ڸ������н��е��������ʱ����Ӧ�Ļص�����
 * @param   
			*timer_id��         �����ʱ����id������ַ
			timer_switch_flag�� ������ر������ʱ��
			user_tick_count��   �û�ָ�������ʱ���Ķ�ʱֵ(RTOS����������)
			timer_function      �����ʱ����Ӧ�ص�����
 * @retval  NO                      
 */
void Soft_Timer_Creat(Soft_Timer_Handle *timer_id,Timer_Switch timer_state,Timer_mode mode,u16 priority,u32 timer_reload_value,void (*timer_function)(void))
{
	static u16 timer_id_init=0;
	configASSERT(timer_id_init < Softimer_NUM);
	
	Softimer_List[timer_id_init].softimer_id = timer_id;
	Softimer_List[timer_id_init].timer_state = timer_state;
	Softimer_List[timer_id_init].timer_mode = mode;
	Softimer_List[timer_id_init].timer_priority = priority;
	Softimer_List[timer_id_init].timer_reload = timer_reload_value;
	Softimer_List[timer_id_init].timer_runflag = timer_state;
	Softimer_List[timer_id_init].timer_count = timer_reload_value;
	Softimer_List[timer_id_init].callback_function = timer_function;
	timer_id_init++;
}

