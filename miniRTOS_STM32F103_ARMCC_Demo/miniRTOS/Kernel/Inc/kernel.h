#ifndef __KERNEL_H
#define	__KERNEL_H

#include "miniRTOSconfig.h"
#include "miniRTOSport.h"

//---------------------------------------------------------------------//
#define MAX_DELAY         0xFFFFFFFF
#define	TASK_NUM          (USER_TASK_NUM+2)   //system task number is 2: idle_task ,timer_guard_task
#define Softimer_NUM      (USER_Softimer_NUM+2) //Softimer_NUM is USER_Softimer_NUM+ idle_softimer
#define HW32_REG(ADDERSS) (*((volatile unsigned long *)(ADDERSS)))

	
//---------------------------------------------------------------------//
typedef enum  
{
	Fail=0,
	Success,
}return_flag;

typedef enum  
{
	Handle_mode = 0x00,
	User_mode   = 0x01,
}MCU_mode;

typedef enum  
{
	TASK_BLOCK = 0x00,
	TASK_READY = 0x01,
}Task_state;

typedef enum  
{
	NO_Pend = 0x00,
	Pend = 0x01,
}Scheduler_state;

typedef enum  
{
	timer_list = 0,
	priority_list,
}List_type;

typedef enum  
{
	reverse_insert = 0,
	order_insert,
}List_insert_type;


//---------------------------------------------------------------------//
struct Task_node
{
			 uint16 *task_id_point;
			 char *task_name;
	volatile uint8 task_state;
	volatile uint8 task_pend_state;
			 uint16 task_priority; 
	volatile uint32 task_delay_ms;
	volatile uint32 task_tick_count;
			 uint32 task_tick_reload;
			 uint32 *task_stack;
			 uint16 task_stack_size_words;
			 void *task_function;
	struct Task_node* timer_list_next;
    struct Task_node* priority_list_next;	
};
typedef struct Task_node Task_Unit;

typedef uint16  Task_Handle;
extern Task_Unit Task_List[TASK_NUM];	
extern volatile uint32 PSP_array[TASK_NUM]; 
extern volatile uint32 SysTick_count;
extern volatile uint32 current_task_id;
extern volatile uint32 next_task_id;
extern volatile Scheduler_state scheduler_pend_flag;
extern Task_Handle idle_task;
extern Task_Unit *Timer_list_head;
extern Task_Unit *Priority_list_head;

//---------------------------------------------------------------------//
void RTOS_Init(void);
void RTOS_Delay(uint32 delay_ms);
void Task_Creat_Init(void);
void Release_Task(Task_Handle Task_x);
void Pend_Task(Task_Handle Task_x);					
void Pend_Schedule(void);
void Release_Schedule(void);	
void RTOS_Tick_IRQ(void);	

uint8 Task_Create( 
				uint16 *task_id,
				char *task_name,
				uint8 task_state,
				uint16 task_priority,
				uint32 task_delay_ms,
				uint32 task_tick_ms,
				uint16 task_stack_size_words,
				void *task_function	
			  );	
#endif 
