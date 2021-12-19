#include "led.h"   
#include "device.h"

Device_Unit LED_Device;

	
 /**
  * @brief  初始化控制LED的IO
  * @param  无
  * @retval 无
  */
void LED_GPIO_Config(void)
{		
		
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB, ENABLE); 
								   
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_Init(GPIOB, &GPIO_InitStructure);													   
		
	GPIO_SetBits(GPIOB, GPIO_Pin_10);
	GPIO_SetBits(GPIOB, GPIO_Pin_11);
	GPIO_SetBits(GPIOB, GPIO_Pin_13);
	GPIO_SetBits(GPIOB, GPIO_Pin_14);
	GPIO_SetBits(GPIOB, GPIO_Pin_15);
}

void Delay_ms(unsigned int ms)
{
	unsigned int i,j;
	for(i=0;i<ms;i++)
	for(j=0;j<10000;j++);
}

//---------------------------------------------------------------------//
void LED_Init(void)
{		
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB, ENABLE); 
								   
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_Init(GPIOB, &GPIO_InitStructure);													   
	GPIO_SetBits(GPIOB, GPIO_Pin_12);
}

//---------------------------------------------------------------------//
u32 LED_Read(u32 pos,void *buffer,u32 size)
{		
	*((u8 *)buffer) = GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12);
	return size;
}

//---------------------------------------------------------------------//
u32 LED_Write(u32 pos,const void *buffer,u32 size)
{		
	GPIO_WriteBit(GPIOB,GPIO_Pin_12,(BitAction)(*((u8 *)buffer)));
	return size;
}

//---------------------------------------------------------------------//
void LED_Control(u32 cmd, void *arg)
{	
	GPIO_InitTypeDef GPIO_InitStructure;
								   										   
	switch((LED_CMD)cmd)
	{
		case LED_output: GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;break;
		
		case LED_intput: GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;break;
		
		default:         GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;break;
	}
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_Init(GPIOB, &GPIO_InitStructure);	
}


//---------------------------------------------------------------------//

void LED_Device_Register(void)
{
	LED_Device.name    = "LED";
	LED_Device.init    = LED_Init;
	LED_Device.open    = NULL;
	LED_Device.close   = NULL;
	LED_Device.read    = LED_Read;
	LED_Device.write   = LED_Write;
	LED_Device.control = LED_Control;
	device_register(&LED_Device);
}
/*********************************************END OF FILE**********************/
