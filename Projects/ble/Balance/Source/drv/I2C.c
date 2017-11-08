
#include "hal_types.h"
#include "I2C.h"

#include "pindefine.h"

static void I2CDelay(void)
{
    volatile uint8 i = 1;
    do 
    {
    } while (--i);
}

void InitI2C()
{
	SET_GPIO_OUTPUT(PIN_SCL);
	SET_GPIO_OUTPUT(PIN_SDA);
	  
	SET_GPIO_BIT(PIN_SCL);
	SET_GPIO_BIT(PIN_SDA);
}

void I2CStart()
{
	SET_GPIO_BIT(PIN_SDA);

	SET_GPIO_BIT(PIN_SCL);
	I2CDelay();
	CLR_GPIO_BIT(PIN_SDA);
	
	I2CDelay();
	CLR_GPIO_BIT(PIN_SCL);
}

void I2CStop()
{
	CLR_GPIO_BIT(PIN_SDA);
	I2CDelay();
	SET_GPIO_BIT(PIN_SCL);
	I2CDelay();
	SET_GPIO_BIT(PIN_SDA);
	I2CDelay();
}

void WriteI2CByte(uint8 c)
{
	uint8 i = 8;

	do 
	{
		if (c & 0x80)
		{
			SET_GPIO_BIT(PIN_SDA);
		}
		else
		{
			CLR_GPIO_BIT(PIN_SDA);
		}
		
		SET_GPIO_BIT(PIN_SCL);
		I2CDelay();
		CLR_GPIO_BIT(PIN_SCL);
		
		I2CDelay();
		
		c <<= 1;
	} while (--i);
}

bool CheckI2CAck()
{
	uint8 loop = 0xff;

	bool res;

	SET_GPIO_INPUT(PIN_SDA);

	SET_GPIO_BIT(PIN_SCL);
	
	while (GET_GPIO_BIT(PIN_SDA) && --loop);
	
	res = (!GET_GPIO_BIT(PIN_SDA));
	
	CLR_GPIO_BIT(PIN_SCL);
	I2CDelay();
	
	SET_GPIO_OUTPUT(PIN_SDA);
	return res;
}

uint8 ReceiveI2CByte()
{
	uint8 i = 8;

	uint8 c = 0;

	SET_GPIO_INPUT(PIN_SDA);

	do
	{
		c <<= 1;

		SET_GPIO_BIT(PIN_SCL);
		I2CDelay();
		if (GET_GPIO_BIT(PIN_SDA))
		{
		  	c |= 0x01;
		}
		
		CLR_GPIO_BIT(PIN_SCL);
		I2CDelay();
	} while (--i);

	SET_GPIO_OUTPUT(PIN_SDA);

	return c;
}

void SendI2CAck(bool ack)
{
	if (ack)
	{
		CLR_GPIO_BIT(PIN_SDA);
	}
	else
	{
		SET_GPIO_BIT(PIN_SDA);
	}

	SET_GPIO_BIT(PIN_SCL);
	I2CDelay();
	CLR_GPIO_BIT(PIN_SCL);
	I2CDelay();
}

