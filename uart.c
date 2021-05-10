#include "uart.h"
#include "stm32f10x.h"
#include <stdio.h>

Device_Unit UART_Device;
char CMD=0;


/// 配置USART1接收中断
void NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure; 
	/* Configure the NVIC Preemption Priority Bits */  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
	/* Enable the USARTy Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;	 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}
 /**
  * @brief  USART1 GPIO 配置,工作模式配置。9600 8-N-1
  * @param  无
  * @retval 无
  */
void USART1_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
	/* config USART1 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
	
	/* USART1 GPIO config */
	/* Configure USART1 Tx (PA.09) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);    
	/* Configure USART1 Rx (PA.10) as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* USART1 mode config */
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);
	
	/* 使能串口1接收中断 */
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	
	USART_Cmd(USART1, ENABLE);
	NVIC_Configuration();
	printf("UART OK\r\n");
}

/// 重定向c库函数printf到USART1
int fputc(int ch, FILE *f)
{
		/* 发送一个字节数据到USART1 */
		USART_SendData(USART1, (uint8_t) ch);
		
		/* 等待发送完毕 */
		while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);		
	
		return (ch);
}

/// 重定向c库函数scanf到USART1
int fgetc(FILE *f)
{
		/* 等待串口1输入数据 */
		while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);

		return (int)USART_ReceiveData(USART1);
}

//---------------------------------------------------------------------//
void UART_Device_Init(void)
{		
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
	/* config USART1 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
	
	/* USART1 GPIO config */
	/* Configure USART1 Tx (PA.09) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);    
	/* Configure USART1 Rx (PA.10) as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* USART1 mode config */
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);
	
	USART_Cmd(USART1, ENABLE);
	printf("UART OK\r\n");
}

//---------------------------------------------------------------------//
u32 UART_Device_Read(u32 pos,void *buffer,u32 size)
{		
	u32 i;
    USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);	
	for(i=0;i<size;i++)
	{
		while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);
		*(((u8 *)buffer)+i) = (u8)USART_ReceiveData(USART1);
	}
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	return i;
}

//---------------------------------------------------------------------//
u32 UART_Device_Write(u32 pos,const void *buffer,u32 size)
{	
    u32 i;	
	for(i=0;i<size;i++)
	{
		while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
		USART_SendData(USART1, *(((u8 *)buffer)+i));
	}
	return i;
}

//---------------------------------------------------------------------//
void UART_Device_Control(u32 cmd, void *arg)
{	
	USART_InitTypeDef USART_InitStructure;
			
	USART_Cmd(USART1, DISABLE);
	
	switch((UART_CMD)cmd)
	{
		case BaudRate_CMD: 
		{
			USART_InitStructure.USART_BaudRate = *(u32 *)arg;
		};break;
		
		case WordLength_CMD:  
		{
			USART_InitStructure.USART_WordLength = *(u32 *)arg;
		};break;
		
		case StopBits_CMD:  
		{
			USART_InitStructure.USART_StopBits = *(u32 *)arg;
		};break;
		
		case Parity_CMD:  
		{
	        USART_InitStructure.USART_Parity = *(u32 *)arg;
		};break;
		
		default:  
		{
			USART_InitStructure.USART_BaudRate = 115200;
			USART_InitStructure.USART_WordLength = USART_WordLength_8b;
			USART_InitStructure.USART_StopBits = USART_StopBits_1;
			USART_InitStructure.USART_Parity = USART_Parity_No ;
		};break;
	}
	
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);	
	USART_Cmd(USART1, ENABLE);
}

//---------------------------------------------------------------------//

void UART_Device_Register(void)
{
	UART_Device.name    = "UART";
	UART_Device.init    = UART_Device_Init;
	UART_Device.open    = NULL;
	UART_Device.close   = NULL;
	UART_Device.read    = UART_Device_Read;
	UART_Device.write   = UART_Device_Write;
	UART_Device.control = UART_Device_Control;
	device_register(&UART_Device);
}
/*********************************************END OF FILE**********************/
