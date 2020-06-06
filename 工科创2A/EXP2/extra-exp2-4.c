//@author C_Zihao
//@Data 2020.6.6
//讨论4
//利用中断实现双跑马灯，按下sw1，跑马暂停

//题目要求：第一步 第1，2个数码管显示1，2 ，LED1，2 点亮
//       第二步 第2，3个数码管显示2，3，LED2，3 点亮
//       第八步 第8，1个数码管显示8，1， LED8，1点亮
//       第九步 回到第一步

//核心思想：利用两个systick中断，第一个中断设置时间较长，用来控制数字变化，第二个中断设置时间较短同时实现两个数字及led的动态显示


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
#include "sysTick.h"
#include "interrupt.h"


#define TCA6424_I2CADDR 0x22
#define PCA9557_I2CADDR 0x18

#define PCA9557_INPUT 0x00
#define PCA9557_OUTPUT 0x01
#define PCA9557_POLINVERT 0x02
#define PCA9557_CONFIG 0x03

#define TCA6424_CONFIG_PORT0 0x0c
#define TCA6424_CONFIG_PORT1 0x0d
#define TCA6424_CONFIG_PORT2 0x0e

#define TCA6424_INPUT_PORT0 0x00
#define TCA6424_INPUT_PORT1 0x01
#define TCA6424_INPUT_PORT2 0x02

#define TCA6424_OUTPUT_PORT0 0x04
#define TCA6424_OUTPUT_PORT1 0x05
#define TCA6424_OUTPUT_PORT2 0x06


void S800_GPIO_Init(void);
uint8_t I2C0_WriteByte(uint8_t DevAddr, uint8_t RegAddr, uint8_t WriteData);
uint8_t I2C0_ReadByte(uint8_t DevAddr, uint8_t RegAddr);
void S800_I2C0_Init(void);
volatile uint8_t result;
uint32_t ui32SysClock;
uint8_t seg7[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x77, 0x7c, 0x58, 0x5e, 0x079, 0x71, 0x5c};
uint8_t led[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
volatile uint16_t systick_1000ms_couter;
volatile uint8_t systick_1000ms_status = false;

volatile uint16_t systick_1ms_couter;  
volatile uint8_t systick_1ms_status = false;
int display_counter = 0;  //用来控制第一个led及数码管
int counter2;  //counter2用来控制第二个led及数码管
int num = 1; //num用来动态反转
int main(void)
{
	//use internal 16M oscillator, HSI
	ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_16MHZ | SYSCTL_OSC_INT | SYSCTL_USE_OSC), 16000000);
	SysTickPeriodSet(ui32SysClock / 1000);
	SysTickEnable();
	SysTickIntEnable();
	IntMasterEnable();
	S800_GPIO_Init();
	S800_I2C0_Init();

	while (1)
	{

		if (systick_1000ms_status)  //数字变换
		{
			systick_1000ms_status = 0;
			display_counter += 1;
			if (display_counter > 8)
			{
				display_counter = 1;
			}

			counter2 = display_counter + 1;  //counter2用来显示第二个数字
			if (counter2 > 8)
			{
				counter2 = 1;       //特殊处理，归0
			}
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, ~GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0)); // Turn on the PF0
			
		}

		if (systick_1ms_status)  //动态扫描
		{
			systick_1ms_status = 0;
			result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT2, 0x00);//消除残影
			if (num == 2)
			{
				num = 1;
				result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT1, seg7[counter2]);
				result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT2, led[counter2 - 1]);
				result = I2C0_WriteByte(PCA9557_I2CADDR, PCA9557_OUTPUT, ~(led[counter2 - 1]));
			}
			else
			{
				num = 2;
				result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT1, seg7[display_counter]);
				result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT2, led[display_counter - 1]);
				result = I2C0_WriteByte(PCA9557_I2CADDR, PCA9557_OUTPUT, ~(led[display_counter - 1]));
			}
		}
	}
}

