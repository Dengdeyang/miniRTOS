/********************************************************************************
  * @file    semaphore.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   ��Ϣ�������ź���
  *								1)��Ϣ���в���mini_malloc����½ڵ㣬mini_free�ͷžɽ��
  *                             2)����IPC�߱��ٽ�α�������
  *								3)����IPC�߱��Զ�ʶ��ǰ��task�͵�ǰARM��״̬���������߳�ģʽ���жϷ�������
  *                             4)�����ź����߱����ȼ���ת����������
  *                             5)�����ͻ����ź����ͷź������ͷ������ڵ�ǰ�ź����ϵ�������ȼ��������ڻ�����Դ����
  *                             6)��ֵ�ź����ͷź������ͷ������ڵ�ǰ�ź����ϵ�����������������ͬ��
  ********************************************************************************/
#include "semaphore.h" 
#include "stm32f10x.h"
#include "soft_timer.h"
#include "stdlib.h"
#include "mini_libc.h"
#include "heap.h"
//----------------------------------���-------------------------------------//
/**
 * @brief  ��̬�ڴ����뷽ʽ���ӵ��������ȣ�ʵ����Ӳ���
 * @param  
			queue_data��     ָ��һ�����е�ָ��
			push_data��      ��Ҫѹ��ö��е�����
			byteNum����Ҫ��ӵ�������ռ�ֽ���
 * @retval ��ӳɹ���ʧ�ܱ�־                      
 */
static char Queue_message_push(Queue_message *queue_data,void *push_data,unsigned int byteNum)
{
    unsigned char *temp = (unsigned char *)push_data;
	unsigned char *data_temp = (unsigned char *)mini_malloc(byteNum);
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

//----------------------------------����-------------------------------------//
/**
 * @brief  ��̬�ڴ��ͷŷ�ʽ��С���������ȣ�ʵ�ֳ��Ӳ���
 * @param  
			queue_data��     ָ��һ�����е�ָ��
			*pull_data��     ���е������ݴ�ű�����ַ
			byteNum����Ҫ��ӵ�������ռ�ֽ���
 * @retval ���ӳɹ���ʧ�ܱ�־                      
 */
static char Queue_message_pull(Queue_message *queue_data,void *pull_data,unsigned int byteNum)
{
	unsigned char *data_temp;
	Queue_point *queue_temp = queue_data->head;
	unsigned char *temp = (unsigned char *)pull_data;
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


//-------------------------��Ϣ���д������ʼ��-------------------------------------//
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

//--------------------------------������Ϣ����--------------------------------------------//
char QueueSend(Queue_message *queue,void *message,unsigned int byteNum,u32 delay_ms,MCU_mode mode)
{
	u16 i;
	char flag = Fail;
	Task_Unit *task_temp = &Task_List[idle_task];
	
	Enter_Critical();
	flag = Queue_message_push(queue,message,byteNum);
	
	if(flag != Success)
	{
		if(mode != Handle_mode)
		{
			if(delay_ms == 0)  flag = Fail;
			else
			{
					queue->task_block_list[current_task_id] = Semaphore_Block;
					Exit_Critical();
					RTOS_Delay(delay_ms);
					Enter_Critical();
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
	
	Exit_Critical();
	return flag;
}

//--------------------------------������Ϣ����--------------------------------------------//
char QueueReceive(Queue_message *queue,void *message,unsigned int byteNum,u32 delay_ms,MCU_mode mode)
{
	char flag = Fail;
	
	Enter_Critical();
	flag = Queue_message_pull(queue,message,byteNum);

	if(flag != Success)
	{
		if(mode != Handle_mode)
		{
			if(delay_ms == 0)  flag = Fail;
			else
			{
				queue->task_block_list[current_task_id] = Semaphore_Block;
				Exit_Critical();
				RTOS_Delay(delay_ms);
				Enter_Critical();
				queue->task_block_list[current_task_id] = Semaphore_Unblock;
				flag = Queue_message_pull(queue,message,byteNum);
			}
		}
		else
		{
			flag = Fail;
		}
	}
	Exit_Critical();
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
char Semaphore_Take(Semaphore_Type type,Semaphore_Handle *Semaphore,u32 delay_ms,MCU_mode mode)
{
	char flag = Fail;
	
	Enter_Critical();

	if(Semaphore->Semaphore)  
	{
		if(type == Count_Semaphore)  Semaphore->Semaphore -= 1;
		else 						 Semaphore->Semaphore = 0;
		flag = Success;
		if(mode != Handle_mode)//���ж�ģʽ�Ż����Semaphore->task_id
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
				Exit_Critical();
				RTOS_Delay(delay_ms);
				Enter_Critical();
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
	Exit_Critical();
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
	Enter_Critical();
	
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
	Exit_Critical();
}

/*********************************************END OF FILE**********************/
