#ifndef __SOFT_TIMER_H
#define	__SOFT_TIMER_H

#include "kernel.h"
 
//-------------软件定时器结构体---------------------//
struct Softimer_node
{
	         u16  *softimer_id;
	volatile u8   timer_state;
	volatile u8   timer_runflag;
	volatile u32  timer_count;
	volatile u32  timer_reload;
	u16           timer_priority;
	volatile u8   timer_mode;
    void (*callback_function)(void);
	struct Softimer_node *softimer_next;
};
typedef struct Softimer_node Softimer_Unit;

//------------软件定时器开启或关闭枚举值------------//
typedef enum  
{
	stop_timer = 0,
	run_timer,
}Timer_Switch;

//------------软件定时器单次运行或重复运行----------//
typedef enum  
{
	once_mode = 0,
	repeat_mode,
}Timer_mode;

typedef u16  Soft_Timer_Handle;
extern Softimer_Unit Softimer_List[Softimer_NUM];
extern Task_Handle timer_guard_task_id;
extern Soft_Timer_Handle idle_softimer;
extern Softimer_Unit *Softimer_list_head;

void soft_timer_guard_task(void);
void Soft_Timer_Creat(Soft_Timer_Handle *timer_id,Timer_Switch timer_state,Timer_mode mode,u16 priority,u32 timer_reload_value,void (*timer_function)(void));
void Start_Soft_Timer(Soft_Timer_Handle timer_id);
void Stop_Soft_Timer(Soft_Timer_Handle timer_id);
void Set_Soft_Timer(Soft_Timer_Handle timer_id,u32 tick_count);	
void Softimer_List_init(void);
#endif 
