/**************************************************************************************************
  Filename:       Balance.c
  Revised:        $Date: 2010-08-06 08:56:11 -0700 (Fri, 06 Aug 2010) $
  Revision:       $Revision: 23333 $

  Description:    This file contains the Balance BLE Peripheral sample application
                  for use with the CC2540 Bluetooth Low Energy Protocol Stack.

  Copyright 2010 - 2013 Texas Instruments Incorporated. All rights reserved.

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

/*********************************************************************
 * INCLUDES
 */

#include <stdio.h>

#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"
#include "osal_cbtimer.h"

#include "OnBoard.h"
#include "hal_key.h"
#include "hal_led.h"

#include "gatt.h"

#include "hci.h"

#include "gapgattserver.h"
#include "gattservapp.h"
#include "gap.h"

#if defined ( PLUS_BROADCASTER )
  #include "peripheralBroadcaster.h"
#else
  #include "peripheral.h"
#endif

#if defined FEATURE_OAD    
  #include "oad.h"    
  #include "oad_target.h"    
#endif

#include "balanceGATTprofile.h"

#include "Parameter.h"

#include "Weight.h"

#include "Battery.h"

#include "Balance.h"

#include "BleComm.h"

#include "Npi.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// What is the advertising interval when device is discoverable (units of 625us, 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL          160//500ms

// Limited discoverable mode advertises for 30.72s, and then stops
// General discoverable mode advertises indefinitely

#define DEFAULT_DISCOVERABLE_MODE             GAP_ADTYPE_FLAGS_GENERAL

// Minimum connection interval (units of 1.25ms, 80=100ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL     24

// Maximum connection interval (units of 1.25ms, 800=1000ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL     40

// Slave latency to use if automatic parameter update request is enabled
#define DEFAULT_DESIRED_SLAVE_LATENCY         0

// Supervision timeout value (units of 10ms, 1000=10s) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_CONN_TIMEOUT          600

// Whether to enable automatic parameter update request when a connection is formed
#define DEFAULT_ENABLE_UPDATE_REQUEST         TRUE

// Connection Pause Peripheral time value (in seconds)
#define DEFAULT_CONN_PAUSE_PERIPHERAL         6

// Company Identifier: Texas Instruments Inc. (13)
#define TI_COMPANY_ID                         0x000D

#define INVALID_CONNHANDLE                    0xFFFF

// Length of bd addr as a string
#define B_ADDR_STR_LEN                        15

#if defined ( PLUS_BROADCASTER )
  #define ADV_IN_CONN_WAIT                    500 // delay 500 ms
#endif

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8 balance_TaskID;   // Task ID for internal task/event processing

static gaprole_States_t gapProfileState = GAPROLE_INIT;

#define BLE_NAME               "Balance"

#define  MAX_BLE_NAME_LEN      20

// GAP - SCAN RSP data (max size = 31 bytes)
static uint8 scanRspData[] =
{
	// complete name
	MAX_BLE_NAME_LEN,   // length of this data
	GAP_ADTYPE_LOCAL_NAME_COMPLETE,

	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',
	'\0',

	// connection interval range
	0x05,   // length of this data
	GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
	LO_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL),
	HI_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL),
	LO_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),
	HI_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),

	// Tx power level
	0x02,   // length of this data
	GAP_ADTYPE_POWER_LEVEL,
	0       // 0dBm
};

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
static uint8 advertData[] =
{
	// Flags; this sets the device to use limited discoverable
	// mode (advertises for 30 seconds at a time) instead of general
	// discoverable mode (advertises indefinitely)
	0x02,   // length of this data
	GAP_ADTYPE_FLAGS,
	DEFAULT_DISCOVERABLE_MODE | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

	// service UUID, to notify central devices what services are included
	// in this peripheral
	0x03,   // length of this data
	GAP_ADTYPE_16BIT_MORE,      // some of the UUID's, but not all
	LO_UINT16( 0xfff0 ),
	HI_UINT16( 0xfff0 ),
};

// GAP GATT Attributes
static uint8 attDeviceName[GAP_DEVICE_NAME_LEN];

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void Balance_ProcessOSALMsg(osal_event_hdr_t *pMsg);
static void peripheralStateNotificationCB(gaprole_States_t newState);

static void BalanceProfileChangeCB(uint8 paramID, uint8 len);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t balance_PeripheralCBs =
{
	peripheralStateNotificationCB,  // Profile State Change Callbacks
	NULL                            // When a valid RSSI is read from controller (not used by application)
};

// Balance GATT Profile Callbacks
static balanceProfileCBs_t balanceProfileCBs =
{
	BalanceProfileChangeCB    // Charactersitic value change callback
};


/*********************************************************************
 * PUBLIC FUNCTIONS
 */

