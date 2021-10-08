#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "delay.h"
#include "sys.h"
#include "usart.h"

#include "rc522.h"
#include "lcd.h"
#include "lock.h"
#include "sw.h"

#include "lcd.h"
#include "usart.h"	 
#include "touch.h"


#define BUFFERSIZE 	256
#define FONT_SIZE		24
#define XOR_CHK_MAGIC_NUMBER 0xA5

struct Ret_dialog{
	uint32_t money;
	uint8_t action;
};//Action = 1: store, Action = 2: spend
		
//Round Card ID: d0 38 c7 73
uint8_t known_card_id[7][4] = {
	{0x00, 0x00, 0x00, 0x00},
	{0xd0, 0x38, 0xc7, 0x73},
	{0x2e, 0x90, 0xe5, 0xe1},
	{0xae, 0x6a, 0x74, 0x95},
	{0xb2, 0x2f, 0xb6, 0x9e},
	{0x43, 0x5c, 0xa2, 0x18},
	{0x3c, 0x0d, 0x4d, 0x49}
};
char *known_card_string[7] = {
	"",
	"Round",
	"Student",
	"ICBC", 
	"XJTU",
	"BOCOM",
	"HUAZHU"
};
const uint8_t known_card_number = 7;

char buf[32];
uint8_t stage;
uint8_t rt_val;

uint8_t sw_flag;
/**
*   连线说明：
*   1--SDA  <----->PA4
*   2--SCK  <----->PA5
*   3--MOSI <----->PA7
*   4--MISO <----->PA6
*   5--悬空
*   6--GND <----->GND
*   7--RST <----->PB0
*   8--VCC <----->VCC
**/
void Initializer(void);
//Return value equaling to a non-zero value represents error happened, need to CONTINUE;
uint8_t open_door(void);
uint8_t purchase_add(void);


void set_money(uint32_t);
//Inner functions
static void print_formatted(char *, ...);
static uint8_t xor_check(uint8_t *);
static void clean_bytes(void *, size_t);
static void sector_dump(uint8_t *);
static void delay_s(uint8_t );

static void draw_interface(void);
static uint8_t judge_touch(int , int );
static struct Ret_dialog dialog(void);

int main(void)
{
	
	uint8_t previous_card_id[4];
	
	Initializer();//初始化硬件
	
	clean_bytes(previous_card_id, 4);
	
	while(1){
		stage = 0; //LCD行显示数
		sw_flag = sw_check();//切换门禁、消费功能
		//Always purchase, the sw GPIO have conflict with touch screen
		//sw_flag = SW_DOOR;
		
		LCD_ShowString(0, 30*FONT_SIZE, 400, 16, 16, lock_islock()?(u8*)"Locked":(u8*)"Unlocked");
		LCD_ShowString(0, 32*FONT_SIZE, 400, 16, 16, sw_flag==SW_DOOR?(u8*)"Door    ":(u8*)"Purchase");
		
		//Read Card
		rt_val = PcdRequest(PICC_REQALL,CT);
		if(rt_val == MI_OK)//是否寻卡成功
			LCD_Clear(WHITE);
		print_formatted("Reading Card...");
		
		//Card Detect
		if(rt_val != MI_OK){//如果寻卡失败
			print_formatted("Card Detect Failed...");
			clean_bytes(previous_card_id, 4);
			continue;
		}
		print_formatted("Card Detected...");//寻卡成功打印信息
		
		//Anti-collide
		rt_val = PcdAnticoll(SN);// 防冲撞  读取卡号
		if(rt_val != MI_OK){//如果读取卡号失败
			print_formatted("Anti-collision Failed...");
			continue;
		}
		print_formatted("Anti-collision...");//读取卡号成功
		
		//Print The Card SN
		print_formatted("Card ID: %02x %02x %02x %02x...", SN[0], SN[1], SN[2], SN[3]);//显示卡号
		
		if(0 == strncmp((char *)SN, (char *)previous_card_id, 4)){ //如果和上一张读的卡号一样
			print_formatted("The Card Is Previous One.");
			continue;
		}
		
		//Select Specific Card
		rt_val = PcdSelect(SN);//选择卡号为SN的卡
		if(rt_val != MI_OK){//如果选择失败
			print_formatted("Card Select Failed...");
			continue;
		}
		print_formatted("Card Selected...");
		
		//Open the door
		if(sw_flag == SW_DOOR){//如果当前为门禁功能
			if(open_door())
				continue;
		}else{//如果当前为消费功能
			//Purchase
			if(purchase_add())
				continue;
		}

		rt_val = PcdHalt();//卡进入休眠
		print_formatted("Card Halted...");
		delay_ms(1500);
		LCD_Clear(WHITE);
		
	}
}

