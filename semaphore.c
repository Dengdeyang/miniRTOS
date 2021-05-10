/********************************************************************************
  * @file    semaphore.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   消息队列与信号量
  *								1)消息队列采用malloc添加新节点，free释放旧结点
  *                             2)各个IPC具备临界段保护机制
  *								3)各个IPC具备自动识别当前的task和当前ARM的状态，可用于线程模式或中断服务函数中
  *                             4)互斥信号量具备优先级翻转问题解决方案
  *                             5)计数和互斥信号量释放函数会释放阻塞在当前信号量上的最高优先级任务，用于互斥资源保护
  *                             6)二值信号量释放函数会释放阻塞在当前信号量上的所有阻塞任务，用于同步
  ********************************************************************************/
#include "semaphore.h" 
#include "stm32f10x.h"
#include "soft_timer.h"
#include "stdio.h"
#include "stdlib.h"

//----------------------------------入队-------------------------------------//
/**
 * @brief  动态内存申请方式增加单项链表长度，实现入队操作
 * @param  
			queue_data：     指向一条队列的指针
			push_data：      需要压入该队列的数据
 * @retval 入队成功或失败标志                      
 */
static char push(Queue *queue_data,u32 push_data)
{
	char push_action_result;
	
	if(queue_data->queue_size==0)
	{
		queue_data->tail->data = push_data;
		queue_data->queue_size++;
		push_action_result = Success;
	}
	else
	{
		Queue_point *p = (Queue_point *)malloc(sizeof(Queue_point));
		if(NULL == p) 
		{
			printf("push fail!\r\n");
			push_action_result = Fail;
		}
		else 
		{
			queue_data->tail->next = p;
			queue_data->tail = p;
			queue_data->tail->data = push_data;
			queue_data->tail->next = NULL;
			queue_data->queue_size++;
			
			push_action_result = Success;
		}
	}
	return push_action_result;
}


//----------------------------------出队-------------------------------------//
/**
 * @brief  动态内存释放方式减小单项链表长度，实现出队操作
 * @param  
			queue_data：     指向一条队列的指针
			*pull_data：     队列弹出数据存放变量地址
 * @retval 出队成功或失败标志                      
 */
static char pull(Queue *queue_data,u32 *pull_data)
{
    Queue_point *p;
	char pull_action_result;
	
	if(!queue_data->queue_size) 
	{
		pull_action_result = Fail;
	}
	else if(queue_data->queue_size == 1) 
	{
		*pull_data = queue_data->head->data;
		queue_data->queue_size--;
		pull_action_result = Success;
	}
	else 
	{
		*pull_data = queue_data->head->data;
		p = queue_data->head;
		queue_data->head = queue_data->head->next;
		free(p);
		queue_data->queue_size--;
		pull_action_result = Success;
	}
	return pull_action_result;
}


//-------------------------消息队列创建与初始化-------------------------------------//
Queue_Handle Creat_queue(void)
{
	Queue *queue_x = (Queue *)malloc(sizeof(Queue));
	
	queue_x->head = (Queue_point *)malloc(sizeof(Queue_point));
	queue_x->tail = queue_x->head;
	queue_x->queue_size = 0;

	queue_x->head->data = 0;
    queue_x->head->next = NULL;

	if((queue_x==NULL)||(queue_x->head==NULL))   return NULL;
	else                                         return  queue_x;
}

//--------------------------------发送消息队列--------------------------------------------//
char QueueSend(Queue *queue_data,u32 push_data,u32 delay_ms)
{
	u16 i;
	char flag = Fail;
	Task_Unit *task_temp = &Task_List[idle_task];
	
	taskENTER_CRITICAL();
	flag = push(queue_data,push_data);
	
	if(flag != Success)
	{
		if((__get_CONTROL() & 0x0F) != Handle_mode)
		{
			if(delay_ms == 0)  flag = Fail;
			else
			{
					queue_data->task_block_list[current_task_id] = Semaphore_Block;
					taskEXIT_CRITICAL();
					RTOS_Delay(delay_ms);
					taskENTER_CRITICAL();
					queue_data->task_block_list[current_task_id] = Semaphore_Unblock;

					flag = push(queue_data,push_data);
			}
		}
		else flag = Fail;
	}
	
	if(flag == Success)
	{
		for(i=0;i<TASK_NUM;i++)
		{
			if((queue_data->task_block_list[i]==Semaphore_Block)\
				&&(*(Task_List[i].task_id_point) != timer_guard_task_id)\
			    &&(task_temp->task_priority < Task_List[i].task_priority))  
				{
					task_temp = &Task_List[i];
				}
		}
		if(task_temp != &Task_List[idle_task])
		{
			queue_data->task_block_list[*(task_temp->task_id_point)] = Semaphore_Unblock;
			task_temp->task_state = TASK_READY;
			task_temp->task_pend_state = TASK_READY;
			List_insert(priority_list,task_temp);
			Launch_Task_Schedule();
		}
	}
	
	taskEXIT_CRITICAL();
	return flag;
}