void SysTick_Handler(void)
{
	if (systick_1000ms_couter != 0)
		systick_1000ms_couter--;
	else
	{
		systick_1000ms_couter = 1000;   //数字变换控制
		systick_1000ms_status = 1;
	}

	if (systick_1ms_couter != 0)
		systick_1ms_couter--;
	else
	{
		systick_1ms_couter = 1;  //为了动态扫描效果，设置的比较小
		systick_1ms_status = 1;
	}
}

void Delay(uint32_t value)
{
	uint32_t ui32Loop;
	for (ui32Loop = 0; ui32Loop < value; ui32Loop++)
	{
	};
}

void S800_GPIO_Init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); //Enable PortF
	while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF))
		;										 //Wait for the GPIO moduleF ready
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); //Enable PortJ
	while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ))
		; //Wait for the GPIO moduleJ ready

	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);				//Set PF0 as Output pin
	GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1); //Set the PJ0,PJ1 as input pin
	GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

void S800_I2C0_Init(void)
{
	uint8_t result;
	SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);		//初始化i2c模块
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);	//使用I2C模块0，引脚配置为I2C0SCL--PB2、I2C0SDA--PB3
	GPIOPinConfigure(GPIO_PB2_I2C0SCL);				//配置PB2为I2C0SCL
	GPIOPinConfigure(GPIO_PB3_I2C0SDA);				//配置PB3为I2C0SDA
	GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2); //I2C将GPIO_PIN_2用作SCL
	GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);	//I2C将GPIO_PIN_3用作SDA

	I2CMasterInitExpClk(I2C0_BASE, ui32SysClock, true); //config I2C0 400k
	I2CMasterEnable(I2C0_BASE);

	result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_CONFIG_PORT0, 0x0ff); //config port 0 as input
	result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_CONFIG_PORT1, 0x0);   //config port 1 as output
	result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_CONFIG_PORT2, 0x0);   //config port 2 as output

	result = I2C0_WriteByte(PCA9557_I2CADDR, PCA9557_CONFIG, 0x00);	 //config port as output
	result = I2C0_WriteByte(PCA9557_I2CADDR, PCA9557_OUTPUT, 0x0ff); //turn off the LED1-8
}

uint8_t I2C0_WriteByte(uint8_t DevAddr, uint8_t RegAddr, uint8_t WriteData)
{
	uint8_t rop;
	while (I2CMasterBusy(I2C0_BASE))
	{
	}; //如果I2C0模块忙，等待
	   //
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
	//设置主机要放到总线上的从机地址。false表示主机写从机，true表示主机读从机

	I2CMasterDataPut(I2C0_BASE, RegAddr);						  //主机写设备寄存器地址
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START); //执行重复写入操作
	while (I2CMasterBusy(I2C0_BASE))
	{
	};

	rop = (uint8_t)I2CMasterErr(I2C0_BASE); //调试用

	I2CMasterDataPut(I2C0_BASE, WriteData);
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH); //执行重复写入操作并结束
	while (I2CMasterBusy(I2C0_BASE))
	{
	};

	rop = (uint8_t)I2CMasterErr(I2C0_BASE); //调试用

	return rop; //返回错误类型，无错返回0
}

uint8_t I2C0_ReadByte(uint8_t DevAddr, uint8_t RegAddr)
{
	uint8_t value, rop;
	while (I2CMasterBusy(I2C0_BASE))
	{
	};
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
	I2CMasterDataPut(I2C0_BASE, RegAddr);
	//	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND); //执行单词写入操作
	while (I2CMasterBusBusy(I2C0_BASE))
		;
	rop = (uint8_t)I2CMasterErr(I2C0_BASE);
	Delay(1);
	//receive data
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, true);			//设置从机地址
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE); //执行单次读操作
	while (I2CMasterBusBusy(I2C0_BASE))
		;
	value = I2CMasterDataGet(I2C0_BASE); //获取读取的数据
	Delay(1);
	return value;
}
