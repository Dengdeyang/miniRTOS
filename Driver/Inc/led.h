#ifndef __LED_H
#define	__LED_H

#include "stm32f10x.h"


/* 直接操作寄存器的方法控制IO */
#define	digitalHi(p,i)			{p->BSRR=i;}			//设置为高电平		
#define digitalLo(p,i)			{p->BRR=i;}				//输出低电平
#define digitalToggle(p,i)		{p->ODR ^=i;}			//输出反转状态


/* 定义控制IO的宏 */
#define LED1_TOGGLE		digitalToggle(GPIOB,GPIO_Pin_12)
#define LED1_OFF		digitalHi(GPIOB,GPIO_Pin_12)
#define LED1_ON			digitalLo(GPIOB,GPIO_Pin_12)

#define GPIO_B10_TOGGLE		digitalToggle(GPIOB,GPIO_Pin_10)
#define GPIO_B11_TOGGLE		digitalToggle(GPIOB,GPIO_Pin_11)
#define GPIO_B12_TOGGLE		digitalToggle(GPIOB,GPIO_Pin_12)
#define GPIO_B13_TOGGLE		digitalToggle(GPIOB,GPIO_Pin_13)
#define GPIO_B14_TOGGLE		digitalToggle(GPIOB,GPIO_Pin_14)
#define GPIO_B15_TOGGLE		digitalToggle(GPIOB,GPIO_Pin_15)

void LED_GPIO_Config(void);
void Delay_ms(unsigned int ms);
void LED_Device_Register(void);

typedef enum
{ 
	LED_output = 0,
    LED_intput,
}LED_CMD;

#endif /* __LED_H */
