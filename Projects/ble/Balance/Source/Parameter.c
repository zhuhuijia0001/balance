#include "hal_types.h"
#include "comdef.h"

#include "EEPROM.h"
#include "Parameter.h"

#include "npi.h"

#define  HEADER     0xA5

#define  PARAMETER_START_ADDR      0

StoreVector g_storeVector;

//for test, will not be used
#define ADC_0G        0x7F7500

#define ADC_50G       0x80EB70

static const Parameter s_defaultParameter = 
{
	.caliItem = 
	{
		[0] = 
		{
			.kgInt = 0,
			.kgFrac = 0,
			
			.adc = ADC_0G
		},

		[1] = 
		{
			.kgInt = 0,
			.kgFrac = 500,
			
			.adc = ADC_50G
		},
	},

	.caliItemCount = 0,

	.tare = 0,
};

bool LoadParameter()
{
	uint16 addr = PARAMETER_START_ADDR;

	uint8 dat;	
	//header
	if (!ReadEEPROM(addr, &dat))
	{		
		return false;
	}

	if (dat != HEADER)
	{	
		return false;
	}

	//size
	uint16 size;
	addr += sizeof(dat);
	if (!ReadEEPROMData(addr, (uint8 *)&size, sizeof(size), NULL))
	{		
		return false;
	}

	if (size != sizeof(Parameter))
	{		
		return false;
	}

	//parameter
	addr += sizeof(size);
	if (!ReadEEPROMData(addr, (uint8 *)&g_storeVector.parameter, size, NULL))
	{
		return false;
	}

	//sum
	uint8 sum = 0x00;
	uint8 *p = (uint8 *)&g_storeVector.parameter;
	int i;
	for (i = 0; i < size; i++)
	{
		sum ^= *p++;
	}
	
	addr += size;
	if (!ReadEEPROM(addr, &dat))
	{		
		return false;
	}
	
	if (sum != dat)
	{		
		return false;
	}

	TRACE("load parameter OK\r\n");
	
	return true;
}

void LoadDefaultParameter()
{
	g_storeVector.parameter = s_defaultParameter;
}

bool SaveParameter()
{
  	uint16 addr = PARAMETER_START_ADDR;

	uint8 dat;

	//header
	dat = HEADER;
	if (!WriteEEPROM(addr, dat))
	{
		return false;
	}

	//size
	addr += sizeof(dat);
	uint16 size = sizeof(g_storeVector.parameter);
	if (!WriteEEPROMData(addr, (uint8 *)&size, sizeof(size)))
	{
		return false;
	}

	//parameter
	addr += sizeof(size);
	if (!WriteEEPROMData(addr, (uint8 *)&g_storeVector.parameter, sizeof(g_storeVector.parameter)))
	{
		return false;
	}
	
	uint8 *p = (uint8 *)&g_storeVector.parameter;
	int i;
	uint8 sum = 0x00;
	for (i = 0; i < size; i++)
	{	
		sum ^= *p++;
	}

	addr += size;
	if (!WriteEEPROM(addr, sum))
	{
		return false;
	}
	
	TRACE("save parameter OK\r\n");
	
	return true;
}

