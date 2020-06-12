
//@author C_Zihao
//@Data 2020.6.12 
//实验三

//题目要求：根据上位机的命令，作出相应回显，如：
//         上位机发送MAY+01 ，回显JUN
//         上位机发送MAY-06，回显 NOV
//保留原有时间显示与串口时钟功能
//额外添加：启动显示学号与显示“HELLo”

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "hw_memmap.h"
#include "debug.h"
#include "gpio.h"
#include "hw_i2c.h"
#include "hw_types.h"
#include "i2c.h"
#include "pin_map.h"
#include "sysctl.h"
#include "systick.h"
#include "interrupt.h"
#include "uart.h"
#include "hw_ints.h"
#include <string.h>

#define SYSTICK_FREQUENCY		1000			//1000hz
#define	I2C_FLASHTIME				500				//500mS
#define GPIO_FLASHTIME			300				//300mS
//*****************************************************************************
//
//I2C GPIO chip address and resigster define
//
//*****************************************************************************
#define TCA6424_I2CADDR 					0x22
#define PCA9557_I2CADDR						0x18

#define PCA9557_INPUT							0x00
#define	PCA9557_OUTPUT						0x01
#define PCA9557_POLINVERT					0x02
#define PCA9557_CONFIG						0x03

#define TCA6424_CONFIG_PORT0			0x0c
#define TCA6424_CONFIG_PORT1			0x0d
#define TCA6424_CONFIG_PORT2			0x0e

#define TCA6424_INPUT_PORT0				0x00
#define TCA6424_INPUT_PORT1				0x01
#define TCA6424_INPUT_PORT2				0x02

#define TCA6424_OUTPUT_PORT0			0x04
#define TCA6424_OUTPUT_PORT1			0x05
#define TCA6424_OUTPUT_PORT2			0x06

void Testtime(void);                 //时间进位判断
void Timeset(void);                 //时间设置
void Timeinc(void);
void Timeget(void);
void Timedisplay(void);    //时间显示
void Display(int count, int num);    //动态扫描演示
void initshow(void);  //初始化，显示学号 
void helloshow(void);  //初始化显示hello

void 		Delay(uint32_t value);
void 		S800_GPIO_Init(void);
void		S800_I2C0_Init(void);
void 		S800_UART_Init(void);
void UART0_Handler(void);
void UARTStringPut(const char *cMessage);
void UARTStringGet(void);
uint8_t UARTStringMonGet(void);
uint8_t counterinit=200;
uint8_t 	I2C0_WriteByte(uint8_t DevAddr, uint8_t RegAddr, uint8_t WriteData);
uint8_t 	I2C0_ReadByte(uint8_t DevAddr, uint8_t RegAddr);
char ASCII2Disp(char *buff);
//systick software counter define
volatile uint16_t systick_10ms_couter,systick_100ms_couter;
volatile uint8_t	systick_10ms_status,systick_100ms_status;
volatile uint8_t result,cnt,key_value,gpio_status;
volatile uint8_t rightshift = 0x01;
volatile uint8_t i=0;
volatile uint8_t DecOneMark=0;

int count = 0;   //count动态扫描显示
int number = 0;  //number负责输出

