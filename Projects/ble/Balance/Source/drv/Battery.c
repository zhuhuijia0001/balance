#include "comdef.h"
#include "hal_types.h"

#include "hal_adc.h"

#include "Battery.h"

#include "npi.h"

//125mV
#define REF_VOLTAGE      125ul
void InitBatteryDetect()
{
	HalAdcInit();
}

uint8 GetBatteryVoltage()
{
	uint16 adc = HalAdcRead(HAL_ADC_CHN_AIN4, HAL_ADC_RESOLUTION_12);

	//uint16 res = adc * REF_VOLTAGE / 2048 * (1220 / 220);
	//r = 1M & 220k
	uint16 res = adc * 61 * REF_VOLTAGE / 2048 / 11 / 10;

	//TRACE("bat voltage:%d.%dV\r\n", res / 10, res % 10);
	
	return res;
}

