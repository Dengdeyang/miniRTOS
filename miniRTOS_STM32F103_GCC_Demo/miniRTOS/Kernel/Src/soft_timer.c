/********************************************************************************
  * @file    soft_timer.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   软件定时器数量可在内存允许条件下根据需要定义，突破硬件定时器的数量限制
  * @atteration   
  ********************************************************************************/
#include "soft_timer.h"   
#include "list.h" 
#include "miniRTOSconfig.h"

//--------------------------------软件定时器列表定义-----------------------------------//
Softimer_Unit Softimer_List[Softimer_NUM]={0};
Softimer_Unit *Softimer_list_head;
Task_Handle timer_guard_task_id;
Soft_Timer_Handle idle_softimer;

//--------------------------------软件定时器守护任务-----------------------------------//
/**
 * @brief   在所有RTOS任务中，软件定时器守护任务的优先级最高，RTOS心跳中断中判断到有软件定时器ready会触发该任务，
            在该任务中进行调用软件定时器对应的回调函数
 * @param   NO
 * @retval  NO                      
 */
void soft_timer_guard_task(void)
{
	uint16 i;
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

//-------------------------------软件定时器的初始化------------------------------------//
/**
 * @brief   在所有RTOS任务中，软件定时器守护任务的优先级最高，RTOS心跳中断中判断到有软件定时器ready会触发该任务，
            在该任务中进行调用软件定时器对应的回调函数
 * @param   
			*timer_id：         软件定时器的id变量地址
			timer_switch_flag： 开启或关闭软件定时器
			user_tick_count：   用户指定软件定时器的定时值(RTOS心跳节拍数)
			timer_function      软件定时器对应回调函数
 * @retval  NO                      
 */
void Soft_Timer_Creat(Soft_Timer_Handle *timer_id,Timer_Switch timer_state,Timer_mode mode,uint16 priority,uint32 timer_reload_value,void (*timer_function)(void))
{
	static uint16 timer_id_init=0;
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

#if miniRTOS_Softimer
//-------------------------------启动对应软件定时器-----------------------------------//
void Start_Soft_Timer(Soft_Timer_Handle timer_id)
{
	uint32 irq_status;
	irq_status = Enter_Critical();
	Softimer_List[timer_id].timer_count = SysTick_count + Softimer_List[timer_id].timer_reload;
	Softimer_list_insert(&Softimer_List[timer_id]);
	Softimer_List[timer_id].timer_state = stop_timer;
	Softimer_List[timer_id].timer_runflag = run_timer;
	Exit_Critical(irq_status);
}

//-------------------------------停止对应软件定时器------------------------------------//
void Stop_Soft_Timer(Soft_Timer_Handle timer_id)
{
	uint32 irq_status;
	irq_status = Enter_Critical();
	Softimer_list_remove_node(&Softimer_List[timer_id]);
	Softimer_List[timer_id].timer_state = stop_timer;
	Softimer_List[timer_id].timer_runflag = stop_timer;
	Exit_Critical(irq_status);
}

//---------------------设置对应软件定时器的定时值(RTOS心跳节拍数)------------------------//
void Set_Soft_Timer(Soft_Timer_Handle timer_id,uint32 tick_count)
{
	uint32 irq_status;
	irq_status = Enter_Critical();
	Softimer_List[timer_id].timer_reload = tick_count;
	Softimer_List[timer_id].timer_count = SysTick_count + tick_count;
    Softimer_list_insert(&Softimer_List[timer_id]);
	Softimer_List[timer_id].timer_state = stop_timer;
	Softimer_List[timer_id].timer_runflag = run_timer;
	Exit_Critical(irq_status);
}

#endif
