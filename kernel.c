/********************************************************************************
  * @file    kernel.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   RTOS内核开发，包含：
  *								1)任务创建函数
  *                             2)临界段代码保护函数
  *								3)任务挂起与恢复函数
  *                             4)RTOS心跳滴答定时器中断服务函数(优先级判决+抢占式任务调度)
  *                             5)RTOS延时函数(释放CPU使用权)
  *                             6)RTOS初始化函数
  * @atteration   启动文件.s中，堆区必须定义较大，在任务创建时，满足任务栈较大或任务数量较多时malloc申请
  ********************************************************************************/
#include "stm32f10x.h"
#include "kernel.h" 
#include "stdlib.h"
#include "list.h" 

//------------------RTOS 任务列表，优先级列表，timer列表，系统栈及其他中间全局变量定义区--------//
Task_Unit Task_List[TASK_NUM]={0};
volatile uint32_t PSP_array[TASK_NUM]; 

Task_Unit *Timer_list_head;
Task_Unit *Priority_list_head;
Task_Handle idle_task;

volatile uint16_t Critical_count = 0;
volatile uint32_t SysTick_count = 0;
volatile uint32_t current_task_id = 0;
volatile uint32_t next_task_id = 1;
volatile uint32_t current_task_sp;
volatile uint32_t next_task_sp;

void Idle_task(void);
void Task_Creat_Init(void);

int __svc(0x00) SVC_ENTER_CRITICAL(void);
int __svc(0x01) SVC_TASK_SCHEDULE(void);


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
				u16 task_stack_size_words,
				void *task_function	
			  )
{
	static u16 task_id_index=0;
	
	configASSERT(task_id_index < TASK_NUM);
	
	Task_List[task_id_index].task_id_point = task_id;
	Task_List[task_id_index].task_name = task_name;
	Task_List[task_id_index].task_state = task_state;
	Task_List[task_id_index].task_pend_state = task_state;
	Task_List[task_id_index].task_priority = task_priority;
	Task_List[task_id_index].task_delay_ms = task_delay_ms;
	Task_List[task_id_index].task_stack = (unsigned int *) calloc(task_stack_size_words,sizeof(unsigned int));
	Task_List[task_id_index].task_stack_size_words = task_stack_size_words;
	Task_List[task_id_index].task_function = task_function;
	if(Task_List[task_id_index].task_stack != NULL) 
	{
		task_id_index++;
		return Success;
	}
	else
	{
		printf("Not enough memory for task stack!\r\n");
		return Fail;
	}
}

//---------------------------------任务调度函数-------------------------------------------//
static void Task_Schedule(void)
{
    next_task_id = *(Priority_list_head->task_id_point);
	if(current_task_id != next_task_id)
	{
        current_task_sp = (uint32_t)&PSP_array[current_task_id];
        next_task_sp    = (uint32_t)&PSP_array[next_task_id];
        current_task_id = next_task_id;
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	}
}

//------------------------------ARM模式自动识别任务调度-------------------------------------//
//在非特权模式下，用户无法直接呼叫PendSV请求，需要通过SVC呼叫PendSV
void Launch_Task_Schedule(void)
{
    if((__get_CONTROL() & 0x0F) == User_mode)  SVC_TASK_SCHEDULE();
	else 									   Task_Schedule();
}
	
//------------------------------RTOS任务的挂起--------------------------------------------//
void Pend_Task(Task_Handle Task_x)
{
	taskENTER_CRITICAL();
	Task_List[Task_x].task_pend_state = TASK_BLOCK;
	Task_List[Task_x].task_state = TASK_BLOCK;
	List_remove_node(priority_list,&Task_List[Task_x]);
	Launch_Task_Schedule();
	taskEXIT_CRITICAL();
}

//------------------------------RTOS任务恢复-----------------------------------------------//
void Release_Task(Task_Handle Task_x)
{
	taskENTER_CRITICAL();
	Task_List[Task_x].task_pend_state = TASK_READY;
	Task_List[Task_x].task_state = TASK_READY;
	List_insert(priority_list,&Task_List[Task_x]);
	Launch_Task_Schedule();
	taskEXIT_CRITICAL();
}

//-------------------------------RTOS心跳中断服务函数--------------------------------------//
void SysTick_Handler(void)
{
	taskENTER_CRITICAL();
	SysTick_count++;
    
	if(SysTick_count > Softimer_list_head->timer_count)
	{
		Softimer_list_head->timer_state = run_timer;
		if(Softimer_list_head->timer_runflag == run_timer) Release_Task(timer_guard_task_id);
		if(Softimer_list_head->softimer_next != NULL) Softimer_list_head = Softimer_list_head->softimer_next;
	}

	if(SysTick_count > Timer_list_head->task_delay_ms)
	{
		Timer_list_head->task_state = TASK_READY;
		List_insert(priority_list,Timer_list_head);
		if(Timer_list_head->timer_list_next != NULL) Timer_list_head = Timer_list_head->timer_list_next;
	}
	Launch_Task_Schedule();
	taskEXIT_CRITICAL();
}

