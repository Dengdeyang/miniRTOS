/********************************************************************************
  * @file    device.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   device���������,�����º������:
						 device_register
						 device_unregister
						 device_find
						 device_init
						 device_open
						 device_close
						 device_read
						 device_write
						 device_control
						 device_set_rx_indicate
						 device_set_tx_complete
  * @atteration  ʹ�÷�����
					�ڶ�Ӧ�������������ж����豸�����º���������ʼ���豸�ṹ���Ա��init��open��close��read��write��control
                    ����device_register���豸���뵽�豸������
					����device_findͨ���豸�����ַ��������豸�����в����豸�ṹ��ָ�롣
					����device_init��device_open��device_read��device_write��device_control��device_close����豸�Ĳ�����
					����device_set_rx_indicate��device_set_tx_completeע���豸����д��ɻص�����
					������ص������з����ź�����֪ͨ����֪���ݴ�������������ݵĴ�����
  ********************************************************************************/
#include "device.h"
#include "string.h"

Device_Handle Device_list_head=NULL;

//----------------------------------------------------------------------//
Device_Handle device_find(char *name)
{
	Device_Handle temp=NULL,result=NULL;
	configASSERT(name != NULL);
	
	for(temp=Device_list_head;temp != NULL;temp=temp->next)
	{
		if(0 == strcmp(temp->name,name))
		{
			result = temp;
			break;
		}
	}
	return result;
}

//----------------------------------------------------------------------//
return_flag device_register(Device_Handle device)
{
	return_flag flag = Fail;
	Device_Handle temp=NULL;
	configASSERT(device != NULL);
	
	if(NULL == device_find(device->name)) 
	{
		if(Device_list_head == NULL)
		{
			Device_list_head = device;
			device->next = NULL;
		}
		else
		{
			for(temp=Device_list_head;temp->next != NULL;temp=temp->next)
			{};
			temp->next = device;
			device->next = NULL;
		}
		device->open_flag = close_state;
		device->init_flag = No_init;
		flag = Success;
	}
    return flag;
}

//----------------------------------------------------------------------//
return_flag device_unregister(Device_Handle device)
{
	return_flag flag = Fail;
	Device_Handle temp=NULL;
	configASSERT(device != NULL);
	
	if(Device_list_head != NULL)
	{
		if(0 == strcmp(Device_list_head->name,device->name))
		{
			Device_list_head = Device_list_head->next;
			flag = Success;
		}
		else
		{
			for(temp=Device_list_head;temp->next != NULL;temp=temp->next)
			{
				if(0 == strcmp(temp->next->name,device->name))
				{
					temp->next = temp->next->next;
					flag = Success;
					break;
				}
			}
		}
	}
	return flag;
}

//----------------------------------------------------------------------//
return_flag device_init(Device_Handle device)
{
	return_flag flag = Fail;
	configASSERT(device != NULL);
	if((device->init_flag == No_init)&&(device->init != NULL)) 
	{
		(*device->init)();
		device->init_flag = Is_init;
		flag = Success;
	}
	return flag;
}

//----------------------------------------------------------------------//
return_flag device_open(Device_Handle device)
{
	return_flag flag = Fail;
	configASSERT(device != NULL);
	if(device->open_flag == close_state)
	{
		if(device->init_flag == No_init) 
		{
		    device_init(device);
		}
		if(device->open != NULL) (*device->open)();
		device->open_flag = open_state;
		flag = Success;
	}
	return flag;
}
 
//----------------------------------------------------------------------//
return_flag device_close(Device_Handle device)
{
	return_flag flag = Fail;
	configASSERT(device != NULL);
	if((device->open_flag == open_state)&&(device->close != NULL))
	{
		(*device->close)();
		device->open_flag = close_state;
		flag = Success;
	}
	return flag;
}

//----------------------------------------------------------------------//
u32 device_read(Device_Handle device,u32 pos,void *buffer,u32 size)
{
	u32 temp=0;
	configASSERT((device != NULL)&&(buffer != NULL));
    if((device->open_flag == open_state)&&(device->read != NULL))
	{
		temp = device->read(pos,buffer,size);
	}
	return temp;	 
}

//----------------------------------------------------------------------//
u32 device_write(Device_Handle device,u32 pos,const void *buffer,u32 size)
{
	u32 temp=0;
	configASSERT((device != NULL)&&(buffer != NULL));
    if((device->open_flag == open_state)&&(device->write != NULL))
	{
		temp = device->write(pos,buffer,size);
	}
	return temp;
}

//----------------------------------------------------------------------//
return_flag device_control(Device_Handle device,u32 cmd, void *arg)
{
	return_flag flag = Fail;
	configASSERT(device != NULL);
	if((device->open_flag == open_state)&&(device->control != NULL))
	{
		device->control(cmd,arg);
		flag = Success;
	}
	return flag;
}

//----------------------------------------------------------------------//
void device_set_rx_indicate(Device_Handle device,void(*rx_ind)(void *buffer,u32 size))
{
	configASSERT((device != NULL)&&(rx_ind != NULL));
	device->rx_complete_callback = rx_ind;
}

//----------------------------------------------------------------------//
void device_set_tx_complete(Device_Handle device,void(*tx_done)(void *buffer,u32 size))
{
	configASSERT((device != NULL)&&(tx_done != NULL));
	device->rx_complete_callback = tx_done;
}

//----------------------------------------------------------------------//
void printf_device_List(void)
{
	Device_Handle temp=NULL;
	
	printf("Device_list_head name =%s,init_flag=%d,open_flag=%d\r\n",Device_list_head->name,Device_list_head->init_flag,Device_list_head->open_flag);
	for(temp = Device_list_head; temp != NULL ;temp = temp->next)
	{
		printf("name =%s,init_flag=%d,open_flag=%d\r\n",temp->name,temp->init_flag,temp->open_flag);
	}
	printf("\r\n");
}
/*********************************************END OF FILE**********************/