#include "comdef.h"
#include "hal_types.h"

#include "Hx711.h"

#include "Parameter.h"
#include "Weight.h"

#include "npi.h"

typedef enum
{
	weigh_status_idle = 0,

	weigh_status_calibrating,

	weigh_status_weighing,
} WeighStatus;

static WeighStatus s_weighStatus = weigh_status_idle;

//weigh
static void (*s_weighCallback)(uint16, uint16) = NULL;
static bool s_bRepeatWeigh = false;

//calibrate
#define CALIBRATE_SAMPLE_COUNT     10

static void (*s_calibrateCallback)(uint16, uint16, uint32) = NULL;

static uint16 s_kgInt = 0;
static uint16 s_kgFrac = 0;

static uint8 s_caliSampleCount = 0;
static uint32 s_adcSum = 0;
static uint32 s_adcMax = 0;
static uint32 s_adcMin = 0;

static const struct CalibrationItem *s_caliItem = NULL;
static uint8 s_caliItemCount = 0;

static uint32 s_tareAdc = 0;

void InitWeigh()
{
	InitHx711();
}

void SetTare(uint32 tareAdc)
{
	s_tareAdc = tareAdc;
}

void SetCalibration(const struct CalibrationItem *caliItem, uint8 caliItemCount)
{
	s_caliItem = caliItem;
	s_caliItemCount = caliItemCount;
}

static void AdcCallback(uint32 adc)
{
	TRACE("adc:0x%08lX\r\n", adc);
	
	switch (s_weighStatus)
	{
	case weigh_status_calibrating:
		s_adcSum += adc;
		if (s_adcMax < adc)
		{
			s_adcMax = adc;
		}

		if (s_adcMin > adc)
		{
			s_adcMin = adc;
		}
		
		s_caliSampleCount--;
		if (s_caliSampleCount == 0)
		{
			StopHx711();
			
			s_adcSum -= s_adcMax + s_adcMin;

			s_adcSum /= (CALIBRATE_SAMPLE_COUNT - 2);

			if (s_calibrateCallback != NULL)
			{
				s_calibrateCallback(s_kgInt, s_kgFrac, s_adcSum);

				s_calibrateCallback = NULL;

				s_weighStatus = weigh_status_idle;
			}
		}
		
		break;

	case weigh_status_weighing:
		{
			if (s_caliItemCount == 2)
			{
				const struct CalibrationItem *caliItemMin, *caliItemMax;
				if (s_caliItem[1].adc >= s_caliItem[0].adc)
				{
					caliItemMin = &s_caliItem[0];
					caliItemMax = &s_caliItem[1];
				}
				else
				{
					caliItemMin = &s_caliItem[1];
					caliItemMax = &s_caliItem[0];
				}
				
				uint32 zeroAdc;
				if (s_tareAdc == 0)
				{
					zeroAdc = caliItemMin->adc;
				}
				else
				{
					zeroAdc = s_tareAdc;
				}

				uint16 integer;
				uint16 frac;
				
				if (adc > zeroAdc)
				{
					uint32 deltaAdc = adc - zeroAdc;
					
					uint32 deltaCaliAdc = caliItemMax->adc - caliItemMin->adc;
					
					uint32 tempInt = (caliItemMax->kgInt - caliItemMin->kgInt) * deltaAdc;
					uint32 tempFrac = (caliItemMax->kgFrac - caliItemMin->kgFrac) * deltaAdc;
					
					TRACE("tempInt1:0x%08lX,tempFrac1:0x%08lX\r\n", tempInt, tempFrac);
					
					tempFrac += (tempInt % deltaCaliAdc) * 10000;
					tempFrac /= deltaCaliAdc;
					
					tempInt /= deltaCaliAdc;
					tempInt += tempFrac / 10000;
					
					tempFrac %= 10000;
					
					integer = tempInt;
					frac = tempFrac;
				}
				else
				{
					integer = 0;
					frac = 0;
				}
				
				if (s_weighCallback != NULL)
				{
					s_weighCallback(integer, frac);
				}
			}

			if (!s_bRepeatWeigh)
			{
				StopHx711();

				s_weighCallback = NULL;

				s_weighStatus = weigh_status_idle;
			}
		}
		
		break;
	}
}

void StartWeigh(void (*callback)(uint16 integer, uint16 frac), bool repeat)
{
	s_weighCallback = callback;
	s_bRepeatWeigh = repeat;
	
	StartHx711(AdcCallback, repeat);

	s_weighStatus = weigh_status_weighing;
}

void StartCalibrate(void (*callback)(uint16 integer, uint16 frac, uint32 adc), uint16 integer, uint16 frac)
{
	s_calibrateCallback = callback;
	s_kgInt = integer;
	s_kgFrac = frac;

	s_caliSampleCount = CALIBRATE_SAMPLE_COUNT;

	s_adcSum = 0;
	s_adcMax = 0;
	s_adcMin = 0xffffffff;
	
	StartHx711(AdcCallback, true);

	s_weighStatus = weigh_status_calibrating;
}

void StopWeigh()
{
	s_weighCallback = NULL;
	
	StopHx711();

	s_bRepeatWeigh = false;
	
	s_weighStatus = weigh_status_idle;
}