//-------------------------------------任务延时函数-------------------------------------------//
void RTOS_Delay(uint32_t delay_ms)
{
	taskENTER_CRITICAL();
	Task_List[current_task_id].task_state = TASK_BLOCK;
	
	if(delay_ms != MAX_DELAY)  
	{
		Task_List[current_task_id].task_delay_ms = SysTick_count + delay_ms;
		List_insert(timer_list,&Task_List[current_task_id]);
	}
	else  
	{
		Task_List[current_task_id].task_pend_state = TASK_BLOCK;
	}
	
	List_remove_node(priority_list,&Task_List[current_task_id]);
	Launch_Task_Schedule();
	taskEXIT_CRITICAL();
}

//---------------------------------操作系统初始化--------------------------------------------//	
void RTOS_Init(void)
{
	u16 i;
	
	//*task_id,*task_name,task_state,task_priority,task_delay_ms,task_stack_size_words,*task_function	
	Task_Create(&idle_task,"Idle_task",TASK_READY,0,MAX_DELAY,50,Idle_task);
	Task_Create(&timer_guard_task_id,"timer_guard_task",TASK_BLOCK,0xFFFF,0,100,soft_timer_guard_task);
	Task_Creat_Init();
	
	//*timer_id,timer_switch_flag,Timer_mode,timer_priority,user_tick_count,*timer_function
	Soft_Timer_Creat(&idle_softimer,stop_timer,repeat_mode,0,MAX_DELAY,Idle_task);

	SCB->CCR = SCB_CCR_STKALIGN;//使能双字栈对齐
	
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
	current_task_id = 0;
	__set_PSP((PSP_array[current_task_id] + 16*4));
	NVIC_SetPriority(PendSV_IRQn,0xFF);
	NVIC_SetPriority(SVCall_IRQn,0);
	SysTick_Config(SysTick_Rhythm*(SystemCoreClock/1000000));
	__set_CONTROL(0x03);
	Idle_task();
}

//---------------------------------PendSV中断服务函数,任务上下文切换------------------------------------//	
__ASM void PendSV_Handler(void)
{
    //----------现场保护----------------//
    MRS R0 , PSP //把PSP值读到R0
	STMDB R0!,{R4 - R11}//R4~R11中的数据依次存入PSP所指地址处，每存一次R0更新地址，记录PSP当前值
    LDR R1,=__cpp(&current_task_sp)//R1=current_task_sp的地址
    LDR R2,[R1] //R2 = current_task_sp，即R2 = &PSP_array[current_task_id];
    STR R0,[R2] //将当前PSP值存储到 &PSP_array[current_task_id];
    
    //----------现场恢复----------------//
    LDR R3,=__cpp(&next_task_sp)//R3=next_task_sp的地址
    LDR R4,[R3] //R4 = next_task_sp，即R4 = &PSP_array[next_task_id];
    LDR R0,[R4] //将&PSP_array[current_task_id]处的值赋给R0，R0为下一个任务的私有栈地址
	LDMIA R0!,{R4 - R11}//PSP所指地址处读出数据加载到R4~R11中，每存一次R0更新地址，记录PSP当前值
	MSR PSP,R0  //R0所指地址加载到PSP
    
    //----------跳转----------------//
	BX LR      //PC返回到LR所指处，该函数调用前地址
	ALIGN 4    //下一条指令或数据对齐到4字节地址处，空位补0
}

//------------------------SVC中断回调函数，根据呼叫SVC编号，选择执行对应的操作--------------------//
void SVC_Handler_C(unsigned int *svc_args)
{
	unsigned int svc_number;
	svc_args[0] = svc_args[0] + svc_args[1];
	svc_number = ((char *)svc_args[6])[-2];
	switch(svc_number)
	{
		case 0x00://SVC 0号服务请求执行内容
		{
			__set_CONTROL(0x00); //ARM在Handler模式，设置CONTROL[0]=0,进入特权级，线程模式
			__set_PRIMASK(0x01); //进入临界段代码保护，置PRIMASK 1bit寄存器1，屏蔽除NMI，硬fault以外的所有异常
		};break;
		
		case 0x01://SVC 1号服务请求执行内容
		{
			Task_Schedule();//RTOS_Delay 函数呼叫一次系统请求，进行任务调度
		};break;
		
		default:break;
	}
}

//------------------------SVC中断函数------------------//
__ASM void SVC_Handler(void)
{ 
	TST LR,#4;
	ITE EQ
	MRSEQ R0,MSP;//判断使用的是哪一种栈指针，读取对应的栈指针
	MRSNE R0,PSP; 
	B __cpp(SVC_Handler_C)//跳转到SVC中断函数的回调函数
	ALIGN 4
}
 
//------------------------------临界段代码保护-------------------------------------//
//进入临界段代码保护
void taskENTER_CRITICAL(void)
{
	if(Critical_count == 0)
	{
		if((__get_CONTROL() & 0x0F) == User_mode)  
		{
			SVC_ENTER_CRITICAL();
		}
		else 									   
		{
			__set_PRIMASK(0x01); //进入临界段代码保护，置PRIMASK 1bit寄存器1，屏蔽除NMI，硬fault以外的所有异常
		}
	}
	Critical_count++;
}

//退出临界段代码保护
void taskEXIT_CRITICAL(void)
{
	if(Critical_count == 1)
	{
		__set_PRIMASK(0x00); //退出临界段代码保护，清除PRIMASK 1bit寄存器0，中断正常响应
		__set_CONTROL(0x03);
	}
	Critical_count--;
}
/*********************************************END OF FILE**********************/
