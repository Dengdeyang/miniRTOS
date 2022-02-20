/********************************************************************************
  * @file    semaphore.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   消息队列与信号量
  *								1)消息队列采用mini_malloc添加新节点，mini_free释放旧结点
  *                             2)各个IPC具备临界段保护机制
  *								3)各个IPC具备自动识别当前的task和当前ARM的状态，可用于线程模式或中断服务函数中
  *                             4)互斥信号量具备优先级翻转问题解决方案
  *                             5)计数和互斥信号量释放函数会释放阻塞在当前信号量上的最高优先级任务，用于互斥资源保护
  *                             6)二值信号量释放函数会释放阻塞在当前信号量上的所有阻塞任务，用于同步
  ********************************************************************************/
#include "semaphore.h" 
#include "soft_timer.h"
#include "stdlib.h"
#include "mini_libc.h"
#include "heap.h"
#include "miniRTOSconfig.h"
#include "miniRTOSport.h"
#include "list.h" 

#if miniRTOS_Semaphore
//----------------------------------入队-------------------------------------//
/**
 * @brief  动态内存申请方式增加单项链表长度，实现入队操作
 * @param  
			queue_data：     指向一条队列的指针
			push_data：      需要压入该队列的数据
			byteNum：需要入队的数据所占字节数
 * @retval 入队成功或失败标志                      
 */
static char Queue_message_push(Queue_message *queue_data,void *push_data,uint32 byteNum)
{
    uint8 *temp = (uint8 *)push_data;
	uint8 *data_temp = (uint8 *)mini_malloc(byteNum);
	Queue_point *queue_temp = (Queue_point *)mini_malloc(sizeof(Queue_point));

	if((temp == NULL)||(queue_temp == NULL)||(data_temp == NULL)) return Fail;
	else
	{
		if((queue_data->tail == NULL)&&(queue_data->head == NULL))  queue_data->head = queue_temp;
		else	queue_data->tail->next = queue_temp;

		queue_data->tail = queue_temp;
		queue_temp->next = NULL;
		queue_temp->data = data_temp;
		while(byteNum--)
		{
			*data_temp++ = *temp++; 
		}
		return Success;
	}
}

//----------------------------------出队-------------------------------------//
/**
 * @brief  动态内存释放方式减小单项链表长度，实现出队操作
 * @param  
			queue_data：     指向一条队列的指针
			*pull_data：     队列弹出数据存放变量地址
			byteNum：需要入队的数据所占字节数
 * @retval 出队成功或失败标志                      
 */
static char Queue_message_pull(Queue_message *queue_data,void *pull_data,uint32 byteNum)
{
	uint8 *data_temp;
	Queue_point *queue_temp = queue_data->head;
	uint8 *temp = (uint8 *)pull_data;
	if((queue_data->tail == NULL)&&(queue_data->head == NULL)) 	return Fail;
	else
	{
		data_temp = queue_data->head->data;
		while(byteNum--)
		{
			*temp++ = *data_temp++;
		}
		queue_data->head = queue_data->head->next;
		mini_free(queue_temp->data);
		mini_free(queue_temp);
		if(queue_data->head == NULL)  queue_data->tail = NULL;
		return Success;
	}
}


//-------------------------消息队列创建与初始化-------------------------------------//
Queue_Handle Creat_queue(void)
{
	Queue_message *queue_x = (Queue_message *)mini_malloc(sizeof(Queue_message));
	
	if(queue_x != NULL)
	{	
		queue_x->head = NULL;
		queue_x->tail = NULL;
		mini_memset(queue_x->task_block_list,0,TASK_NUM);
	}
	return  queue_x;
}

