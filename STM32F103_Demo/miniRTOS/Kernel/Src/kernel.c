/********************************************************************************
  * @file    kernel.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   RTOS内核开发，包含：
  *								1)任务创建函数
  *                             2)临界段代码保护函数
  *								3)任务挂起与恢复函数
  *								4)调度器的挂起与恢复函数
  *                             5)RTOS心跳滴答定时器中断服务函数(优先级判决+抢占式任务调度)
  *                             6)RTOS延时函数(释放CPU使用权)
  *                             7)RTOS初始化函数
  * @atteration   启动文件.s中，堆区必须定义较大，在任务创建时，满足任务栈较大或任务数量较多时mini_malloc申请
  ********************************************************************************/
#include "stm32f10x.h"
#include "kernel.h" 
#include "list.h" 
#include "heap.h"
#include "miniRTOSport.h"
//------------------RTOS 任务列表，优先级列表，timer列表，系统栈及其他中间全局变量定义区--------//
Task_Unit Task_List[TASK_NUM]={0};
volatile uint32_t PSP_array[TASK_NUM]; 
Task_Unit *Timer_list_head;
Task_Unit *Priority_list_head;
Task_Handle idle_task;

volatile uint32_t SysTick_count = 0;
volatile uint32_t current_task_id = 0;
volatile uint32_t next_task_id = 1;
volatile Scheduler_state scheduler_pend_flag = NO_Pend;

void Idle_task(void);
void Task_Creat_Init(void);


//---------------------------------RTOS 任务创建函数-----------------------------------------//
/**
  * @brief  动态申请任务栈内存，并对任务列表中对应的任务结构体变量进行初始化。
  * @param  u16 *task_id:              任务id变量地址
			char *task_name:           任务名称，字符串
			u8 task_state:            任务状态
			u16 task_priority:         任务优先级
			u32 task_delay_ms:         任务延时值
			u16 task_stack_size_words: 任务栈大小(单位:32bit word)
			void *task_function:       任务函数地址
  * @retval 任务创建成功或失败标志                       
  */
u8 Task_Create( 
				u16 *task_id,
				char *task_name,
				u8 task_state,
				u16 task_priority,
				u32 task_delay_ms,
				u32 task_tick_ms,
				u16 task_stack_size_words,
				void *task_function	
			  )
{
	u32 i;
	static u16 task_id_index=0;
	
	configASSERT(task_id_index < TASK_NUM);
	
	Task_List[task_id_index].task_id_point = task_id;
	Task_List[task_id_index].task_name = task_name;
	Task_List[task_id_index].task_state = task_state;
	Task_List[task_id_index].task_pend_state = task_state;
	Task_List[task_id_index].task_priority = task_priority;
	Task_List[task_id_index].task_delay_ms = task_delay_ms;
	Task_List[task_id_index].task_tick_reload = task_tick_ms;
	Task_List[task_id_index].task_tick_count = task_tick_ms;
	Task_List[task_id_index].task_stack = (unsigned int *) mini_malloc((task_stack_size_words)*sizeof(unsigned int));
	Task_List[task_id_index].task_stack_size_words = task_stack_size_words;
	Task_List[task_id_index].task_function = task_function;
	if(Task_List[task_id_index].task_stack != NULL) 
	{
		for(i=0;i<Task_List[task_id_index].task_stack_size_words;i++)
		{
			*(Task_List[task_id_index].task_stack+i) = 0xdeadbeef;
		}
		task_id_index++;
		return Success;
	}
	else
	{
		mini_printf("Not enough memory for task stack!\r\n");
		return Fail;
	}
}

//------------------------------RTOS任务的挂起--------------------------------------------//
void Pend_Task(Task_Handle Task_x)
{
	u32 irq_status;
	irq_status = Enter_Critical();
	Task_List[Task_x].task_pend_state = TASK_BLOCK;
	Task_List[Task_x].task_state = TASK_BLOCK;
	List_remove_node(priority_list,&Task_List[Task_x]);
	Launch_Task_Schedule();
	Exit_Critical(irq_status);
}

//------------------------------RTOS任务恢复-----------------------------------------------//
void Release_Task(Task_Handle Task_x)
{
	u32 irq_status;
	irq_status = Enter_Critical();
	Task_List[Task_x].task_pend_state = TASK_READY;
	Task_List[Task_x].task_state = TASK_READY;
	List_insert(priority_list,&Task_List[Task_x],order_insert);
	Launch_Task_Schedule();
	Exit_Critical(irq_status);
}

