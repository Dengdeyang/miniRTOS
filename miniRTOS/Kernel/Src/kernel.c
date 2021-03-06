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
#include "kernel.h" 
#include "list.h" 
#include "heap.h"
#include "miniRTOSport.h"
//------------------RTOS 任务列表，优先级列表，timer列表，系统栈及其他中间全局变量定义区--------//
Task_Unit Task_List[TASK_NUM]={0};
volatile uint32 PSP_array[TASK_NUM]; 
Task_Unit *Timer_list_head;
Task_Unit *Priority_list_head;
Task_Handle idle_task,start_task;

volatile uint32 SysTick_count = 0;
volatile uint32 current_task_id = 0;
volatile uint32 next_task_id = 1;
volatile Scheduler_state scheduler_pend_flag = NO_Pend;

void Idle_task(void);
void Task_Creat_Init(void);
void Start_task(void);

#define miniRTOS_VERSION      1
#define miniRTOS_MAINVERSION  0
#define miniRTOS_SUBVERSION   0

static void miniRTOS_Show_Vision(void)
{
	mini_printf("           /\\\r\n");
	mini_printf("__________/  \\__________\r\n");
	mini_printf("   ______/ /\\ \\______\r\n");
	mini_printf("        / /  \\ \\\r\n");
	mini_printf("       / /    \\ \\\r\n");	
	mini_printf("  %d.%d.%d build %s\r\n",miniRTOS_VERSION,miniRTOS_MAINVERSION,miniRTOS_SUBVERSION, __DATE__);
    mini_printf("2020 - 2023 Copyright by ddy\r\n");	
    mini_printf("miniRTOS - mini Real Timer Operating System\r\n");
}

//---------------------------------RTOS 任务创建函数-----------------------------------------//
/**
  * @brief  动态申请任务栈内存，并对任务列表中对应的任务结构体变量进行初始化。
  * @param  uint16 *task_id:              任务id变量地址
			char *task_name:           任务名称，字符串
			uint8 task_state:            任务状态
			uint16 task_priority:         任务优先级
			uint32 task_delay_ms:         任务延时值
			uint16 task_stack_size_words: 任务栈大小(单位:32bit word)
			void *task_function:       任务函数地址
  * @retval 任务创建成功或失败标志                       
  */
uint8 Task_Create( 
				uint16 *task_id,
				char *task_name,
				uint8 task_state,
				uint16 task_priority,
				uint32 task_delay_ms,
				uint32 task_tick_ms,
				uint16 task_stack_size_words,
				void *task_function	
			  )
{
	uint32 i;
	static uint16 task_id_index=0;
	
	configASSERT(task_id_index < TASK_NUM);
	
	Task_List[task_id_index].task_id_point = task_id;
	Task_List[task_id_index].task_name = task_name;
	Task_List[task_id_index].task_state = task_state;
	Task_List[task_id_index].task_pend_state = task_state;
	Task_List[task_id_index].task_priority = task_priority;
	Task_List[task_id_index].task_delay_ms = task_delay_ms;
	Task_List[task_id_index].task_tick_reload = task_tick_ms;
	Task_List[task_id_index].task_tick_count = task_tick_ms;
	Task_List[task_id_index].task_stack = (uint32 *) mini_malloc((task_stack_size_words)*sizeof(uint32));
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
	uint32 irq_status;
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
	uint32 irq_status;
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
	uint32 irq_status;
	irq_status = Enter_Critical();
	scheduler_pend_flag = Pend;
	Exit_Critical(irq_status);
}

//------------------------------调度器恢复--------------------------------------------//
void Release_Schedule(void)
{
	uint32 irq_status;
	irq_status = Enter_Critical();
	scheduler_pend_flag = NO_Pend;
	Exit_Critical(irq_status);
}

//-------------------------------RTOS心跳中断服务函数--------------------------------------//
void RTOS_Tick_IRQ(void)
{
	uint32 irq_status;
	irq_status = Enter_Critical();
	SysTick_count++;
   
	if((SysTick_count > Task_List[current_task_id].task_tick_count) && \
	   (Task_List[current_task_id].task_state == TASK_READY) && \
	   (Task_List[current_task_id].priority_list_next->task_priority == Task_List[current_task_id].task_priority))
	{
		if(((*Task_List[current_task_id].task_id_point) != start_task) && (((*Task_List[current_task_id].task_id_point) != idle_task)))
		{
			List_insert(priority_list,&Task_List[current_task_id],reverse_insert);
		}
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
void RTOS_Delay(uint32 delay_ms)
{
	uint32 irq_status;
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
	uint16 i;
	//*task_id,*task_name,task_state,task_priority,task_delay_ms,task_tick_count,task_stack_size_words,*task_function	
	Task_Create(&start_task,"start_task",TASK_READY,0,MAX_DELAY,MAX_DELAY,50,Start_task);
	Task_Create(&idle_task,"Idle_task",TASK_READY,0,MAX_DELAY,MAX_DELAY,100,Idle_task);
	Task_Create(&timer_guard_task_id,"timer_guard_task",TASK_BLOCK,0xFFFF,0,MAX_DELAY,100,soft_timer_guard_task);
	Task_Creat_Init();
	
	//*timer_id,timer_switch_flag,Timer_mode,timer_priority,user_tick_count,*timer_function
	Soft_Timer_Creat(&idle_softimer,stop_timer,repeat_mode,0,MAX_DELAY,Idle_task);
	
	//--------------初始化任务ID和关联任务列表与系统栈---------------//
	for(i=0;i<TASK_NUM;i++)
	{
		*(Task_List[i].task_id_point) = i;
		PSP_array[i] = ((uint32) Task_List[i].task_stack)+(Task_List[i].task_stack_size_words * sizeof(uint32))-16*4;
		HW32_REG((PSP_array[i] + (14<<2))) = (unsigned long) Task_List[i].task_function;
		HW32_REG((PSP_array[i] + (15<<2))) = 0x01000000;
	}
	Task_list_init();
	Softimer_List_init();
	//--------------初始化操作系统-------------------------------//
	Disable_Interrupt();
	Pend_Schedule();
	miniRTOS_Show_Vision();
	current_task_id = start_task;
	__set_PSP((PSP_array[current_task_id] + 16*4));
    miniRTOS_Init();
}

//---------------启动任务----------------//
void Start_task(void)
{
	Pend_Task(start_task);
	Enable_Interrupt();
	Release_Schedule();
	while(1)
	{
		
	}
}

//---------------空闲任务----------------//
void Idle_task(void)
{
	while(1)
	{
		Idle_Task_Hook();
	}
}

/*********************************************END OF FILE**********************/
