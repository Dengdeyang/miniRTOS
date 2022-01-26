/********************************************************************************
  * @file    list.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   任务优先级链表初始化，插入节点，移除节点，打印链表函数
  * @atteration   
  ********************************************************************************/ 
#include "list.h" 
#include "mini_libc.h"
 
//-------------------------任务优先级链表和timer链表打印函数--------------------------------//
void printf_List(List_type type)
{
	Task_Unit *temp;

	switch(type)
	{
		case timer_list:
		{
			mini_printf("time_head name =%s,delay=%d\r\n",Task_List[*(Timer_list_head->task_id_point)].task_name,Timer_list_head->task_delay_ms);
			for(temp = Timer_list_head; temp != NULL ;temp = temp->timer_list_next)
			{
				mini_printf("id = %s,time = %d\r\n",temp->task_name,temp->task_delay_ms);
			}
		};break;
		
		case priority_list:
		{
			mini_printf("Priority_list_head name =%s,Priority=%d\r\n",Priority_list_head->task_name,Priority_list_head->task_priority);
			for(temp = Priority_list_head; temp != NULL ;temp = temp->priority_list_next)
			{
				mini_printf("name =%s,Priority=%d,delay=%d,tick=%d,\r\n",temp->task_name,temp->task_priority,temp->task_delay_ms,temp->task_tick_count);
			}
		};break;
		
		default:break;
	}
	mini_printf("\r\n");
}

//---------------------------任务优先级链表和timer链表移除节点函数-------------------------------//
void List_remove_node(List_type type,Task_Unit *node)
{
	Task_Unit *temp;
	
	switch(type)
	{
		case timer_list:
		{
			if(node->task_id_point == Timer_list_head->task_id_point) 
			{
				Timer_list_head = Timer_list_head->timer_list_next;
			}
			else
			{
				for(temp = Timer_list_head; temp->timer_list_next != NULL ;temp = temp->timer_list_next)
				{
					if(temp->timer_list_next->task_id_point == node->task_id_point)  
					{
						temp->timer_list_next = temp->timer_list_next->timer_list_next;
						break;
					}
				}
			}
		};break;
		
		case priority_list:
		{
			if(node->task_id_point == Priority_list_head->task_id_point) 
			{
				Priority_list_head = Priority_list_head->priority_list_next;
			}
			else
			{
				for(temp = Priority_list_head; temp->priority_list_next != NULL ;temp = temp->priority_list_next)
				{
					if(temp->priority_list_next->task_id_point == node->task_id_point)  
					{
						temp->priority_list_next = temp->priority_list_next->priority_list_next;
						break;
					}
				}
			}
		};break;
		
	    default:break;
	}
}

//------------------------任务优先级链表和timer链表插入节点函数-------------------------//
void List_insert(List_type type,Task_Unit *node,List_insert_type insert_type)
{
	Task_Unit *temp;
	
	List_remove_node(type,node);
	switch(type)
	{
		case timer_list:
		{
			if(node->task_delay_ms < Timer_list_head->task_delay_ms)  //首节点插入 
			{
				node->timer_list_next =  Timer_list_head;
				Timer_list_head = node;
			}
			else
			{
				for(temp = Timer_list_head; temp->timer_list_next != NULL ;temp = temp->timer_list_next)
				{
					if((node->task_delay_ms>=temp->task_delay_ms)&&(node->task_delay_ms<=temp->timer_list_next->task_delay_ms)) 
					{
						node->timer_list_next = temp->timer_list_next;
						temp->timer_list_next = node;
						break;
					}
				}
				if(temp->timer_list_next == NULL) //尾结点插入 
				{
					temp->timer_list_next = node;
					node->timer_list_next = NULL;
				}
			}
		};break;
		
		case priority_list:
		{
			if((node->task_state != TASK_BLOCK)&&(node->task_pend_state != TASK_BLOCK)) 
			{
				if(node->task_priority > Priority_list_head->task_priority)  //首节点插入 
				{
					node->priority_list_next =  Priority_list_head;
					Priority_list_head = node;
				}
				else
				{

					for(temp = Priority_list_head; temp->priority_list_next != NULL ;temp = temp->priority_list_next)
					{
						if(insert_type == reverse_insert)//同优先级逆序插入
						{
							if((node->task_priority <= temp->task_priority)&&(node->task_priority > temp->priority_list_next->task_priority)) 
							{
								node->priority_list_next = temp->priority_list_next;
								temp->priority_list_next = node;
								break;
							}
						}
						else//同优先级顺序插入
						{
							if((node->task_priority <= temp->task_priority)&&(node->task_priority >= temp->priority_list_next->task_priority)) 
							{
								node->priority_list_next = temp->priority_list_next;
								temp->priority_list_next = node;
								break;
							}
						}
					}
					if(temp->priority_list_next == NULL) //尾结点插入 
					{
						temp->priority_list_next = node;
						node->priority_list_next = NULL;
					}
				}
			}
		};break;
		default:break;
	}
}

