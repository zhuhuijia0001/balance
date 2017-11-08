#include "comdef.h"
#include "hal_types.h"

#include "hal_drivers.h"
#include "osal.h"
#include "hal_mcu.h"

#include "PinDefine.h"

#include "HX711.h"

#include "npi.h"

#define HX711_INT_RISING_EDGE   0
#define HX711_INT_FALLING_EDGE  1

/* CPU port interrupt */
#define HX711_INT_CPU_PORT_1_IF  P1IF

/* int is at P1.3 */
#define HX711_INT_PORT   P1
#define HX711_INT_BIT    BV(3)
#define HX711_INT_SEL    P1SEL
#define HX711_INT_DIR    P1DIR

/* edge interrupt */
#define HX711_INT_EDGEBIT  BV(1)
#define HX711_INT_EDGE     HX711_INT_FALLING_EDGE

/* interrupts */
#define HX711_INT_IEN      IEN2  /* CPU interrupt mask register */
#define HX711_INT_IENBIT   BV(4) /* Mask bit for all of Port_1 */
#define HX711_INT_ICTL     P1IEN /* Port Interrupt Control register */
#define HX711_INT_ICTLBIT  BV(3) /* P0IEN - P1.3 enable/disable bit */
#define HX711_INT_PXIFG    P1IFG /* Interrupt flag at source */

/* mode */
#define HX711_PORT_MODE    P1INP
#define HX711_PORT_MODEBIT BV(3)

//callback
static void (*s_hx711callback)(uint32) = NULL;

static bool s_repeat = false;

static void EnableDoutInterrupt(bool enable)
{
	HX711_INT_PXIFG &= ~(HX711_INT_BIT);
	
	if (enable)
	{
		HX711_INT_ICTL |= HX711_INT_ICTLBIT;
	}
	else
	{
		HX711_INT_ICTL &= ~(HX711_INT_ICTLBIT);
	}
}

void InitHx711()
{
	SET_GPIO_OUTPUT(PIN_VDD);
	SET_GPIO_BIT(PIN_VDD);
	
	SET_GPIO_OUTPUT(PIN_SLK);
	SET_GPIO_BIT(PIN_SLK);
	
	HX711_INT_SEL &= ~(HX711_INT_BIT);    /* Set pin function to GPIO */
  	HX711_INT_DIR &= ~(HX711_INT_BIT);    /* Set pin direction to Input */

	//tristate mode
	HX711_PORT_MODE |= HX711_PORT_MODEBIT;
	
  	/* Rising/Falling edge configuratinn */
    PICTL &= ~(HX711_INT_EDGEBIT);    /* Clear the edge bit */
    /* For falling edge, the bit must be set. */
  #if (HX711_INT_EDGE == HX711_INT_FALLING_EDGE)
    PICTL |= HX711_INT_EDGEBIT;
  #endif

	HX711_INT_IEN |= HX711_INT_IENBIT;
}

static uint32 GetHx711ADCValue()
{
	uint32 adc = 0;
	uint8 i;

	uint8 level;
	HAL_ENTER_CRITICAL_SECTION(level);
	
	for (i = 0; i < 24; i++)
	{
		SET_GPIO_BIT(PIN_SLK);

		adc <<= 1;
		
		CLR_GPIO_BIT(PIN_SLK);

		if (GET_GPIO_BIT(PIN_DT))
		{
			adc |= 0x01;
		}
	}

	SET_GPIO_BIT(PIN_SLK);

	adc = adc ^ 0x800000;
		
	CLR_GPIO_BIT(PIN_SLK);

	HAL_EXIT_CRITICAL_SECTION(level);
	
	return adc;
}

HAL_ISR_FUNCTION(intPort1Isr, P1INT_VECTOR)
{
	HAL_ENTER_ISR();

	if (HX711_INT_PXIFG & HX711_INT_BIT)
	{
		EnableDoutInterrupt(false);

		osal_set_event(Hal_TaskID, HX711_INT_EVENT);
	}
	
	/*
		Clear the CPU interrupt flag for Port_1
		PxIFG has to be cleared before PxIF
  	*/
  
	HX711_INT_CPU_PORT_1_IF = 0;

	CLEAR_SLEEP_MODE();

	HAL_EXIT_ISR();
}

void StartHx711(void (*callback)(uint32), bool repeat)
{
	s_hx711callback = callback;
	s_repeat = repeat;

	CLR_GPIO_BIT(PIN_SLK);
	
	EnableDoutInterrupt(true);
}

void StopHx711()
{
	s_hx711callback = NULL;
	s_repeat = false;
	
	EnableDoutInterrupt(false);

	SET_GPIO_BIT(PIN_SLK);
}

void ProcessHx711Event()
{
	uint32 adc = GetHx711ADCValue();
	//TRACE("adc:0x%08lX\r\n", adc);
	
	if (s_repeat)
	{
		EnableDoutInterrupt(true);
	}
	
	if (s_hx711callback != NULL)
	{
		s_hx711callback(adc);
	}
}

