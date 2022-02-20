#include "miniRTOSport.h"
#include "kernel.h" 
#include "heap.h"

void Idle_task(void);

//------------------------------临界段代码保护-------------------------------------//
//进入临界段代码保护
uint32 Enter_Critical(void)
{
	uint32 irq_flag = __get_PRIMASK();
	__set_PRIMASK(0x01); //进入临界段代码保护，置PRIMASK 1bit寄存器1，屏蔽除NMI，硬fault以外的所有异常	
	return irq_flag;
}

//退出临界段代码保护
void Exit_Critical(uint32 irq_flag)
{
	__set_PRIMASK(irq_flag); //退出临界段代码保护，清除PRIMASK 1bit寄存器0，中断正常响应
}

void Disable_Interrupt(void)
{
	__disable_irq();
}

void Enable_Interrupt(void)
{
	__enable_irq();
}

//在hardfault中添加，出问题的是哪个任务，当前任务的栈完整性检查，MCU core寄存器值，当前任务栈信息打印
void Backtrace(uint32 psp)
{
	uint32 i;
	MemoryInforBlockNode *information;
    information = (MemoryInforBlockNode *) ((unsigned int)Task_List[current_task_id].task_stack - sizeof(MemoryInforBlockNode));
	mini_printf("============miniRTOS task information============\r\n");
	mini_printf("Current task name is %s\r\n",Task_List[current_task_id].task_name);  
	mini_printf("task stack size is %d words\r\n",Task_List[current_task_id].task_stack_size_words); 
	if(information->Isolate_Zone != Isolate_Zone_Flag) mini_printf("task stack is overflow\r\n");
	
	mini_printf("==========MCU core registers information=======\r\n");
	mini_printf("R0 : 0x%X\r\n",HW32_REG((0+psp)));
	mini_printf("R1 : 0x%X\r\n",HW32_REG((4+psp)));
	mini_printf("R2 : 0x%X\r\n",HW32_REG((8+psp)));
	mini_printf("R3 : 0x%X\r\n",HW32_REG((12+psp)));
	mini_printf("R12: 0x%X\r\n",HW32_REG((16+psp)));
	mini_printf("LR : 0x%X\r\n",HW32_REG((20+psp)));
	mini_printf("PC : 0x%X\r\n",HW32_REG((24+psp)));
	mini_printf("PSR: 0x%X\r\n",HW32_REG((28+psp)));
	mini_printf("PSP      : 0x%X\r\n",psp);
	mini_printf("MSP      : 0x%X\r\n",__get_MSP());
	mini_printf("CONTROL  : 0x%X\r\n",__get_CONTROL());
	mini_printf("BASEPRI  : 0x%X\r\n",__get_BASEPRI());
	mini_printf("PRIMASK  : 0x%X\r\n",__get_PRIMASK());
	mini_printf("FAULTMASK: 0x%X\r\n",__get_FAULTMASK());
	
	mini_printf("=============task stack==============\r\n");
    for(i=0;i<Task_List[current_task_id].task_stack_size_words;i++)
	mini_printf("addr:0x%X, data:0x%X\r\n",(uint32)(Task_List[current_task_id].task_stack+i), *(Task_List[current_task_id].task_stack+i));
}

void HardFault_Handler(void)
{
	Backtrace(__get_PSP());
	while(1);
}

void SysTick_Handler(void)
{
	RTOS_Tick_IRQ();
}

void Launch_Task_Schedule(void)
{
	if(scheduler_pend_flag == NO_Pend)
	{
		next_task_id = *(Priority_list_head->task_id_point);
		if(current_task_id != next_task_id)
		{
			if(((*Task_List[next_task_id].task_id_point) != start_task) && (((*Task_List[next_task_id].task_id_point) != idle_task)))
			{
				Task_List[next_task_id].task_tick_count = SysTick_count + Task_List[next_task_id].task_tick_reload;
			}
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		}
	}
}

void miniRTOS_Init(void)
{
	NVIC_SetPriority(PendSV_IRQn,0xFF);
	NVIC_SetPriority(SVCall_IRQn,0);
	SysTick_Config(SysTick_Rhythm*(OS_Core_Clock/1000000));
	__set_CONTROL(0x02);
	Start_task();
}

void Idle_Task_Hook(void)
{
	//mini_printf("Idle_Task_Hook\r\n");
	//__WFI();
}

//--------------------------------------------------------------//
void mini_sendchr(char ch)
{  
	UART_putch(ch);		
}
