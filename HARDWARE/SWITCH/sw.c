#include "sw.h"

void sw_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	
	GPIO_Init(GPIOE, &GPIO_InitStructure);
}

uint8_t sw_check(void)
{
	if(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_0))//读取输入口PC13值)
		return SW_DOOR;
	else
	return SW_PURCHASE;
}
