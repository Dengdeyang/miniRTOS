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
#include "mini_libc.h"
#include "heap.h"
//------------------RTOS 任务列表，优先级列表，timer列表，系统栈及其他中间全局变量定义区--------//
Task_Unit Task_List[TASK_NUM]={0};
volatile uint32_t PSP_array[TASK_NUM]; 

Task_Unit *Timer_list_head;
Task_Unit *Priority_list_head;
Task_Handle idle_task;

volatile int Critical_count = 0;
volatile uint32_t SysTick_count = 0;
volatile uint32_t current_task_id = 0;
volatile uint32_t next_task_id = 1;
volatile Scheduler_state scheduler_pend_flag = NO_Pend;

void Idle_task(void);
void Task_Creat_Init(void);

int __svc(0x00) SVC_TASK_START(void);


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

//---------------------------------任务调度函数-------------------------------------------//
void Launch_Task_Schedule(void)
{
	if(scheduler_pend_flag == NO_Pend)
	{
		next_task_id = *(Priority_list_head->task_id_point);
		if(current_task_id != next_task_id)
		{
			Task_List[next_task_id].task_tick_count = SysTick_count + Task_List[next_task_id].task_tick_reload;
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		}
	}
}
	
//------------------------------RTOS任务的挂起--------------------------------------------//
void Pend_Task(Task_Handle Task_x)
{
	Enter_Critical();
	Task_List[Task_x].task_pend_state = TASK_BLOCK;
	Task_List[Task_x].task_state = TASK_BLOCK;
	List_remove_node(priority_list,&Task_List[Task_x]);
	Launch_Task_Schedule();
	Exit_Critical();
}

//------------------------------RTOS任务恢复-----------------------------------------------//
void Release_Task(Task_Handle Task_x)
{
	Enter_Critical();
	Task_List[Task_x].task_pend_state = TASK_READY;
	Task_List[Task_x].task_state = TASK_READY;
	List_insert(priority_list,&Task_List[Task_x],order_insert);
	Launch_Task_Schedule();
	Exit_Critical();
}

//------------------------------调度器挂起--------------------------------------------//
void Pend_Schedule(void)
{
	Enter_Critical();
	scheduler_pend_flag = Pend;
	Exit_Critical();
}

//------------------------------调度器恢复--------------------------------------------//
void Release_Schedule(void)
{
	Enter_Critical();
	scheduler_pend_flag = NO_Pend;
	Exit_Critical();
}

//-------------------------------RTOS心跳中断服务函数--------------------------------------//
void SysTick_Handler(void)
{
	Enter_Critical();
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
	Exit_Critical();
}

//-------------------------------------任务延时函数-------------------------------------------//
void RTOS_Delay(uint32_t delay_ms)
{
	Enter_Critical();
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
	Exit_Critical();
}

//---------------------------------操作系统初始化--------------------------------------------//	
void RTOS_Init(void)
{
	u16 i;
	Enter_Critical();
	//*task_id,*task_name,task_state,task_priority,task_delay_ms,task_tick_count,task_stack_size_words,*task_function	
	Task_Create(&idle_task,"Idle_task",TASK_READY,0,MAX_DELAY,MAX_DELAY,50,Idle_task);
	Task_Create(&timer_guard_task_id,"timer_guard_task",TASK_BLOCK,0xFFFF,0,MAX_DELAY,100,soft_timer_guard_task);
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
	Exit_Critical();
	__set_CONTROL(0x02);
	Idle_task();
}

//---------------------------------PendSV中断服务函数,任务上下文切换------------------------------------//	