void Initializer(void)
{
	delay_init();	    	 //延时函数初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口初始化为115200
	
	LCD_Init();
	RC522_Init();       //初始化射频卡模块
	lock_init();
	sw_init();
	Buzzer_Init();
	tp_dev.init();

	LCD_Clear(WHITE);
}

uint8_t open_door(void)
{
	uint8_t known_card = 0;

	for(int i = 1; i < known_card_number; i++){
	if(0 == strncmp((char *)SN, (char *)known_card_id[i], 4)){//如果是已知卡号
			sprintf((char *)buf, "%s Card Detected, Unlocking...", known_card_string[i]);
			known_card = i;
			break;
		}
	}
	if(!known_card)//如果不是已知卡号
	{
		Buzzer_flash();
		strcpy((char *)buf, "Unknown Card");
	}
	
	print_formatted(buf);
	
	
	if(known_card){//如果是已知卡号
		lock_unlock();//开锁
		Buzzer_flash();
		delay_s(3);//延时5秒后再上锁
		print_formatted("Locking...");
		lock_lock();
	}
	return 0;
}

uint8_t purchase_add(void)
{
	uint8_t dat[16];
	uint32_t tmp;
	struct Ret_dialog r;
	
	//Verify The Card
	rt_val = PcdAuthState(0x60,0x09,KEY,SN);//验证卡密码
	if(rt_val != MI_OK){//如果不正确
		print_formatted("Card Verify Failed...");	
		return 0xff;
	}
	print_formatted("Card Verified...");//验证密码正确

//	set_money(0xabcdef00);
	
	rt_val = PcdRead(0x09, dat);//读出0x09 里面的 16个字节的数据
	sector_dump(dat);
	
	//Check
	if(xor_check(dat)){//校验数据
		print_formatted("XOR Check Successfully...");
	}else{
		print_formatted("XOR Check Failed...");
		return 0xff;
	}
	
	tmp = (uint32_t)dat[0] + (((uint32_t)dat[1])<<8) + (((uint32_t)dat[2])<<16) + (((uint32_t)dat[3])<<24);
	//把数据转换成金额
	

	r = dialog();//绘制图形界面
	stage = 0;
	LCD_Clear(WHITE);
	print_formatted("Current Money: $%u", tmp);//当前金额
	print_formatted("Action=%hu, Money=%u", r.action, r.money);//操作、金额
	switch(r.action){//根据不同的操作打印不同数据
		case 1://Store
			tmp += r.money;
			set_money(tmp);
		print_formatted("Store money.");
			print_formatted("Current money:$%u", tmp);
			break;
		case 2://Spend
			if(r.money>tmp){
				print_formatted("Money is not enough.");
			}else{
				tmp -= r.money;
				set_money(tmp);
				print_formatted("Spend $%u", r.money);
				print_formatted("Remain $%u",tmp);
			}
			break;
	}
	delay_ms(1000);
	delay_ms(1000);
	delay_ms(1000);
	delay_ms(1000);
	delay_ms(1000);
	return 0;
}


void set_money(uint32_t m)
{
	uint8_t dat[16];
	uint8_t xor_chk = XOR_CHK_MAGIC_NUMBER;
	
	clean_bytes(dat, 16);
	
	for(int i = 0; i < 4; i++){
		dat[i] = (uint8_t)(m&0xff);
		m >>= 8;
		
		xor_chk ^= dat[i];
	}
	dat[4] = xor_chk;
	
	PcdWrite(0x09, dat);

}

static void print_formatted(char *str, ...)
{
	char *buf[BUFFERSIZE];

	va_list va;
	
	va_start(va, str);
	
	vsprintf((char *)buf, str, va);
	LCD_ShowString(0,++stage*FONT_SIZE,400,16,16,(u8*)buf);	
	
	va_end(va);
}	

static uint8_t xor_check(uint8_t *p)
{
	uint8_t xor_chk = XOR_CHK_MAGIC_NUMBER;
	
	for(int i = 0; i < 4; i++){
		xor_chk ^= p[i];
	}
	
	return xor_chk == p[4];
}

static void sector_dump(uint8_t *dat)
{
	print_formatted("Block +00: %02x %02x %02x %02x...", dat[0x0], dat[0x1], dat[0x2], dat[0x3]);		
	print_formatted("Block +04: %02x %02x %02x %02x...", dat[0x4], dat[0x5], dat[0x6], dat[0x7]);
	print_formatted("Block +08: %02x %02x %02x %02x...", dat[0x8], dat[0x9], dat[0xa], dat[0xb]);
	print_formatted("Block +0C: %02x %02x %02x %02x...", dat[0xc], dat[0xd], dat[0xe], dat[0xf]);
}