void SetBleName(const uint8 *name)
{
	int i;
	for (i = 0; i < MAX_BLE_NAME_LEN; i++)
	{
		if (name[i] == '\0')
		{
			break;
		}
		
		scanRspData[2 + i] = name[i];

		attDeviceName[i] = name[i];
	}
}

/*********************************************************************
 * @fn      Balance_Init
 *
 * @brief   Initialization function for the Balance BLE Peripheral App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */

void Balance_Init( uint8 task_id )
{
	HalLedSet(LED_POWER, HAL_LED_MODE_OFF);

	balance_TaskID = task_id;

  	RegisterForKeys(balance_TaskID);

	if (!LoadParameter())
	{
		LoadDefaultParameter();

		SaveParameter();
	}
	
	SetBleName(BLE_NAME);

	SetCalibration(g_caliItem, g_caliItemCount);
	
  	hciStatus_t hci_status = HCI_EXT_SetTxPowerCmd (HCI_EXT_TX_POWER_4_DBM);  

  	HCI_EXT_HaltDuringRfCmd(HCI_EXT_HALT_DURING_RF_DISABLE);
  
  	// Setup the GAP
  	VOID GAP_SetParamValue(TGAP_CONN_PAUSE_PERIPHERAL, DEFAULT_CONN_PAUSE_PERIPHERAL);
  
  	// Setup the GAP Peripheral Role Profile
  	{
      	// For other hardware platforms, device starts advertising upon initialization
      	uint8 initial_advertising_enable = TRUE;

    	// By setting this to zero, the device will go into the waiting state after
    	// being discoverable for 30.72 second, and will not being advertising again
    	// until the enabler is set back to TRUE
    	uint16 gapRole_AdvertOffTime = 0;

    	uint8 enable_update_request = DEFAULT_ENABLE_UPDATE_REQUEST;
    	uint16 desired_min_interval = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
		uint16 desired_max_interval = DEFAULT_DESIRED_MAX_CONN_INTERVAL;
    	uint16 desired_slave_latency = DEFAULT_DESIRED_SLAVE_LATENCY;
    	uint16 desired_conn_timeout = DEFAULT_DESIRED_CONN_TIMEOUT;

    	// Set the GAP Role Parameters
    	GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &initial_advertising_enable );
    	GAPRole_SetParameter( GAPROLE_ADVERT_OFF_TIME, sizeof( uint16 ), &gapRole_AdvertOffTime );

    	GAPRole_SetParameter( GAPROLE_SCAN_RSP_DATA, sizeof ( scanRspData ), scanRspData );
    	GAPRole_SetParameter( GAPROLE_ADVERT_DATA, sizeof( advertData ), advertData );

    	GAPRole_SetParameter( GAPROLE_PARAM_UPDATE_ENABLE, sizeof( uint8 ), &enable_update_request );
    	GAPRole_SetParameter( GAPROLE_MIN_CONN_INTERVAL, sizeof( uint16 ), &desired_min_interval );
    	GAPRole_SetParameter( GAPROLE_MAX_CONN_INTERVAL, sizeof( uint16 ), &desired_max_interval );
    	GAPRole_SetParameter( GAPROLE_SLAVE_LATENCY, sizeof( uint16 ), &desired_slave_latency );
    	GAPRole_SetParameter( GAPROLE_TIMEOUT_MULTIPLIER, sizeof( uint16 ), &desired_conn_timeout );
  	}

  	// Set the GAP Characteristics
  	GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName);

  	// Set advertising interval
  	{
    	uint16 advInt = DEFAULT_ADVERTISING_INTERVAL;

    	GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MIN, advInt);
    	GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MAX, advInt);
    	GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MIN, advInt);
    	GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MAX, advInt);
  	}

  	// Initialize GATT attributes
  	GGS_AddService(GATT_ALL_SERVICES);            // GAP
  	GATTServApp_AddService(GATT_ALL_SERVICES);    // GATT attributes

#if defined FEATURE_OAD    
	  VOID OADTarget_AddService();					  // OAD Profile	
#endif

	BalanceProfile_AddService(GATT_ALL_SERVICES);  // balance GATT Profile

	BalanceProfile_RegisterAppCBs(&balanceProfileCBs);
	
  	// Enable clock divide on halt
  	// This reduces active current while radio is active and CC254x MCU
  	// is halted
  	HCI_EXT_ClkDivOnHaltCmd(HCI_EXT_DISABLE_CLK_DIVIDE_ON_HALT);

#if defined ( DC_DC_P0_7 )

  	// Enable stack to toggle bypass control on TPS62730 (DC/DC converter)
  	HCI_EXT_MapPmIoPortCmd(HCI_EXT_PM_IO_PORT_P0, HCI_EXT_PM_IO_PORT_PIN7);

#endif // defined ( DC_DC_P0_7 )

  	// Setup a delayed profile startup
  	osal_set_event(balance_TaskID, BALANCE_START_DEVICE_EVT);
}

