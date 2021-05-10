/********************************************************************************
  * @file    semaphore.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   ��Ϣ�������ź���
  *								1)��Ϣ���в���malloc����½ڵ㣬free�ͷžɽ��
  *                             2)����IPC�߱��ٽ�α�������
  *								3)����IPC�߱��Զ�ʶ��ǰ��task�͵�ǰARM��״̬���������߳�ģʽ���жϷ�������
  *                             4)�����ź����߱����ȼ���ת����������
  *                             5)�����ͻ����ź����ͷź������ͷ������ڵ�ǰ�ź����ϵ�������ȼ��������ڻ�����Դ����
  *                             6)��ֵ�ź����ͷź������ͷ������ڵ�ǰ�ź����ϵ�����������������ͬ��
  ********************************************************************************/
#include "semaphore.h" 
#include "stm32f10x.h"
#include "soft_timer.h"
#include "stdio.h"
#include "stdlib.h"

//----------------------------------���-------------------------------------//
/**
 * @brief  ��̬�ڴ����뷽ʽ���ӵ��������ȣ�ʵ����Ӳ���
 * @param  
			queue_data��     ָ��һ�����е�ָ��
			push_data��      ��Ҫѹ��ö��е�����
 * @retval ��ӳɹ���ʧ�ܱ�־                      
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


//----------------------------------����-------------------------------------//
/**
 * @brief  ��̬�ڴ��ͷŷ�ʽ��С���������ȣ�ʵ�ֳ��Ӳ���
 * @param  
			queue_data��     ָ��һ�����е�ָ��
			*pull_data��     ���е������ݴ�ű�����ַ
 * @retval ���ӳɹ���ʧ�ܱ�־                      
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


//-------------------------��Ϣ���д������ʼ��-------------------------------------//
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

//--------------------------------������Ϣ����--------------------------------------------//
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

//--------------------------------������Ϣ����--------------------------------------------//
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

//-------------------------��ֵ+����+�����ź���Take����-------------------------------------//
/**
 * @brief  �ź�����ȡ�������ɸ�����Ҫ�����ź���������(��ֵ�����⡢����)��������������ʱ
 * @param  
			type��       �����ź���������
            *Semaphore�� �ź�����ַ
            delay_ms��   ������ʱֵ
 * @retval �ź�����ȡ�ɹ���ʧ�ܱ�־                      
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
		if((__get_CONTROL() & 0x0F) != Handle_mode)//���ж�ģʽ�Ż����Semaphore->task_id
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
				//�����ź��������ȼ���ת����������
				//���ȼ�����
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
				//���ȼ���ԭ
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


//-------------------------��ֵ+����+�����ź���Give����-------------------------------------//
/**
 * @brief  �ź����ͷź�����������
 * @param  
			type��       �����ź���������
            *Semaphore�� �ź�����ַ
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