static void clean_bytes(void *p, size_t s)
{
	memset(p, 0, s);
}

static void delay_s(uint8_t n)
{
	while(n--){
		delay_ms(1000);
	}
}

static void draw_interface(void)
{
	uint8_t cnt = 0;
	LCD_Clear(WHITE);
	POINT_COLOR=BLACK;
	
	cnt=0;LCD_DrawLine(0, 0+cnt*64, 240, 0+cnt*64);
	++cnt;LCD_DrawLine(0, 0+cnt*64, 240, 0+cnt*64);
	++cnt;LCD_DrawLine(0, 0+cnt*64, 240, 0+cnt*64);
	++cnt;LCD_DrawLine(0, 0+cnt*64, 240, 0+cnt*64);
	++cnt;LCD_DrawLine(0, 0+cnt*64, 240, 0+cnt*64);
	++cnt;LCD_DrawLine(0, 0+cnt*64, 240, 0+cnt*64);
	cnt=0;LCD_DrawLine(cnt*80, 0+64, cnt*80, 320);
	++cnt;LCD_DrawLine(cnt*80, 0+64, cnt*80, 320);
	++cnt;LCD_DrawLine(cnt*80, 0+64, cnt*80, 320);
	
	cnt=0;LCD_ShowString(40,0+cnt*64+96,200,16,16,(u8*)"1");
	++cnt;LCD_ShowString(40,0+cnt*64+96,200,16,16,(u8*)"4");
	++cnt;LCD_ShowString(40,0+cnt*64+96,200,16,16,(u8*)"7");
	
	cnt=0;LCD_ShowString(120,0+cnt*64+96,200,16,16,(u8*)"2");
	++cnt;LCD_ShowString(120,0+cnt*64+96,200,16,16,(u8*)"5");
	++cnt;LCD_ShowString(120,0+cnt*64+96,200,16,16,(u8*)"8");
	++cnt;LCD_ShowString(120,0+cnt*64+96,200,16,16,(u8*)"0");
	
	cnt=0;LCD_ShowString(200,0+cnt*64+96,200,16,16,(u8*)"3");
	++cnt;LCD_ShowString(200,0+cnt*64+96,200,16,16,(u8*)"6");
	++cnt;LCD_ShowString(200,0+cnt*64+96,200,16,16,(u8*)"9");
	
	LCD_ShowString(20, 0+3*64+96, 200, 16, 16, (u8*)"STORE");
	LCD_ShowString(180, 0+3*64+96, 200, 16, 16, (u8*)"SPEND");
	LCD_ShowString(10, 32, 200, 16, 16, (u8*)"CLEAN");
}
//ret: 1-9->1-9, 10->store, 11->0, 12->spend, 13->Del, 0->Nothing
static uint8_t judge_touch(int x, int y)
{
	if(y > 480)
		/*return 0*/;//Nothing happened*/
	else if(y < 64){
		return 13;
	}else if(y < 128){
		if(x<80)
			return 1;
		else if(x< 160)
			return 2;
		else
			return 3;
	}else if(y < 192){
		if(x<80)
			return 4;
		else if(x< 160)
			return 5;
		else
			return 6;
	}else if(y < 256){
		if(x<80)
			return 7;
		else if(x<160)
			return 8;
		else
			return 9;
	}else{//if(y < 480+320)
		if(x<80)
			return 10;
		else if(x<160)
			return 11;
		else
			return 12;
	}
}

static struct Ret_dialog dialog(void)
{
	int x, y;
	int state;
	int i, cnt;
	
	static struct Ret_dialog ret = {0,0};
	ret.money = 0;ret.action = 0;
	
	uint8_t buf[128];
	draw_interface();
	
	while(1){
		tp_dev.scan(0);
		
//		if((tp_dev.sta)&(1<<0)){
		if(tp_dev.sta){
			x = tp_dev.x[0];
			y = tp_dev.y[0];
			do{
				cnt = 0;
				for(i = 0; i < 100; i++){
					tp_dev.scan(0);
					if(tp_dev.sta&0x01)
						cnt++;
				}
			}while(cnt);
		}else{
			x = y = 0;
		}
		state = judge_touch(x, y);

		switch(state){
			case 0:
				break;
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				ret.money = ret.money*10+state;
				break;
			case 10:
				ret.action = 1;
				break;
			case 11:
				ret.money *= 10;
				break;
			case 12:
				ret.action = 2;
				break;
			case 13:
				ret.money = 0;
				break;
		}
		sprintf((char *)buf, "%10u", ret.money);
		LCD_ShowString(160,32, 200, 16, 16, buf);
		
		if(ret.action)
			break;
	}
	
	return ret;
}