/*
static void MyWeighCallback(uint16 integer, uint16 frac)
{
	TRACE("%d.%04dkg\r\n", integer, frac);
}
*/

/*********************************************************************
 * @fn      Balance_ProcessEvent
 *
 * @brief   PowerAsist Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16 Balance_ProcessEvent(uint8 task_id, uint16 events)
{
	VOID task_id; // OSAL required parameter that isn't used in this function

	if (events & SYS_EVENT_MSG)
	{
		uint8 *pMsg;

		if ((pMsg = osal_msg_receive(balance_TaskID)) != NULL)
		{
			Balance_ProcessOSALMsg((osal_event_hdr_t *)pMsg);

			// Release the OSAL message
			osal_msg_deallocate(pMsg);
		}

		// return unprocessed events
		return events ^ SYS_EVENT_MSG;
	}

	//onInit
	if (events & BALANCE_START_DEVICE_EVT)
	{
		// Start the Device
		VOID GAPRole_StartDevice(&balance_PeripheralCBs);

		//StartWeigh(NULL, true);
		
		return events ^ BALANCE_START_DEVICE_EVT;
	}

	if (events & BALANCE_PERIOD_EVT)
	{
		uint8 voltage = GetBatteryVoltage();
		TRACE("bat voltage:%d.%dV\r\n", voltage / 10, voltage % 10);
		
		return events ^ BALANCE_PERIOD_EVT;
	}

	if (events & BALANCE_PARSE_PACKET_TIMEOUT_EVT)
	{
		ProcessBleParsePacketTimeout();
		
		return events ^ BALANCE_PARSE_PACKET_TIMEOUT_EVT;
	}
	
#if defined ( PLUS_BROADCASTER )
	if (events & POWERASIST_ADV_IN_CONNECTION_EVT)
	{
		uint8 turnOnAdv = TRUE;
		// Turn on advertising while in a connection
		GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &turnOnAdv);

		return (events ^ POWERASIST_ADV_IN_CONNECTION_EVT);
	}
#endif // PLUS_BROADCASTER

	// Discard unknown events
	return 0;
}

/*********************************************************************
 * @fn      Balance_ProcessOSALMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void Balance_ProcessOSALMsg( osal_event_hdr_t *pMsg )
{
	switch (pMsg->event)
	{
    case KEY_CHANGE:
    	{
    		//keyChange_t *key = (keyChange_t *)pMsg;

    		//TRACE("key:0x%02X,type:0x%02X\r\n", key->keys, key->state);
    	}
    	
		break;

	default:
		// do nothing
		break;
	}
}

/*********************************************************************
 * @fn      peripheralStateNotificationCB
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void peripheralStateNotificationCB(gaprole_States_t newState)
{
	switch ( newState )
	{
	case GAPROLE_STARTED:
		{
			TRACE("started..\r\n");
		}
		break;

	case GAPROLE_ADVERTISING:  //ÕýÔÚ¹ã²¥
		{
			TRACE("advertising\r\n");
		}
		break;

	case GAPROLE_CONNECTED:  //cnnOK
		{
			TRACE("connected\r\n");

			//StartWeigh(MyWeighCallback, true);
		}
		break;

	case GAPROLE_WAITING:
		{
			TRACE("disconnected\r\n");

			ProcessBleDisconnected();
		}

		break;

	case GAPROLE_WAITING_AFTER_TIMEOUT:
		{
			TRACE("timeout.\r\n");

			ProcessBleDisconnected();
		}

		break;

	case GAPROLE_ERROR:
		{
			TRACE("err.\r\n");

			ProcessBleDisconnected();
		}

		break;

	default:
		{
		}

		break;
	}

	gapProfileState = newState;

	VOID gapProfileState;     // added to prevent compiler warning with
	            				// "CC2540 Slave" configurations
}

uint8 Balance_StartTimer(uint16 eventId, uint32 timeout, bool repeat)
{
	if (repeat)
	{
		return osal_start_reload_timer(balance_TaskID, eventId, timeout);
	}
	else
	{
		return osal_start_timerEx(balance_TaskID, eventId, timeout);
	}
}

uint8 Balance_StopTimer(uint16 eventId)
{
	return osal_stop_timerEx(balance_TaskID, eventId);
}

static void BalanceProfileChangeCB(uint8 paramID, uint8 len)
{
	uint8 buf[20];

	switch (paramID)
	{
    case BALANCEPROFILE_TRX:
    	{
			bStatus_t status = BalanceProfile_GetParameter(BALANCEPROFILE_TRX, buf, len);

			if (status == SUCCESS)
			{
				ProcessBleCom(buf, len);
			}
		}
		
      	break;

    default:
      	/* should not reach here! */

		break;
	}
}

