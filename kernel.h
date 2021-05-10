#ifndef __KERNEL_H
#define	__KERNEL_H

#include "stdio.h"
//----------------����ϵͳ�ü�--------------------
#define	USER_TASK_NUM        8     //�û�ָ��������(������������)
#define USER_Softimer_NUM    3     //�û�ָ�������ʱ������
#define SysTick_Rhythm       1000  //�û�ָ������ϵͳ����ʱ�����ڣ���λus

//--------------------------------------------
#define MAX_DELAY         0xFFFFFFFF
#define	TASK_NUM          (USER_TASK_NUM+2)   //system task number is 2: idle_task ,timer_guard_task
#define Softimer_NUM      (USER_Softimer_NUM+1) //Softimer_NUM is USER_Softimer_NUM+ idle_softimer
#define HW32_REG(ADDERSS) (*((volatile unsigned long *)(ADDERSS)))
#define stop_cpu          __breakpoint(0)

//����
#define vAssertCalled(char,int) printf("Error:%s,%d\r\n",char,int)
#define configASSERT(x) if((x)==0) vAssertCalled(__FILE__,__LINE__)
	
//---------------------------------------------------------------------//
typedef enum  
{
	Fail=0,
	Success,
}return_flag;

typedef enum  
{
	Handle_mode = 0x01,
	User_mode   = 0x03,
}ARM_mode;

typedef enum  
{
	TASK_BLOCK = 0x00,
	TASK_READY = 0x01,
}Task_state;

typedef enum  
{
	timer_list = 0,
	priority_list,
	softimer_list,
}List_type;

typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;
//---------------------------------------------------------------------//
struct Task_node
{
			 u16 *task_id_point;
			 char *task_name;
	volatile u8 task_state;
	volatile u8 task_pend_state;
			 u16 task_priority; 
	volatile u32 task_delay_ms;
			 unsigned int *task_stack;
			 u16 task_stack_size_words;
			 void *task_function;
	struct Task_node* timer_list_next;
    struct Task_node* priority_list_next;	
};
typedef struct Task_node Task_Unit;

typedef u16  Task_Handle;
extern Task_Unit Task_List[TASK_NUM];	
extern volatile u32 SysTick_count;
extern volatile u32 current_task_id;
extern Task_Handle idle_task;
extern Task_Unit *Timer_list_head;
extern Task_Unit *Priority_list_head;

//---------------------------------------------------------------------//
void RTOS_Init(void);
void RTOS_Delay(u32 delay_ms);
void Task_Creat_Init(void);
u8 Task_Create( 
				u16 *task_id,
				char *task_name,
				u8 task_state,
				u16 task_priority,
				u32 task_delay_ms,
				u16 task_stack_size_words,
				void *task_function	
			  );						
void taskENTER_CRITICAL(void);
void taskEXIT_CRITICAL(void);
void Release_Task(Task_Handle Task_x);
void Pend_Task(Task_Handle Task_x);		
void List_init(void);	
void List_insert(List_type type,Task_Unit *node);
void Launch_Task_Schedule(void);				
#endif 