//------------------------------调度器挂起--------------------------------------------//
void Pend_Schedule(void)
{
	u32 irq_status;
	irq_status = Enter_Critical();
	scheduler_pend_flag = Pend;
	Exit_Critical(irq_status);
}

//------------------------------调度器恢复--------------------------------------------//
void Release_Schedule(void)
{
	u32 irq_status;
	irq_status = Enter_Critical();
	scheduler_pend_flag = NO_Pend;
	Exit_Critical(irq_status);
}

//-------------------------------RTOS心跳中断服务函数--------------------------------------//
void RTOS_Tick_IRQ(void)
{
	u32 irq_status;
	irq_status = Enter_Critical();
	SysTick_count++;
   
	if((SysTick_count > Task_List[current_task_id].task_tick_count) && \
	   (Task_List[current_task_id].task_state == TASK_READY) && \
	   (Task_List[current_task_id].priority_list_next->task_priority == Task_List[current_task_id].task_priority))
	{
		List_insert(priority_list,&Task_List[current_task_id],reverse_insert);
	}

	if(SysTick_count > Timer_list_head->task_delay_ms)
	{
		Timer_list_head->task_state = TASK_READY;
		List_insert(priority_list,Timer_list_head,order_insert);
		if(Timer_list_head->timer_list_next != NULL) Timer_list_head = Timer_list_head->timer_list_next;
	}
	
	if(SysTick_count > Softimer_list_head->timer_count)
	{
		Softimer_list_head->timer_state = run_timer;
		if(Softimer_list_head->timer_runflag == run_timer) Release_Task(timer_guard_task_id);
		if(Softimer_list_head->softimer_next != NULL) Softimer_list_head = Softimer_list_head->softimer_next;
	}
	
	Launch_Task_Schedule();
	Exit_Critical(irq_status);
}

//-------------------------------------任务延时函数-------------------------------------------//
void RTOS_Delay(uint32_t delay_ms)
{
	u32 irq_status;
	irq_status = Enter_Critical();
	Task_List[current_task_id].task_state = TASK_BLOCK;
	
	if(delay_ms != MAX_DELAY)  
	{
		Task_List[current_task_id].task_delay_ms = SysTick_count + delay_ms;
		List_insert(timer_list,&Task_List[current_task_id],order_insert);
	}
	else  
	{
		Task_List[current_task_id].task_pend_state = TASK_BLOCK;
	}
	
	List_remove_node(priority_list,&Task_List[current_task_id]);
	Launch_Task_Schedule();
	Exit_Critical(irq_status);
}

//---------------------------------操作系统初始化--------------------------------------------//
void RTOS_Init(void)
{
	u16 i;
	//*task_id,*task_name,task_state,task_priority,task_delay_ms,task_tick_count,task_stack_size_words,*task_function	
	Task_Create(&idle_task,"Idle_task",TASK_READY,0,MAX_DELAY,MAX_DELAY,50,Idle_task);
	
	Task_Create(&timer_guard_task_id,"timer_guard_task",TASK_BLOCK,0xFFFF,0,MAX_DELAY,100,soft_timer_guard_task);
	Task_Creat_Init();
	
	//*timer_id,timer_switch_flag,Timer_mode,timer_priority,user_tick_count,*timer_function
	Soft_Timer_Creat(&idle_softimer,stop_timer,repeat_mode,0,MAX_DELAY,Idle_task);
	
	//--------------初始化任务ID和关联任务列表与系统栈---------------//
	for(i=0;i<TASK_NUM;i++)
	{
		*(Task_List[i].task_id_point) = i;
		PSP_array[i] = ((unsigned int) Task_List[i].task_stack)+(Task_List[i].task_stack_size_words * sizeof(unsigned int))-16*4;
		HW32_REG((PSP_array[i] + (14<<2))) = (unsigned long) Task_List[i].task_function;
		HW32_REG((PSP_array[i] + (15<<2))) = 0x01000000;
	}
	
	Task_list_init();
	Softimer_List_init();
	
	//---------------初始化操作系统-------------------------------//
    miniRTOS_Init();
}


/*********************************************END OF FILE**********************/
