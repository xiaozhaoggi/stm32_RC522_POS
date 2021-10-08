#ifndef __LOCK_H_
#define __LOCK_H_

#include "stm32f10x.h"

#define BUZZER PBout(8)// PC1	

#define BUZZER_ON (BUZZER=0)//��
#define BUZZER_OFF (BUZZER=1)//�ر�


void Buzzer_Init(void);//��ʼ��
void Buzzer_on(int on);
void Buzzer_flash(void);

void lock_init(void);
void lock_lock(void);
void lock_unlock(void);

uint8_t lock_islock(void);

#endif