//--------------------------------接收消息队列--------------------------------------------//
char QueueReceive(Queue *queue_data,u32 *pull_data,u32 delay_ms)
{
	char flag = Fail;
	
	taskENTER_CRITICAL();
	
	if(queue_data->queue_size > 0)
	{
		pull(queue_data,pull_data);
		flag = Success;
	}
	else
	{
		if((__get_CONTROL() & 0x0F) != Handle_mode)
		{
			if(delay_ms == 0)  flag = Fail;
			else
			{
				queue_data->task_block_list[current_task_id] = Semaphore_Block;
				taskEXIT_CRITICAL();
				RTOS_Delay(delay_ms);
				taskENTER_CRITICAL();
				queue_data->task_block_list[current_task_id] = Semaphore_Unblock;
				
				if(queue_data->queue_size > 0) 
				{
					pull(queue_data,pull_data);
					flag = Success;
				}
				else  flag = Fail;
			}
		}
		else
		{
			flag = Fail;
		}
	}
	taskEXIT_CRITICAL();
	return flag;
}

//-------------------------二值+互斥+计数信号量Take函数-------------------------------------//
/**
 * @brief  信号量获取函数，可根据需要设置信号量的种类(二值、互斥、计数)，并设置阻塞延时
 * @param  
			type：       操作信号量的种类
            *Semaphore： 信号量地址
            delay_ms：   阻塞延时值
 * @retval 信号量获取成功或失败标志                      
 */
char Semaphore_Take(Semaphore_Type type,Semaphore_Handle *Semaphore,u32 delay_ms)
{
	char flag = Fail;
	
	taskENTER_CRITICAL();

	if(Semaphore->Semaphore)  
	{
		if(type == Count_Semaphore)  Semaphore->Semaphore -= 1;
		else 						 Semaphore->Semaphore = 0;
		flag = Success;
		if((__get_CONTROL() & 0x0F) != Handle_mode)//非中断模式才会更新Semaphore->task_id
		{
			Semaphore->task_id = current_task_id;
			Semaphore->task_priority = Task_List[current_task_id].task_priority;
		}
	}
	else                  
	{
		if((__get_CONTROL() & 0x0F) != Handle_mode)
		{
			if(delay_ms == 0)  flag = Fail;
			else
			{
				//互斥信号量的优先级翻转问题解决方案
				//优先级提升
				if((Semaphore->task_id != 0)\
					&&(Semaphore->task_priority != 0)\
					&&(Task_List[current_task_id].task_priority > Semaphore->task_priority)
				    &&(type == Mutex_Semaphore))  
					{
						Task_List[Semaphore->task_id].task_priority = Task_List[current_task_id].task_priority;
					}

				Semaphore->task_block_list[current_task_id] = Semaphore_Block;
				taskEXIT_CRITICAL();
				RTOS_Delay(delay_ms);
				taskENTER_CRITICAL();
				Semaphore->task_block_list[current_task_id] = Semaphore_Unblock;
				//优先级还原
				if((Semaphore->task_id != 0)\
					&&(Semaphore->task_priority != 0)\
					&&(Task_List[current_task_id].task_priority > Semaphore->task_priority)
				    &&(type == Mutex_Semaphore))				
				{
					Task_List[Semaphore->task_id].task_priority = Semaphore->task_priority;
				}
					
				if(Semaphore->Semaphore) 
				{
					if(type == Count_Semaphore)  Semaphore->Semaphore -= 1;
					else 						 Semaphore->Semaphore = 0;
					flag = Success;
					Semaphore->task_id = current_task_id;
					Semaphore->task_priority = Task_List[current_task_id].task_priority;
				}
				else  flag = Fail;
			}
		}
		else                  
		{
			flag = Fail;
		}
	}
	taskEXIT_CRITICAL();
	return flag;
}


//-------------------------二值+互斥+计数信号量Give函数-------------------------------------//
/**
 * @brief  信号量释放函数，无阻赛
 * @param  
			type：       操作信号量的种类
            *Semaphore： 信号量地址
 * @retval  NO                      
 */
void Semaphore_Give(Semaphore_Type type,Semaphore_Handle *Semaphore)
{
	u16 i;
	Task_Unit *task_temp = &Task_List[idle_task];
	taskENTER_CRITICAL();
	
	if(type == Count_Semaphore)  Semaphore->Semaphore += 1;
	else 						 Semaphore->Semaphore = 1;

	if(type ==Binary_Semaphore)
	{
		for(i=0;i<TASK_NUM;i++)
		{
			if((Semaphore->task_block_list[i]==Semaphore_Block)&&(*(Task_List[i].task_id_point) != timer_guard_task_id))  
			{
				Semaphore->task_block_list[i] = Semaphore_Unblock;
				Task_List[i].task_state = TASK_READY;
				Task_List[i].task_pend_state = TASK_READY;
				List_insert(priority_list,&Task_List[i]);
			}
		}
	}
	else
	{
		for(i=0;i<TASK_NUM;i++)
		{
			if((Semaphore->task_block_list[i]==Semaphore_Block)\
				&&(*(Task_List[i].task_id_point) != timer_guard_task_id)\
			    &&(task_temp->task_priority < Task_List[i].task_priority))  
				{
					task_temp = &Task_List[i];
				}
		}
		if(task_temp != &Task_List[idle_task])
		{
			Semaphore->task_block_list[*(task_temp->task_id_point)] = Semaphore_Unblock;
			task_temp->task_state = TASK_READY;
			task_temp->task_pend_state = TASK_READY;
			List_insert(priority_list,task_temp);
		}
	}				
	Launch_Task_Schedule();
	taskEXIT_CRITICAL();
}

/*********************************************END OF FILE**********************/
