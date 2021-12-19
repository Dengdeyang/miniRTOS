/********************************************************************************
  * @file    kernel.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   RTOS�ں˿�����������
  *								1)���񴴽�����
  *                             2)�ٽ�δ��뱣������
  *								3)���������ָ�����
  *								4)�������Ĺ�����ָ�����
  *                             5)RTOS�����δ�ʱ���жϷ�����(���ȼ��о�+��ռʽ�������)
  *                             6)RTOS��ʱ����(�ͷ�CPUʹ��Ȩ)
  *                             7)RTOS��ʼ������
  * @atteration   �����ļ�.s�У��������붨��ϴ������񴴽�ʱ����������ջ�ϴ�����������϶�ʱmini_malloc����
  ********************************************************************************/
#include "stm32f10x.h"
#include "kernel.h" 
#include "list.h" 
#include "mini_libc.h"
#include "heap.h"
//------------------RTOS �����б����ȼ��б�timer�б�ϵͳջ�������м�ȫ�ֱ���������--------//
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


//---------------------------------RTOS ���񴴽�����-----------------------------------------//
/**
  * @brief  ��̬��������ջ�ڴ棬���������б��ж�Ӧ������ṹ��������г�ʼ����
  * @param  u16 *task_id:              ����id������ַ
			char *task_name:           �������ƣ��ַ���
			u8 task_state:            ����״̬
			u16 task_priority:         �������ȼ�
			u32 task_delay_ms:         ������ʱֵ
			u16 task_stack_size_words: ����ջ��С(��λ:32bit word)
			void *task_function:       ��������ַ
  * @retval ���񴴽��ɹ���ʧ�ܱ�־                       
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

//---------------------------------������Ⱥ���-------------------------------------------//
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
	
//------------------------------RTOS����Ĺ���--------------------------------------------//
void Pend_Task(Task_Handle Task_x)
{
	Enter_Critical();
	Task_List[Task_x].task_pend_state = TASK_BLOCK;
	Task_List[Task_x].task_state = TASK_BLOCK;
	List_remove_node(priority_list,&Task_List[Task_x]);
	Launch_Task_Schedule();
	Exit_Critical();
}

//------------------------------RTOS����ָ�-----------------------------------------------//
void Release_Task(Task_Handle Task_x)
{
	Enter_Critical();
	Task_List[Task_x].task_pend_state = TASK_READY;
	Task_List[Task_x].task_state = TASK_READY;
	List_insert(priority_list,&Task_List[Task_x],order_insert);
	Launch_Task_Schedule();
	Exit_Critical();
}

//------------------------------����������--------------------------------------------//
void Pend_Schedule(void)
{
	Enter_Critical();
	scheduler_pend_flag = Pend;
	Exit_Critical();
}

//------------------------------�������ָ�--------------------------------------------//
void Release_Schedule(void)
{
	Enter_Critical();
	scheduler_pend_flag = NO_Pend;
	Exit_Critical();
}

//-------------------------------RTOS�����жϷ�����--------------------------------------//
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

//-------------------------------------������ʱ����-------------------------------------------//
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

//---------------------------------����ϵͳ��ʼ��--------------------------------------------//	
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

	SCB->CCR = SCB_CCR_STKALIGN;//ʹ��˫��ջ����
	
	//--------------��ʼ������ID�͹��������б���ϵͳջ---------------//
	for(i=0;i<TASK_NUM;i++)
	{
		*(Task_List[i].task_id_point) = i;
		PSP_array[i] = ((unsigned int) Task_List[i].task_stack)+(Task_List[i].task_stack_size_words * sizeof(unsigned int))-16*4;
		HW32_REG((PSP_array[i] + (14<<2))) = (unsigned long) Task_List[i].task_function;
		HW32_REG((PSP_array[i] + (15<<2))) = 0x01000000;
	}
	
	Task_list_init();
	Softimer_List_init();
	
	//---------------��ʼ������ϵͳ-------------------------------//
	current_task_id = 0;
	__set_PSP((PSP_array[current_task_id] + 16*4));
	NVIC_SetPriority(PendSV_IRQn,0xFF);
	NVIC_SetPriority(SVCall_IRQn,0);
	SysTick_Config(SysTick_Rhythm*(SystemCoreClock/1000000));
	Exit_Critical();
	__set_CONTROL(0x02);
	Idle_task();
}

//---------------------------------PendSV�жϷ�����,�����������л�------------------------------------//	

__ASM void PendSV_Handler(void)//��Ӳ��fault
{
	cpsid i
    MRS R0 , PSP //��PSPֵ����R0
	STMDB R0!,{R4 - R11}//R4~R11�е��������δ���PSP��ָ��ַ����ÿ��һ��R0���µ�ַ����¼PSP��ǰֵ
	LDR R1,=__cpp(&current_task_id);//��C������ȫ�ֱ���current_task_id�ĵ�ַ����R1��������C��������cpp����
	LDR R2,[R1]//��current_task_id��ֵ����R2��
	LDR R3, =__cpp(&PSP_array);//��C������ȫ�ֱ���PSP_array�ĵ�ַ����R1��������C��������cpp����
	STR R0,[R3,R2,LSL #2]//R0���ݼ��ص���ַ=R3+R2<<2��(PSP��ǰ��ַ�浽PSP_array+current_task_id*2��)
	
	LDR R4,=__cpp(&next_task_id);//��C������ȫ�ֱ���next_task_id�ĵ�ַ����R4��������C��������cpp����
	LDR R4,[R4]//��next_task_id��ֵ����R4��
	STR R4,[R1]//��current_task_id = next_task_id
	LDR R0,[R3,R4,LSL #2]//����ַ=R3+R2<<2�����ݼ��ص�R0��(PSP_array+next_task_id*2�����ݼ��ص�PSP��)
	LDMIA R0!,{R4 - R11}//PSP��ָ��ַ���������ݼ��ص�R4~R11�У�ÿ��һ��R0���µ�ַ����¼PSP��ǰֵ
	MSR PSP,R0  //R0��ָ��ַ���ص�PSP
	cpsie i
	BX LR      //PC���ص�LR��ָ�����ú�������ǰ��ַ
	ALIGN 4    //��һ��ָ������ݶ��뵽4�ֽڵ�ַ������λ��0
}


//------------------------SVC�жϻص����������ݺ���SVC��ţ�ѡ��ִ�ж�Ӧ�Ĳ���--------------------//
void SVC_Handler_C(unsigned int *svc_args)
{
	unsigned int svc_number;
	svc_args[0] = svc_args[0] + svc_args[1];
	svc_number = ((char *)svc_args[6])[-2];
	switch(svc_number)
	{
		case 0x00://SVC 0�ŷ�������ִ������
		{
			
		};break;
		
		default:break;
	}
}

//------------------------SVC�жϺ���------------------//
__ASM void SVC_Handler(void)
{ 
	TST LR,#4;
	ITE EQ
	MRSEQ R0,MSP;//�ж�ʹ�õ�����һ��ջָ�룬��ȡ��Ӧ��ջָ��
	MRSNE R0,PSP; 
	B __cpp(SVC_Handler_C)//��ת��SVC�жϺ����Ļص�����
	ALIGN 4
}
 
//------------------------------�ٽ�δ��뱣��-------------------------------------//
//�����ٽ�δ��뱣��
void Enter_Critical(void)
{
	if(Critical_count == 0)  __set_PRIMASK(0x01); //�����ٽ�δ��뱣������PRIMASK 1bit�Ĵ���1�����γ�NMI��Ӳfault����������쳣
	Critical_count++;
}

//�˳��ٽ�δ��뱣��
void Exit_Critical(void)
{
	if(Critical_count == 1) __set_PRIMASK(0x00); //�˳��ٽ�δ��뱣�������PRIMASK 1bit�Ĵ���0���ж�������Ӧ
	Critical_count--;
}

//��hardfault����ӣ�����������ĸ����񣬵�ǰ�����ջ�����Լ�飬MCU core�Ĵ���ֵ����ǰ����ջ��Ϣ��ӡ
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