char Message[1000]={'\0'};
char RecMonth[3]={'\0'};
char RecTimeCnt[11]={'\0'};
uint32_t ui32SysClock;
uint8_t seg7[] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x40};
uint8_t segchoose[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
char *Month[]={"JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"}; //月份
uint8_t disp_buff[8];
uint8_t hour=12, minute=56, second=3;
uint8_t minutecnt=0, secondcnt=0;
uint8_t OriginMonth;
uint8_t PresentMonth;
bool TimeCntFlag;
//the disp_tab and disp_tab_7seg must be the Corresponding relation
//the last character should be the space(not display) character
char	const disp_tab[]			={'0','1','2','3','4','5',     //the could display char and its segment code   
															 '6','7','8','9','A','b',
															 'C','d','E','F',
															 'H','L','P','o',
															 '.','-','_',' '}; 
char const disp_tab_7seg[]	={0x3F,0x06,0x5B,0x4F,0x66,0x6D,  
															0x7D,0x07,0x7F,0x6F,0x77,0x7C,
															0x39,0x5E,0x79,0x71, 
															0x76,0x38,0x73,0x5c,
															0x80,0x40, 0x08,0x00}; 
int main(void)
{
	volatile uint16_t	i2c_flash_cnt,gpio_flash_cnt;
	//use internal 16M oscillator, PIOSC
   //ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_16MHZ |SYSCTL_OSC_INT |SYSCTL_USE_OSC), 16000000);		
	//ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_16MHZ |SYSCTL_OSC_INT |SYSCTL_USE_OSC), 8000000);		
	//use external 25M oscillator, MOSC
   //ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |SYSCTL_OSC_MAIN |SYSCTL_USE_OSC), 25000000);		

	//use external 25M oscillator and PLL to 120M
   //ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |SYSCTL_CFG_VCO_480), 120000000);;		
	ui32SysClock = SysCtlClockFreqSet((SYSCTL_OSC_INT | SYSCTL_USE_PLL |SYSCTL_CFG_VCO_480), 20000000);
	
  SysTickPeriodSet(ui32SysClock/SYSTICK_FREQUENCY);
	SysTickEnable();
	SysTickIntEnable();
  IntMasterEnable();		

	S800_GPIO_Init();
	S800_I2C0_Init();
	S800_UART_Init();
	
	IntEnable(INT_UART0);
	UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
	IntMasterEnable();
	
	IntPriorityGroupingSet(3);														//Set all priority to pre-emtption priority
	IntPrioritySet(INT_UART0,0);													//Set INT_UART0 to highest priority
	IntPrioritySet(FAULT_SYSTICK,0xe0);									//Set INT_SYSTICK to lowest priority
	
	
	while(counterinit>0)
	{
		counterinit=counterinit-1;
	  initshow();   //显示学号
  }
	counterinit=1000;
	
	while(counterinit>0)
	{
		counterinit=counterinit-1;
	  helloshow();   //显示学号
  }
	
 	while (1)
	{ 
		//常规显示
		Timedisplay();
		//常规计时
		if (systick_100ms_status)
		{
			systick_100ms_status=0;
			if (++i2c_flash_cnt >=10)
			{
			i2c_flash_cnt=0;
			second++;
			}
		}
		Testtime();
		
		if ((strncmp(Message,"SET",3)==0) || (strncmp(Message,"set",3)==0))
		{
			Timeset();
		}
    
		if ((strncmp(Message,"inc",3)==0) ||(strncmp(Message,"INC",3)==0) )
    {
			Timeinc();
		}
		if ((strcmp(Message,"GETTIME")==0)||(strcmp(Message,"gettime")==0))
		{
			Timeget();
		}
		
		if(OriginMonth!=0) //月份加减
		{
			if(Message[3]=='+')
			{
				PresentMonth=OriginMonth +(Message[4]-'0')*10+(Message[5]-'0');
				if(PresentMonth>12)
					PresentMonth-=12;
				OriginMonth=0;
				UARTStringPut(Month[PresentMonth-1]);
			}
			else if(Message[3]=='-')
			{ 
				if(OriginMonth>((Message[4]-'0')*10+(Message[5]-'0')))
					PresentMonth=OriginMonth -(Message[4]-'0')*10-(Message[5]-'0');
				else
					PresentMonth=OriginMonth -(Message[4]-'0')*10-(Message[5]-'0')+12;
				OriginMonth=0;
				UARTStringPut(Month[PresentMonth-1]);
			}
			else 
			{
				UARTStringPut((uint8_t *)"\r\nWRONG!\r\n");
				OriginMonth=0;
			}
			memset(Message, 0, sizeof(Message));
		}
		
	}

}

void helloshow(void)
{
	  
	  count=0;
	  while(count<5)
		{
	    switch (count)//动态扫描
	  	{
	  	case 0: number = 16;break; //h
		  case 1: number = 14;break; //e
		  case 2: number = 17;break; //l
		  case 3: number = 17;break; //l
		  case 4: number = 19;break; //o,为了与0区分，没有使用大写
		}
		Display(segchoose[count], number); //动态显示
		Delay(10000);
		count++;
	}
		
}

void Timedisplay(void)
{
	  count=0;
	  while(count<8)
		{
	    switch (count)//动态扫描
	  	{
	  	case 0: number = hour / 10;break; //小时
		  case 1: number = hour % 10;break;
		  case 2: number = 21;break; //number=10时，显示"-"
		  case 3: number = minute / 10;break; //分钟
		  case 4: number = minute % 10;break;
		  case 5: number = 21;break; //number=10，显示"-"
		  case 6: number = second / 10;break; //秒
		  case 7: number = second % 10;break;
		}
		Display(segchoose[count], number); //动态显示
		Delay(3000);
		count++;
	}
		
}
void Display(int counter, int num)  //演示
{
	result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT2, 0x00); //清除残影
	result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT1, disp_tab_7seg[num]);
	result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT2, (uint8_t)(counter));
}
void initshow()
{
    count=0;
	  while(count<8)
		{
	    switch (count)//动态扫描
	  	{
	  	case 0: number = 2;break; //小时
		  case 1: number = 1;break;
		  case 2: number = 9;break; //number=10时，显示"-"
		  case 3: number = 1;break; //分钟
		  case 4: number = 1;break;
		  case 5: number = 1;break; //number=10，显示"-"
		  case 6: number = 8;break; //秒
		  case 7: number = 7;break;
		}
		Display(segchoose[count], number); //动态显示
		Delay(10000);
		count++;
	}
}


