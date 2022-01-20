/********************************************************************************
  * @file    device.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   device驱动抽象层,由以下函数组成:
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
  * @atteration  使用方法：
					在对应的外设驱动层中定义设备的以下函数，并初始化设备结构体成员，init，open，close，read，write，control
                    调用device_register将设备插入到设备链表。
					调用device_find通过设备名称字符串，在设备链表中查找设备结构体指针。
					调用device_init，device_open，device_read，device_write，device_control，device_close完成设备的操作。
					调用device_set_rx_indicate，device_set_tx_complete注册设备读或写完成回调函数
					可在其回调函数中发送信号量或通知，告知数据处理任务进行数据的处理。
  ********************************************************************************/
#include "device.h"
#include "mini_libc.h"
#include "miniRTOSconfig.h"

#if miniRTOS_Device

Device_Handle Device_list_head=NULL;

//----------------------------------------------------------------------//
Device_Handle device_find(char *name)
{
	Device_Handle temp=NULL,result=NULL;
	configASSERT(name != NULL);
	
	for(temp=Device_list_head;temp != NULL;temp=temp->next)
	{
		if(0 == mini_strcmp(temp->name,name))
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
		if(0 == mini_strcmp(Device_list_head->name,device->name))
		{
			Device_list_head = Device_list_head->next;
			flag = Success;
		}
		else
		{
			for(temp=Device_list_head;temp->next != NULL;temp=temp->next)
			{
				if(0 == mini_strcmp(temp->next->name,device->name))
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
return_flag device_open(Device_Handle device,u32 mode)
{
	return_flag flag = Fail;
	configASSERT(device != NULL);
	if(device->open_flag == close_state)
	{
		if(device->init_flag == No_init) 
		{
		    device_init(device);
		}
		if(device->open != NULL) (*device->open)(mode);
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
	
	mini_printf("Device_list_head name =%s,init_flag=%d,open_flag=%d\r\n",Device_list_head->name,Device_list_head->init_flag,Device_list_head->open_flag);
	for(temp = Device_list_head; temp != NULL ;temp = temp->next)
	{
		mini_printf("name =%s,init_flag=%d,open_flag=%d\r\n",temp->name,temp->init_flag,temp->open_flag);
	}
	mini_printf("\r\n");
}
#endif
/*********************************************END OF FILE**********************/