//--------------------------------发送消息队列--------------------------------------------//
char QueueSend(Queue_message *queue,void *message,uint32 byteNum,uint32 delay_ms,MCU_mode mode)
{
	uint16 i;
	char flag = Fail;
	Task_Unit *task_temp = &Task_List[idle_task];
	
	uint32 irq_status;
	irq_status = Enter_Critical();
	flag = Queue_message_push(queue,message,byteNum);
	
	if(flag != Success)
	{
		if(mode != Handle_mode)
		{
			if(delay_ms == 0)  flag = Fail;
			else
			{
					queue->task_block_list[current_task_id] = Semaphore_Block;
					Exit_Critical(irq_status);
					RTOS_Delay(delay_ms);
					irq_status = Enter_Critical();
					queue->task_block_list[current_task_id] = Semaphore_Unblock;

					flag = Queue_message_push(queue,message,byteNum);
			}
		}
		else flag = Fail;
	}
	
	if(flag == Success)
	{
		for(i=0;i<TASK_NUM;i++)
		{
			if((queue->task_block_list[i]==Semaphore_Block)\
				&&(*(Task_List[i].task_id_point) != timer_guard_task_id)\
			    &&(task_temp->task_priority < Task_List[i].task_priority))  
				{
					task_temp = &Task_List[i];
				}
		}
		if(task_temp != &Task_List[idle_task])
		{
			queue->task_block_list[*(task_temp->task_id_point)] = Semaphore_Unblock;
			task_temp->task_state = TASK_READY;
			task_temp->task_pend_state = TASK_READY;
			List_insert(priority_list,task_temp,order_insert);
			Launch_Task_Schedule();
		}
	}
	
	Exit_Critical(irq_status);
	return flag;
}

//--------------------------------接收消息队列--------------------------------------------//
char QueueReceive(Queue_message *queue,void *message,uint32 byteNum,uint32 delay_ms,MCU_mode mode)
{
	char flag = Fail;
	
	uint32 irq_status;
	irq_status = Enter_Critical();
	flag = Queue_message_pull(queue,message,byteNum);

	if(flag != Success)
	{
		if(mode != Handle_mode)
		{
			if(delay_ms == 0)  flag = Fail;
			else
			{
				queue->task_block_list[current_task_id] = Semaphore_Block;
				Exit_Critical(irq_status);
				RTOS_Delay(delay_ms);
				irq_status = Enter_Critical();
				queue->task_block_list[current_task_id] = Semaphore_Unblock;
				flag = Queue_message_pull(queue,message,byteNum);
			}
		}
		else
		{
			flag = Fail;
		}
	}
	Exit_Critical(irq_status);
	return flag;
}

//-------------------------二值+互斥+计数信号量初始化函数-------------------------------------//
/**
 * @brief  初始化信号量(二值、互斥、计数)
 * @param  
			type：       操作信号量的种类
            *Semaphore： 信号量地址
 * @retval NO                   
 */
void Semaphore_Creat(Semaphore_Type type,Semaphore_Handle *Semaphore,uint32 init_value)
{
	if(type == Count_Semaphore)
	{
		Semaphore->Semaphore = init_value;
	}
	else
	{
		if(init_value != 0)  Semaphore->Semaphore = 1;
		else                 Semaphore->Semaphore = 0;
	}
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
char Semaphore_Take(Semaphore_Type type,Semaphore_Handle *Semaphore,uint32 delay_ms,MCU_mode mode)
{
	char flag = Fail;
	
	uint32 irq_status;
	irq_status = Enter_Critical();

	if(Semaphore->Semaphore)  
	{
		if(type == Count_Semaphore)  Semaphore->Semaphore -= 1;
		else 						 Semaphore->Semaphore = 0;
		flag = Success;
		if(mode != Handle_mode)//非中断模式才会更新Semaphore->task_id
		{
			Semaphore->task_id = current_task_id;
			Semaphore->task_priority = Task_List[current_task_id].task_priority;
		}
	}
	else                  
	{
		if(mode != Handle_mode)
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
				Exit_Critical(irq_status);
				RTOS_Delay(delay_ms);
				irq_status = Enter_Critical();
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
	Exit_Critical(irq_status);
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
	uint16 i;
	Task_Unit *task_temp = &Task_List[idle_task];
	uint32 irq_status;
	irq_status = Enter_Critical();
	
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
				List_insert(priority_list,&Task_List[i],order_insert);
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
			List_insert(priority_list,task_temp,order_insert);
		}
	}				
	Launch_Task_Schedule();
	Exit_Critical(irq_status);
}
#endif
/*********************************************END OF FILE**********************/
