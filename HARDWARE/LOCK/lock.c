#include "lock.h"

#include "delay.h"

uint8_t lock_flag;

static void lock_delay(void)
{
	delay_ms(1000);
}

void lock_init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	lock_lock();
}

void lock_lock(void)
{
	GPIO_ResetBits(GPIOE,GPIO_Pin_1);	

	lock_flag = 1;
}

void lock_unlock(void)
{
	
	GPIO_SetBits(GPIOE,GPIO_Pin_1);
	
	lock_flag = 0;
}

uint8_t lock_islock(void)
{
	return lock_flag;
}

//功能：蜂鸣器初始化
//参数：on　1：响　0不响
//返回：无
void Buzzer_Init(void)
{ 
    GPIO_InitTypeDef  GPIO_InitStructure;
    		

    //buzzer
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //使能PB8端口时钟

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;				 //
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
    GPIO_Init(GPIOB, &GPIO_InitStructure);					 //
    GPIO_ResetBits(GPIOB,GPIO_Pin_8);						 //
}

//功能：控制蜂鸣器
//参数：on　1：响　0不响
//返回：无
void Buzzer_on(int on){
    if(on>0) BUZZER=0;
    else BUZZER=1;
}

//功能：控制蜂鸣器响一声
//参数：无
//返回：无
void Buzzer_flash(void){

    Buzzer_on(0);
    delay_ms(200);
    Buzzer_on(1);
}