__ASM void PendSV_Handler(void)//有硬件fault
{
	cpsid i
    MRS R0 , PSP //把PSP值读到R0
	STMDB R0!,{R4 - R11}//R4~R11中的数据依次存入PSP所指地址处，每存一次R0更新地址，记录PSP当前值
	LDR R1,=__cpp(&current_task_id);//把C语言中全局变量current_task_id的地址存入R1，汇编调用C变量需用cpp函数
	LDR R2,[R1]//将current_task_id数值存入R2中
	LDR R3, =__cpp(&PSP_array);//把C语言中全局变量PSP_array的地址存入R1，汇编调用C变量需用cpp函数
	STR R0,[R3,R2,LSL #2]//R0数据加载到地址=R3+R2<<2处(PSP当前地址存到PSP_array+current_task_id*2处)
	
	LDR R4,=__cpp(&next_task_id);//把C语言中全局变量next_task_id的地址存入R4，汇编调用C变量需用cpp函数
	LDR R4,[R4]//将next_task_id数值存入R4中
	STR R4,[R1]//将current_task_id = next_task_id
	LDR R0,[R3,R4,LSL #2]//将地址=R3+R2<<2处数据加载到R0，(PSP_array+next_task_id*2处数据加载到PSP处)
	LDMIA R0!,{R4 - R11}//PSP所指地址处读出数据加载到R4~R11中，每存一次R0更新地址，记录PSP当前值
	MSR PSP,R0  //R0所指地址加载到PSP
	cpsie i
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
void Enter_Critical(void)
{
	if(Critical_count == 0)  __set_PRIMASK(0x01); //进入临界段代码保护，置PRIMASK 1bit寄存器1，屏蔽除NMI，硬fault以外的所有异常
	Critical_count++;
}

//退出临界段代码保护
void Exit_Critical(void)
{
	if(Critical_count == 1) __set_PRIMASK(0x00); //退出临界段代码保护，清除PRIMASK 1bit寄存器0，中断正常响应
	Critical_count--;
}

//在hardfault中添加，出问题的是哪个任务，当前任务的栈完整性检查，MCU core寄存器值，当前任务栈信息打印
void task_debug(void)
{
	u32 i;
	u32 addr;
	MemoryInforBlockNode *information;
    information = (MemoryInforBlockNode *) ((unsigned int)Task_List[current_task_id].task_stack - sizeof(MemoryInforBlockNode));
	mini_printf("============miniRTOS task information============\r\n");
	mini_printf("Current task name is %s\r\n",Task_List[current_task_id].task_name);  
	mini_printf("task stack size is %d words\r\n",Task_List[current_task_id].task_stack_size_words); 
	if(information->Isolate_Zone != Isolate_Zone_Flag) mini_printf("task stack is overflow\r\n");
	
	mini_printf("==========MCU core registers information=======\r\n");
	addr = __get_PSP();
	mini_printf("R0 : 0x%X\r\n",HW32_REG((0+addr)));
	mini_printf("R1 : 0x%X\r\n",HW32_REG((4+addr)));
	mini_printf("R2 : 0x%X\r\n",HW32_REG((8+addr)));
	mini_printf("R3 : 0x%X\r\n",HW32_REG((12+addr)));
	mini_printf("R12: 0x%X\r\n",HW32_REG((16+addr)));
	mini_printf("LR : 0x%X\r\n",HW32_REG((20+addr)));
	mini_printf("PC : 0x%X\r\n",HW32_REG((24+addr)));
	mini_printf("PSR: 0x%X\r\n",HW32_REG((28+addr)));
	mini_printf("PSP      : 0x%X\r\n",addr);
	mini_printf("MSP      : 0x%X\r\n",__get_MSP());
	mini_printf("CONTROL  : 0x%X\r\n",__get_CONTROL());
	mini_printf("BASEPRI  : 0x%X\r\n",__get_BASEPRI());
	mini_printf("PRIMASK  : 0x%X\r\n",__get_PRIMASK());
	mini_printf("FAULTMASK: 0x%X\r\n",__get_FAULTMASK());
	
	mini_printf("=============task stack==============\r\n");
    for(i=0;i<Task_List[current_task_id].task_stack_size_words;i++)
	mini_printf("addr:0x%X, data:0x%X\r\n",(u32)(Task_List[current_task_id].task_stack+i), *(Task_List[current_task_id].task_stack+i));
}
/*********************************************END OF FILE**********************/
