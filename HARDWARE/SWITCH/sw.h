#ifndef __SW_H_
#define __SW_H_

#include "stm32f10x.h"
#include "sys.h"
#define SW_DOOR			0
#define SW_PURCHASE	1

void sw_init(void);
uint8_t sw_check(void);

#endif
