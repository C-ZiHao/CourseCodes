//@author C_Zihao
//@Data 2020.6.6  
//����6��ʱ��ʵ��
//���ܣ����ð˸��������ʾСʱ-����-�룬��09:18:59,����ĿҪ��֮�����Сʱ
//����˼��:����״̬������systickʵ��ÿ��ļ�ʱ������̬ɨ����ʾʱ��
//         ����sw1������systickÿ200ms����1s
//         ����sw2������systickÿ200ms����1min
//         ����sw1��sw2������systickÿ200msͬʱ����1s��1min

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

void Delay(uint32_t value);
void Display(int count, int num);    //��ʾ
void S800_GPIO_Init(void); 
void Testtime(void);                 //ʱ���λ�ж�
uint8_t I2C0_WriteByte(uint8_t DevAddr, uint8_t RegAddr, uint8_t WriteData);
uint8_t I2C0_ReadByte(uint8_t DevAddr, uint8_t RegAddr);
void S800_I2C0_Init(void);
volatile uint8_t result;
uint32_t ui32SysClock;
uint8_t seg7[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x40};
uint8_t led[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
uint8_t segchoose[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
volatile uint16_t systick_1000ms_couter = 1000;
volatile uint8_t systick_1000ms_status = false;
volatile uint16_t systick_200ms_couter = 200;
volatile uint8_t systick_200ms_status = false;
volatile uint16_t systick_1ms_couter;
volatile uint8_t systick_1ms_status = false;
int second = 0;  //�洢��
int minute = 0;  //�洢��
int hour = 0;    //�洢ʱ
int count = 0;   //count��̬ɨ����ʾ
int number = 0;  //number�������

int main(void)
{
	ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_16MHZ | SYSCTL_OSC_INT | SYSCTL_USE_OSC), 16000000);
	SysTickPeriodSet(ui32SysClock / 1000);
	SysTickEnable();
	SysTickIntEnable();
	IntMasterEnable();
	S800_GPIO_Init();
	S800_I2C0_Init();
	count = 0;
	while (1)
	{
		if (systick_1ms_status)
		{
			systick_1ms_status = 0;
			count += 1;
			if (count > 7)
				count = 0;
		}
		if (systick_1000ms_status)
		{
			systick_1000ms_status = 0;
			second += 1;
		}
		switch (count)//��̬ɨ��
		{
		case 0: number = hour / 10;break; //Сʱ
		case 1: number = hour % 10;break;
		case 2: number = 10;break; //number=10ʱ����ʾ"-"
		case 3: number = minute / 10;break; //����
		case 4: number = minute % 10;break;
		case 5: number = 10;break; //number=10����ʾ"-"
		case 6: number = second / 10;break; //��
		case 7: number = second % 10;break;
		}
		Display(segchoose[count], number); //��̬��ʾ
		Testtime();
	}
}
void Testtime(void)  //ʱ���λ�жϺ���
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
void SysTick_Handler(void)
{
	if (systick_1000ms_couter != 0)
		systick_1000ms_couter--;
	else
	{
		systick_1000ms_couter = 1000;
		systick_1000ms_status = 1;
	}

	if (systick_1ms_couter != 0)
		systick_1ms_couter--;
	else
	{
		systick_1ms_couter = 1;
		systick_1ms_status = 1;
	}
	//ͬʱ����SW1��SW2
	if (GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0) == 0 && GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_1) == 0) 
	{
		systick_1000ms_status = 0;
		if (systick_200ms_couter != 0)
			systick_200ms_couter--;
		else
		{
			systick_200ms_couter = 200;
			systick_200ms_status = 1;
		}
		if (systick_200ms_status) //ÿ200ms
		{
			systick_200ms_status = 0;
			second += 1;
			minute += 1; //ͬʱ��1
			Testtime();
		}
	}
	//����sw1
	if (GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0) == 0 && GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_1) != 0) 
	{
		systick_1000ms_status = 0;
		if (systick_200ms_couter != 0)
			systick_200ms_couter--;
		else
		{
			systick_200ms_couter = 200;
			systick_200ms_status = 1;
		}
		if (systick_200ms_status)//ÿ200ms
		{
			systick_200ms_status = 0;
			second += 1; //��+1
			Testtime();
		}
	}
	//����SW2
	if (GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0) != 0 && GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_1) == 0) 
	{
		systick_1000ms_status = 0;
		if (systick_200ms_couter != 0)
			systick_200ms_couter--;
		else
		{
			systick_200ms_couter = 200;
			systick_200ms_status = 1;
		}
		if (systick_200ms_status)//ÿ200ms
		{
			systick_200ms_status = 0;
			minute += 1; //��+1
			Testtime();
		}
	}
}

void Display(int counter, int num)  //��ʾ
{
	result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT2, 0x00); //�����Ӱ
	result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT1, seg7[num]);
	result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT2, (uint8_t)(counter));
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
	SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);		//��ʼ��i2cģ��
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);	//ʹ��I2Cģ��0����������ΪI2C0SCL--PB2��I2C0SDA--PB3
	GPIOPinConfigure(GPIO_PB2_I2C0SCL);				//����PB2ΪI2C0SCL
	GPIOPinConfigure(GPIO_PB3_I2C0SDA);				//����PB3ΪI2C0SDA
	GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2); //I2C��GPIO_PIN_2����SCL
	GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);	//I2C��GPIO_PIN_3����SDA

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
	};	//���I2C0ģ��æ���ȴ�
		//
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
	//��������Ҫ�ŵ������ϵĴӻ���ַ��false��ʾ����д�ӻ���true��ʾ�������ӻ�

	I2CMasterDataPut(I2C0_BASE, RegAddr);						  //����д�豸�Ĵ�����ַ
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START); //ִ���ظ�д�����
	while (I2CMasterBusy(I2C0_BASE))
	{
	};

	rop = (uint8_t)I2CMasterErr(I2C0_BASE); //������

	I2CMasterDataPut(I2C0_BASE, WriteData);
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH); //ִ���ظ�д�����������
	while (I2CMasterBusy(I2C0_BASE))
	{
	};

	rop = (uint8_t)I2CMasterErr(I2C0_BASE); //������

	return rop; //���ش������ͣ��޴���0
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
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND); //ִ�е���д�����
	while (I2CMasterBusBusy(I2C0_BASE))
		;
	rop = (uint8_t)I2CMasterErr(I2C0_BASE);
	Delay(1);
	//receive data
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, true);			//���ôӻ���ַ
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE); //ִ�е��ζ�����
	while (I2CMasterBusBusy(I2C0_BASE))
		;
	value = I2CMasterDataGet(I2C0_BASE); //��ȡ��ȡ������
	Delay(1);
	return value;
}
