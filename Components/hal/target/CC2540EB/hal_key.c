/**************************************************************************************************
  Filename:       hal_key.c
  Revised:        $Date: 2012-12-06 14:00:35 -0800 (Thu, 06 Dec 2012) $
  Revision:       $Revision: 32474 $

  Description:    This file contains the interface to the HAL KEY Service.


  Copyright 2006-2012 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/**************************************************************************************************
 *                                            INCLUDES
 **************************************************************************************************/
#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"
#include "hal_drivers.h"
#include "hal_key.h"
#include "osal.h"

#if (defined HAL_KEY) && (HAL_KEY == TRUE)

/**************************************************************************************************
 *                                            CONSTANTS
 **************************************************************************************************/
#define HAL_KEY_RISING_EDGE   0
#define HAL_KEY_FALLING_EDGE  1

/* debounce time */
#define HAL_KEY_DEBOUNCE_TIME   30

/* check time */
#define HAL_KEY_CHECK_PERIOD    100

#define KEY_LONG_COUNT      10
#define KEY_CONTINUE_COUNT  1

/* CPU port interrupt */
#define HAL_KEY_CPU_PORT_0_IF P0IF

/* key 1 is at P0.1 */
#define HAL_KEY_1_PORT   P0
#define HAL_KEY_1_BIT    BV(1)
#define HAL_KEY_1_SEL    P0SEL
#define HAL_KEY_1_DIR    P0DIR

/* edge interrupt */
#define HAL_KEY_1_EDGEBIT  BV(0)
#define HAL_KEY_1_EDGE     HAL_KEY_FALLING_EDGE

/* key 1 interrupts */
#define HAL_KEY_1_IEN      IEN1  /* CPU interrupt mask register */
#define HAL_KEY_1_IENBIT   BV(5) /* Mask bit for all of Port_0 */
#define HAL_KEY_1_ICTL     P0IEN /* Port Interrupt Control register */
#define HAL_KEY_1_ICTLBIT  BV(1) /* P0IEN - P0.1 enable/disable bit */
#define HAL_KEY_1_PXIFG    P0IFG /* Interrupt flag at source */


//key status 
typedef enum
{
	KEY_STATUS_IDLE = 0,
	KEY_STATUS_DEBOUNCE,
	KEY_STATUS_PRESSED,
	KEY_STATUS_CONTINUE
} KeyStatus;

/* function to detect key */
typedef bool (*KeyFun)();

/* function to enable/disable key interrupt */
typedef void (*KeyInterruptFun)(bool);

/* key 1 function */
static bool CheckKey1() 
{
	return !(HAL_KEY_1_PORT & HAL_KEY_1_BIT);
}

static void EnableKey1Interrupt(bool enable)
{
	if (enable)
	{
		HAL_KEY_1_ICTL |= HAL_KEY_1_ICTLBIT;
	}
	else
	{
		HAL_KEY_1_ICTL &= ~(HAL_KEY_1_ICTLBIT);
	}

	HAL_KEY_1_PXIFG = ~(HAL_KEY_1_BIT);
}

typedef struct 
{
	uint8 key;
	
	uint8 keyStatus;

	KeyFun keyFun;
	
	KeyInterruptFun keyInterruptFun;
	
	uint8 count;
} KeyStruct;

static KeyStruct keyStruct[] = 
{
	[KEY_1] = 
	{	
		.key = KEY_1,
		
		.keyStatus = KEY_STATUS_IDLE,
		
		.keyFun = CheckKey1,
		
		.keyInterruptFun = EnableKey1Interrupt,
		
		.count = 0
	},
};

/**************************************************************************************************
 *                                            TYPEDEFS
 **************************************************************************************************/


/**************************************************************************************************
 *                                        GLOBAL VARIABLES
 **************************************************************************************************/
static halKeyCBack_t pHalKeyProcessFunction;

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/

/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/


/**************************************************************************************************
 * @fn      HalKeyInit
 *
 * @brief   Initilize Key Service
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void HalKeyInit( void )
{
	/* config key */
  	HAL_KEY_1_SEL &= ~(HAL_KEY_1_BIT);    /* Set pin function to GPIO */
  	HAL_KEY_1_DIR &= ~(HAL_KEY_1_BIT);    /* Set pin direction to Input */

  	/* Rising/Falling edge configuratinn */
    PICTL &= ~(HAL_KEY_1_EDGEBIT);    /* Clear the edge bit */
    /* For falling edge, the bit must be set. */
  #if (HAL_KEY_1_EDGE == HAL_KEY_FALLING_EDGE)
    PICTL |= HAL_KEY_1_EDGEBIT;
  #endif

	uint8 i;
	for (i = 0; i < sizeof(keyStruct) / sizeof(keyStruct[0]); i++)
	{
		keyStruct[i].keyInterruptFun(true);
	}
	
	HAL_KEY_1_IEN |= HAL_KEY_1_IENBIT;

  	/* Initialize callback function */
  	pHalKeyProcessFunction  = NULL;
}


/**************************************************************************************************
 * @fn      HalKeyConfig
 *
 * @brief   Configure the Key serivce
 *
 * @param   cback - pointer to the CallBack function
 *
 * @return  None
 **************************************************************************************************/