void Testtime(void)  //时间进位判断函数
{
	if (second > 59)
	{
		second = 0;
		minute += 1;
	}
	if (minute > 59)
	{
		minute = 0;
		hour += 1;
	}
	if (hour > 23)
	{
		hour = 0;
	}
}

void Timeset(void)  //时间设置
{
      hour=(Message[3]-'0')*10+(Message[4]-'0');
		  minute=(Message[6]-'0')*10+(Message[7]-'0');
		  second=(Message[9]-'0')*10+(Message[10]-'0');
			Message[0]='T';Message[1]='I';Message[2]='M';Message[3]='E';
	    Message[4]=(hour/10)+'0';Message[5]=(hour%10)+'0';Message[6]=':';
			Message[7]=(minute/10)+'0';Message[8]=(minute%10)+'0';Message[9]=':';
			Message[10]=(second/10)+'0';Message[11]=(second%10)+'0';Message[12]='\0';
			UARTStringPut(Message);
			memset(Message, 0, sizeof(Message));    
}
void Timeinc(void)  //时间增加设置
{
    hour=(Message[3]-'0')*10+(Message[4]-'0')+hour;
		minute=(Message[6]-'0')*10+(Message[7]-'0')+minute;
		second=(Message[9]-'0')*10+(Message[10]-'0')+second;
		if (second>59) {minute++; second=second-60;}
		if (minute>59) {hour++; minute=minute-60;}
		if (hour>23) {hour=hour-24;}
		Message[0]='T';Message[1]='I';Message[2]='M';Message[3]='E';
	  Message[4]=(hour/10)+'0';Message[5]=(hour%10)+'0';Message[6]=':';
	  Message[7]=(minute/10)+'0';Message[8]=(minute%10)+'0';Message[9]=':';
	  Message[10]=(second/10)+'0';Message[11]=(second%10)+'0';Message[12]='\0';
		UARTStringPut(Message);
		memset(Message, 0, sizeof(Message));   
}
void Timeget(void)  //时间获取
{
    Message[0]='T';Message[1]='I';Message[2]='M';Message[3]='E';
	  Message[4]=(hour/10)+'0';Message[5]=(hour%10)+'0';Message[6]=':';
	  Message[7]=(minute/10)+'0';Message[8]=(minute%10)+'0';Message[9]=':';
	  Message[10]=(second/10)+'0';Message[11]=(second%10)+'0';Message[12]='\0';
		UARTStringPut(Message);
		memset(Message, 0, sizeof(Message));    
}
void UART0_Handler(void)
{
	int32_t uart0_int_status;
	uart0_int_status= UARTIntStatus(UART0_BASE, true);	// Get the interrrupt status
	UARTIntClear(UART0_BASE, uart0_int_status);//Clear the asserted interrupt 
	UARTStringGet();
	OriginMonth=UARTStringMonGet();

	GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,GPIO_PIN_1 );
	Delay(1000);
  GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,0);
}

void SysTick_Handler(void)
{ 
  if (systick_100ms_couter	!= 0)
		systick_100ms_couter--;
	else
	{
		systick_100ms_couter	= SYSTICK_FREQUENCY/10;
		systick_100ms_status 	= 1;
	}
	
	if (systick_10ms_couter	!= 0)
		systick_10ms_couter--;
	else
	{
		systick_10ms_couter	= SYSTICK_FREQUENCY/100;
		systick_10ms_status = 1;
	}
	
	if (GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0) == 0)
	{
		systick_100ms_status	= systick_10ms_status = 0;
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0,GPIO_PIN_0);		
	}
	else
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0,0);		
}

char ASCII2Disp(char *buff)
{
	char * pcDisp;
	pcDisp 				=(char*)strchr(disp_tab,*buff);	
	if (pcDisp		== NULL)
		return 0x0;
	else
		return (disp_tab_7seg[pcDisp-disp_tab]);

}

void Delay(uint32_t value)
{
	uint32_t ui32Loop;
	for(ui32Loop = 0; ui32Loop < value; ui32Loop++){};
}


void UARTStringPut(const char *cMessage)
{
	while(*cMessage!='\0')
		UARTCharPut(UART0_BASE,*(cMessage++));
}

