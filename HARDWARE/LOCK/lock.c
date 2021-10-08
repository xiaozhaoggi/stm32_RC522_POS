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

//���ܣ���������ʼ��
//������on��1���졡0����
//���أ���
void Buzzer_Init(void)
{ 
    GPIO_InitTypeDef  GPIO_InitStructure;
    		

    //buzzer
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //ʹ��PB8�˿�ʱ��

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;				 //
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //�������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
    GPIO_Init(GPIOB, &GPIO_InitStructure);					 //
    GPIO_ResetBits(GPIOB,GPIO_Pin_8);						 //
}

//���ܣ����Ʒ�����
//������on��1���졡0����
//���أ���
void Buzzer_on(int on){
    if(on>0) BUZZER=0;
    else BUZZER=1;
}

//���ܣ����Ʒ�������һ��
//��������
//���أ���
void Buzzer_flash(void){

    Buzzer_on(0);
    delay_ms(200);
    Buzzer_on(1);
}