void HalKeyConfig (halKeyCBack_t cback)
{
	/* Register the callback fucntion */
	pHalKeyProcessFunction = cback;
}


/**************************************************************************************************
 * @fn      HalKeyRead
 *
 * @brief   Read the current value of a key
 *
 * @param   None
 *
 * @return  keys - current keys status
 **************************************************************************************************/
uint8 HalKeyRead ( void )
{
  uint8 keys = 0;

  if ( (HAL_KEY_1_PORT & HAL_KEY_1_BIT))    /* Key is active high */
  {
    keys = KEY_1;
  }

  return keys;
}


/**************************************************************************************************
 * @fn      HalKeyPoll
 *
 * @brief   Called by hal_driver to poll the keys
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
void HalKeyPoll(uint8 index)
{
  	uint8 key = KEY_NONE;
  	uint8 status = 0;

  	uint32 period = 0;

	if (keyStruct[index].keyFun())
	{
		switch (keyStruct[index].keyStatus)
		{
		case KEY_STATUS_DEBOUNCE:
			key = keyStruct[index].key;
			status = HAL_KEY_STATE_PRESS;

			keyStruct[index].count = KEY_LONG_COUNT;
			keyStruct[index].keyStatus = KEY_STATUS_PRESSED;

			period = HAL_KEY_CHECK_PERIOD;
			
			break;

		case KEY_STATUS_PRESSED:
			if (--keyStruct[index].count <= 0)
			{
				key = keyStruct[index].key;
				status = HAL_KEY_STATE_LONG;
				keyStruct[index].count = KEY_CONTINUE_COUNT;
				keyStruct[index].keyStatus = KEY_STATUS_CONTINUE;
			}

			period = HAL_KEY_CHECK_PERIOD;
			
			break;
			
		case KEY_STATUS_CONTINUE:
			if (--keyStruct[index].count <= 0)
			{
				key = keyStruct[index].key;
				status = HAL_KEY_STATE_CONTINUE;
				keyStruct[index].count = KEY_CONTINUE_COUNT;
			}

			period = HAL_KEY_CHECK_PERIOD;
			
			break;
		}
	}
	else
	{
		switch (keyStruct[index].keyStatus)
		{
		case KEY_STATUS_PRESSED:
		case KEY_STATUS_CONTINUE:
			key = keyStruct[index].key;
			status = HAL_KEY_STATE_RELEASE;
		    
			break;
		}
		
		keyStruct[index].keyInterruptFun(true);
	}
	
	/* Invoke Callback if new keys were depressed */
	if (key != KEY_NONE && (pHalKeyProcessFunction))
	{
		pHalKeyProcessFunction(key, status);
	}

	if (period > 0)
	{
		osal_start_timerEx (Hal_TaskID, HAL_KEY_EVENT << index, period);
	}
}

/**************************************************************************************************
 * @fn      halProcessKeyInterrupt
 *
 * @brief   Checks to see if it's a valid key interrupt, saves interrupt driven key states for
 *          processing by HalKeyRead(), and debounces keys by scheduling HalKeyRead() 25ms later.
 *
 * @param
 *
 * @return
 **************************************************************************************************/
static void halProcessKeyInterrupt (void)
{	
	if (HAL_KEY_1_PXIFG & HAL_KEY_1_BIT)  /* Interrupt Flag has been set */
	{
		keyStruct[KEY_1].keyInterruptFun(false);
		
		keyStruct[KEY_1].keyStatus = KEY_STATUS_DEBOUNCE;

		//debounce
		osal_start_timerEx (Hal_TaskID, HAL_KEY_EVENT << KEY_1, HAL_KEY_DEBOUNCE_TIME);
	}
}

/**************************************************************************************************
 * @fn      HalKeyEnterSleep
 *
 * @brief  - Get called to enter sleep mode
 *
 * @param
 *
 * @return
 **************************************************************************************************/
void HalKeyEnterSleep ( void )
{
}

/**************************************************************************************************
 * @fn      HalKeyExitSleep
 *
 * @brief   - Get called when sleep is over
 *
 * @param
 *
 * @return  - return saved keys
 **************************************************************************************************/
uint8 HalKeyExitSleep ( void )
{
	/* Wake up and read keys */
	return ( HalKeyRead () );
}

/***************************************************************************************************
 *                                    INTERRUPT SERVICE ROUTINE
 ***************************************************************************************************/

/**************************************************************************************************
 * @fn      halKeyPort0Isr
 *
 * @brief   Port0 ISR
 *
 * @param
 *
 * @return
 **************************************************************************************************/
HAL_ISR_FUNCTION( halKeyPort0Isr, P0INT_VECTOR )
{
	HAL_ENTER_ISR();
    
	halProcessKeyInterrupt();

	/*
		Clear the CPU interrupt flag for Port_0
		PxIFG has to be cleared before PxIF
  	*/
  
	HAL_KEY_CPU_PORT_0_IF = 0;

	CLEAR_SLEEP_MODE();

	HAL_EXIT_ISR();
}

#endif
/**************************************************************************************************
**************************************************************************************************/