//---------------------------任务优先级链表和timer链表初始化函数------------------------------------//
void Task_list_init(void)
{
	uint16 i;	
	
	Timer_list_head = &Task_List[0];
	Priority_list_head= &Task_List[0];

	for(i=0;i<TASK_NUM-1;i++)
	{
		Task_List[i].timer_list_next = &Task_List[i+1];
		Task_List[i].priority_list_next = &Task_List[i+1];
	}
	Task_List[i].timer_list_next = NULL;
    Task_List[i].priority_list_next = NULL;
	
	for(i=0;i<TASK_NUM;i++)
	{
        List_insert(priority_list,&Task_List[i],order_insert);
		List_insert(timer_list,&Task_List[i],order_insert);
	}
}

//----------------------------------软件定时器链表打印函数----------------------------------//
void printf_Softimer_List(void)
{
	Softimer_Unit *temp;

	mini_printf("Softimer_list_head id =%d,timer_count=%d\r\n",*(Softimer_list_head->softimer_id),Softimer_list_head->timer_count);
	for(temp = Softimer_list_head; temp != NULL ;temp = temp->softimer_next)
	{
		mini_printf("id =%d,count=%d,\r\n",*(temp->softimer_id),temp->timer_count);
	}
	mini_printf("\r\n");
}

//-------------------------------软件定时器列表打印函数------------------------------------//
void printf_Softimer_array(void)
{
    uint16 i;
	
	for(i=0;i<Softimer_NUM;i++)
	{
		mini_printf("array id =%d,priority=%d,\r\n",*Softimer_List[i].softimer_id,Softimer_List[i].timer_priority);
	}
}

//-----------------------------软件定时器链表移除节点函数-----------------------------------//
void Softimer_list_remove_node(Softimer_Unit *node)
{
	Softimer_Unit *temp;
	
	if(node->softimer_id == Softimer_list_head->softimer_id) 
	{
		Softimer_list_head = Softimer_list_head->softimer_next;
	}
	else
	{
		for(temp = Softimer_list_head; temp->softimer_next != NULL ;temp = temp->softimer_next)
		{
			if(temp->softimer_next->softimer_id == node->softimer_id)  
			{
				temp->softimer_next = temp->softimer_next->softimer_next;
				break;
			}
		}
	}
}

//---------------------------软件定时器插入节点函数-----------------------------------------//
void Softimer_list_insert(Softimer_Unit *node)
{
	Softimer_Unit *temp;
	
	Softimer_list_remove_node(node);

	if(node->timer_count < Softimer_list_head->timer_count)  //首节点插入 
	{
		node->softimer_next =  Softimer_list_head;
		Softimer_list_head = node;
	}
	else
	{
		for(temp = Softimer_list_head; temp->softimer_next != NULL ;temp = temp->softimer_next)
		{
			if((node->timer_count >= temp->timer_count)&&(node->timer_count <= temp->softimer_next->timer_count)) 
			{
				node->softimer_next = temp->softimer_next;
				temp->softimer_next = node;
				break;
			}
		}
		if(temp->softimer_next == NULL) //尾结点插入 
		{
			temp->softimer_next = node;
			node->softimer_next = NULL;
		}
	}
}

//----------------------------软件定时器链表初始化函数---------------------------------------//
void Softimer_List_init(void)
{
	uint16 i,j;	
	Softimer_Unit Softimer_temp;
	for(i=0;i<Softimer_NUM;i++)
	{
		for(j=i+1;j<Softimer_NUM;j++)
		{
			if(Softimer_List[i].timer_priority < Softimer_List[j].timer_priority) 
			{
				Softimer_temp = Softimer_List[j];
				Softimer_List[j] = Softimer_List[i];
				Softimer_List[i] = Softimer_temp;
			}
		}
	}

	for(i=0;i<Softimer_NUM;i++)
	{
		*(Softimer_List[i].softimer_id) = i;
	}

	Softimer_list_head = &Softimer_List[0];
	for(i=0;i<Softimer_NUM-1;i++)
	{
		Softimer_List[i].softimer_next = &Softimer_List[i+1];
	}
	Softimer_List[i].softimer_next = NULL;

	for(i=0;i<Softimer_NUM;i++)
	{
		Softimer_list_insert(&Softimer_List[i]);
	}
}
/*********************************************END OF FILE**********************/
