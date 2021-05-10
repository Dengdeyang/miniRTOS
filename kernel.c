/********************************************************************************
  * @file    kernel.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   RTOS�ں˿�����������
  *								1)���񴴽�����
  *                             2)�ٽ�δ��뱣������
  *								3)���������ָ�����
  *                             4)RTOS�����δ�ʱ���жϷ�����(���ȼ��о�+��ռʽ�������)
  *                             5)RTOS��ʱ����(�ͷ�CPUʹ��Ȩ)
  *                             6)RTOS��ʼ������
  * @atteration   �����ļ�.s�У��������붨��ϴ������񴴽�ʱ����������ջ�ϴ�����������϶�ʱmalloc����
  ********************************************************************************/
#include "stm32f10x.h"
#include "kernel.h" 
#include "stdlib.h"
#include "list.h" 

//------------------RTOS �����б����ȼ��б�timer�б�ϵͳջ�������м�ȫ�ֱ���������--------//
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

//---------------------------------������Ⱥ���-------------------------------------------//
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

//------------------------------ARMģʽ�Զ�ʶ���������-------------------------------------//
//�ڷ���Ȩģʽ�£��û��޷�ֱ�Ӻ���PendSV������Ҫͨ��SVC����PendSV
void Launch_Task_Schedule(void)
{
    if((__get_CONTROL() & 0x0F) == User_mode)  SVC_TASK_SCHEDULE();
	else 									   Task_Schedule();
}
	
//------------------------------RTOS����Ĺ���--------------------------------------------//
void Pend_Task(Task_Handle Task_x)
{
	taskENTER_CRITICAL();
	Task_List[Task_x].task_pend_state = TASK_BLOCK;
	Task_List[Task_x].task_state = TASK_BLOCK;
	List_remove_node(priority_list,&Task_List[Task_x]);
	Launch_Task_Schedule();
	taskEXIT_CRITICAL();
}

//------------------------------RTOS����ָ�-----------------------------------------------//
void Release_Task(Task_Handle Task_x)
{
	taskENTER_CRITICAL();
	Task_List[Task_x].task_pend_state = TASK_READY;
	Task_List[Task_x].task_state = TASK_READY;
	List_insert(priority_list,&Task_List[Task_x]);
	Launch_Task_Schedule();
	taskEXIT_CRITICAL();
}

//-------------------------------RTOS�����жϷ�����--------------------------------------//
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

//-------------------------------------������ʱ����-------------------------------------------//
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

//---------------------------------����ϵͳ��ʼ��--------------------------------------------//	
void RTOS_Init(void)
{
	u16 i;
	
	//*task_id,*task_name,task_state,task_priority,task_delay_ms,task_stack_size_words,*task_function	
	Task_Create(&idle_task,"Idle_task",TASK_READY,0,MAX_DELAY,50,Idle_task);
	Task_Create(&timer_guard_task_id,"timer_guard_task",TASK_BLOCK,0xFFFF,0,100,soft_timer_guard_task);
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
	__set_CONTROL(0x03);
	Idle_task();
}

//---------------------------------PendSV�жϷ�����,�����������л�------------------------------------//	
__ASM void PendSV_Handler(void)
{
    //----------�ֳ�����----------------//
    MRS R0 , PSP //��PSPֵ����R0
	STMDB R0!,{R4 - R11}//R4~R11�е��������δ���PSP��ָ��ַ����ÿ��һ��R0���µ�ַ����¼PSP��ǰֵ
    LDR R1,=__cpp(&current_task_sp)//R1=current_task_sp�ĵ�ַ
    LDR R2,[R1] //R2 = current_task_sp����R2 = &PSP_array[current_task_id];
    STR R0,[R2] //����ǰPSPֵ�洢�� &PSP_array[current_task_id];
    
    //----------�ֳ��ָ�----------------//
    LDR R3,=__cpp(&next_task_sp)//R3=next_task_sp�ĵ�ַ
    LDR R4,[R3] //R4 = next_task_sp����R4 = &PSP_array[next_task_id];
    LDR R0,[R4] //��&PSP_array[current_task_id]����ֵ����R0��R0Ϊ��һ�������˽��ջ��ַ
	LDMIA R0!,{R4 - R11}//PSP��ָ��ַ���������ݼ��ص�R4~R11�У�ÿ��һ��R0���µ�ַ����¼PSP��ǰֵ
	MSR PSP,R0  //R0��ָ��ַ���ص�PSP
    
    //----------��ת----------------//
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
			__set_CONTROL(0x00); //ARM��Handlerģʽ������CONTROL[0]=0,������Ȩ�����߳�ģʽ
			__set_PRIMASK(0x01); //�����ٽ�δ��뱣������PRIMASK 1bit�Ĵ���1�����γ�NMI��Ӳfault����������쳣
		};break;
		
		case 0x01://SVC 1�ŷ�������ִ������
		{
			Task_Schedule();//RTOS_Delay ��������һ��ϵͳ���󣬽����������
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
			__set_PRIMASK(0x01); //�����ٽ�δ��뱣������PRIMASK 1bit�Ĵ���1�����γ�NMI��Ӳfault����������쳣
		}
	}
	Critical_count++;
}

//�˳��ٽ�δ��뱣��
void taskEXIT_CRITICAL(void)
{
	if(Critical_count == 1)
	{
		__set_PRIMASK(0x00); //�˳��ٽ�δ��뱣�������PRIMASK 1bit�Ĵ���0���ж�������Ӧ
		__set_CONTROL(0x03);
	}
	Critical_count--;
}
/*********************************************END OF FILE**********************/