uint8_t UARTStringMonGet(void)
{ 
  for(i=0;i<3;++i)
	{
		RecMonth[i]=Message[i];
	}
	if(strcmp(RecMonth,Month[0])==0) //比对目前月份
	{return 1;}
	else if(strcmp(RecMonth,Month[1])==0)
	{return 2;}
	else if(strcmp(RecMonth,Month[2])==0)
	{return 3;}
	else if(strcmp(RecMonth,Month[3])==0)
	{return 4;}
	else if(strcmp(RecMonth,Month[4])==0)
	{return 5;}
	else if(strcmp(RecMonth,Month[5])==0)
	{return 6;}
	else if(strcmp(RecMonth,Month[6])==0)
	{return 7;}
	else if(strcmp(RecMonth,Month[7])==0)
	{return 8;}
	else if(strcmp(RecMonth,Month[8])==0)
	{return 9;}
	else if(strcmp(RecMonth,Month[9])==0)
	{return 10;}
	else if(strcmp(RecMonth,Month[10])==0)
	{return 11;}
	else if(strcmp(RecMonth,Month[11])==0)
	{return 12;}
	else return 0;
  
}


void UARTStringGet(void)
{ 
	i=0;
  while(UARTCharsAvail(UART0_BASE))
	{
		Message[i]=UARTCharGet(UART0_BASE);
		i++;
	}
		Message[i]='\0';
		i=0;	
}
void S800_UART_Init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);						//Enable PortA
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));			//Wait for the GPIO moduleA ready

	GPIOPinConfigure(GPIO_PA0_U0RX);												// Set GPIO A0 and A1 as UART pins.
  GPIOPinConfigure(GPIO_PA1_U0TX);    			
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	// Configure the UART for 115,200, 8-N-1 operation.
  UARTConfigSetExpClk(UART0_BASE, ui32SysClock,115200,(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |UART_CONFIG_PAR_NONE));
	UARTStringPut((uint8_t *)"\r\nHello, world!\r\n");
	UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX1_8, UART_FIFO_RX7_8);
}
void S800_GPIO_Init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);						//Enable PortF
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));			//Wait for the GPIO moduleF ready
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);						//Enable PortJ	
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ));			//Wait for the GPIO moduleJ ready	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);						//Enable PortN	
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));			//Wait for the GPIO moduleN ready		
	
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);			//Set PF0 as Output pin
  GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0|GPIO_PIN_1);			//Set PN0 as Output pin

	GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE,GPIO_PIN_0 | GPIO_PIN_1);//Set the PJ0,PJ1 as input pin
	GPIOPadConfigSet(GPIO_PORTJ_BASE,GPIO_PIN_0 | GPIO_PIN_1,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
}

void S800_I2C0_Init(void)
{
	uint8_t result;
  SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	GPIOPinConfigure(GPIO_PB2_I2C0SCL);
  GPIOPinConfigure(GPIO_PB3_I2C0SDA);
  GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
  GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

	I2CMasterInitExpClk(I2C0_BASE,ui32SysClock, true);										//config I2C0 400k
	I2CMasterEnable(I2C0_BASE);	

	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_CONFIG_PORT0,0x0ff);		//config port 0 as input
	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_CONFIG_PORT1,0x0);			//config port 1 as output
	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_CONFIG_PORT2,0x0);			//config port 2 as output 

	result = I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_CONFIG,0x00);					//config port as output
	result = I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT,0x0ff);				//turn off the LED1-8
	
}


uint8_t I2C0_WriteByte(uint8_t DevAddr, uint8_t RegAddr, uint8_t WriteData)
{
	uint8_t rop;
	while(I2CMasterBusy(I2C0_BASE)){};
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
	I2CMasterDataPut(I2C0_BASE, RegAddr);
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
	while(I2CMasterBusy(I2C0_BASE)){};
	rop = (uint8_t)I2CMasterErr(I2C0_BASE);

	I2CMasterDataPut(I2C0_BASE, WriteData);
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
	while(I2CMasterBusy(I2C0_BASE)){};

	rop = (uint8_t)I2CMasterErr(I2C0_BASE);
	return rop;
}

uint8_t I2C0_ReadByte(uint8_t DevAddr, uint8_t RegAddr)
{
	uint8_t value,rop;
	while(I2CMasterBusy(I2C0_BASE)){};	
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
	I2CMasterDataPut(I2C0_BASE, RegAddr);
//	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);		
	I2CMasterControl(I2C0_BASE,I2C_MASTER_CMD_SINGLE_SEND);
	while(I2CMasterBusBusy(I2C0_BASE));
	rop = (uint8_t)I2CMasterErr(I2C0_BASE);
	Delay(1);
	//receive data
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, true);
	I2CMasterControl(I2C0_BASE,I2C_MASTER_CMD_SINGLE_RECEIVE);
	while(I2CMasterBusBusy(I2C0_BASE));
	value=I2CMasterDataGet(I2C0_BASE);
		Delay(1);
	return value;
}