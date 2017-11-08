/**************************************************************************************************
  Filename:       balanceGATTprofile.c
  Revised:        $Date: 2013-05-06 13:33:47 -0700 (Mon, 06 May 2013) $
  Revision:       $Revision: 34153 $

  Description:    This file contains the Balance GATT profile sample GATT service 
                  profile for use with the BLE sample application.

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
#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "gapbondmgr.h"

#include "npi.h"
#include "balanceGATTprofile.h"

/*********************************************************************
 * MACROS
 */
#define   BALANCEPROFILE_CHAR_TRX_VAL_INDEX      2

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// Balance Profile Service UUID: 0xFFF0
static CONST uint8 balanceProfileServUUID[ATT_BT_UUID_SIZE] =
{ 
	LO_UINT16(BALANCEPROFILE_SERV_UUID), HI_UINT16(BALANCEPROFILE_SERV_UUID)
};

// Characteristic trx UUID: 0xFFF1
static CONST uint8 balanceTRxUUID[ATT_BT_UUID_SIZE] =
{ 
	LO_UINT16(BALANCE_TRX_UUID), HI_UINT16(BALANCE_TRX_UUID)
};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static balanceProfileCBs_t *balanceProfile_AppCBs = NULL;

/*********************************************************************
 * Profile Attributes - variables
 */

// Balance Profile Service attribute
static CONST gattAttrType_t balanceProfileService = { ATT_BT_UUID_SIZE, balanceProfileServUUID };

// Balance Profile Characteristic TRx Properties
static uint8 balanceProfileTRxProps = GATT_PROP_WRITE_NO_RSP | GATT_PROP_NOTIFY;
// Characteristic TRx Value
static uint8 balanceProfileTRx[20] = { 0 };

// Characteristic TRx Config
static gattCharCfg_t balanceProfileTRxConfig[GATT_MAX_NUM_CONN];

// Characteristic TRx User Description
static uint8 balanceProfileTRxUserDesp[] = "Tx&Rx";
/*********************************************************************
 * Profile Attributes - Table
 */
static gattAttribute_t balanceProfileAttrTbl[] = 
{
	// Balance Profile Service
	{ 
		{ ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
		GATT_PERMIT_READ,						  /* permissions */
		0,										  /* handle */
		(uint8 *)&balanceProfileService		      /* pValue */
	},

	// Characteristic TRx Declaration
	{ 
		{ ATT_BT_UUID_SIZE, characterUUID },
		GATT_PERMIT_READ, 
		0,
		&balanceProfileTRxProps 
	},

	// Characteristic TRx Value
	{ 
		{ ATT_BT_UUID_SIZE, balanceTRxUUID },
		GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
		0, 
		balanceProfileTRx
	},

	// Characteristic TRx configuration
	{ 
		{ ATT_BT_UUID_SIZE, clientCharCfgUUID },
		GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
		0, 
		(uint8 *)balanceProfileTRxConfig 
	},
	  
	// Characteristic TRx User Description
	{ 
		{ ATT_BT_UUID_SIZE, charUserDescUUID },
		GATT_PERMIT_READ, 
		0, 
		balanceProfileTRxUserDesp 
	},
};


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t balanceProfile_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );

static void balanceProfile_HandleConnStatusCB( uint16 connHandle, uint8 changeType );


/*********************************************************************
 * PROFILE CALLBACKS
 */
// Balance Profile Service Callbacks
CONST gattServiceCBs_t balanceProfileCBs =
{
	NULL,  // Read callback function pointer
	balanceProfile_WriteAttrCB, // Write callback function pointer
	NULL                       // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      BalanceProfile_AddService
 *
 * @brief   Initializes the Balance Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t BalanceProfile_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, balanceProfileTRxConfig );
	
  // Register with Link DB to receive link status change callback
  VOID linkDB_Register(balanceProfile_HandleConnStatusCB );  
  
  if (services & BALANCEPROFILE_SERVICE)
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService(balanceProfileAttrTbl, 
                                          GATT_NUM_ATTRS( balanceProfileAttrTbl ),
                                          &balanceProfileCBs );
  }

  return ( status );
}


/*********************************************************************
 * @fn      BalanceProfile_RegisterAppCBs
 *
 * @brief   Registers the application callback function. Only call 
 *          this function once.
 *
 * @param   callbacks - pointer to application callbacks.
 *
 * @return  SUCCESS or bleAlreadyInRequestedMode
 */
bStatus_t BalanceProfile_RegisterAppCBs(balanceProfileCBs_t *appCallbacks )
{
  if ( appCallbacks )
  {
    balanceProfile_AppCBs = appCallbacks;
    
    return ( SUCCESS );
  }
  else
  {
    return ( bleAlreadyInRequestedMode );
  }
}
 
/*********************************************************************
 * @fn      BalanceProfile_GetParameter
 *
 * @brief   Get a Balance Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to put.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t BalanceProfile_GetParameter( uint8 param, void *value, uint8 len)
{
	bStatus_t ret = SUCCESS;
	switch ( param )
	{
    case BALANCEPROFILE_TRX:
		osal_memcpy(value, balanceProfileTRx, len);
      
		break;
	  
    default:
		ret = INVALIDPARAMETER;
		break;
	}
  
	return ( ret );

}

/*********************************************************************
 * @fn      balanceProfile_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 *
 * @return  Success or Failure
 */
static bStatus_t balanceProfile_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset )
{
	bStatus_t status = SUCCESS;
	uint8 notifyApp = 0xFF;
  
	// If attribute permissions require authorization to write, return error
	if ( gattPermitAuthorWrite( pAttr->permissions ) )
	{
		// Insufficient authorization
		return ( ATT_ERR_INSUFFICIENT_AUTHOR );
	}
  
	if ( pAttr->type.len == ATT_BT_UUID_SIZE )
	{
    	// 16-bit UUID
    	uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
	    if (uuid == BALANCE_TRX_UUID)
	    {
			if (status == SUCCESS)
			{
				uint8 *pCurValue = (uint8 *)pAttr->pValue;
				osal_memcpy(pCurValue, pValue, len);

				if (pAttr->pValue == balanceProfileTRx)
				{
					notifyApp = BALANCEPROFILE_TRX;
				}
			}
	    }
	    else if (uuid == GATT_CLIENT_CHAR_CFG_UUID)
	    {
			status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,    
			                                             offset, GATT_CLIENT_CFG_NOTIFY ); 
	    }
	    else
	    {
			status = ATT_ERR_ATTR_NOT_FOUND;
	    }
	}
	else
	{
		// 128-bit UUID
    	status = ATT_ERR_INVALID_HANDLE;
	}

    // If a charactersitic value changed then callback function to notify application of change
	if ( (notifyApp != 0xFF ) && balanceProfile_AppCBs && balanceProfile_AppCBs->pfnBalanceProfileChange )
	{
		balanceProfile_AppCBs->pfnBalanceProfileChange(notifyApp, len);  
	}
  
	return ( status );

}

/*********************************************************************
 * @fn          balanceProfile_HandleConnStatusCB
 *
 * @brief       Balance Profile link status change handler function.
 *
 * @param       connHandle - connection handle
 * @param       changeType - type of change
 *
 * @return      none
 */
static void balanceProfile_HandleConnStatusCB( uint16 connHandle, uint8 changeType )
{ 
  // Make sure this is not loopback connection
  if ( connHandle != LOOPBACK_CONNHANDLE )
  {
    // Reset Client Char Config if connection has dropped
    if ( ( changeType == LINKDB_STATUS_UPDATE_REMOVED )      ||
         ( ( changeType == LINKDB_STATUS_UPDATE_STATEFLAGS ) && 
           ( !linkDB_Up( connHandle ) ) ) )
    { 
      GATTServApp_InitCharCfg( connHandle, balanceProfileTRxConfig );
    }
  }
}

uint8 BalanceProfile_Notify(const uint8 *pbuf, uint8 length)
{
	uint8 len;
	
	attHandleValueNoti_t pReport;
	
	if (length > 20)
	{
		len = 20;
	}
	else
	{
		len = length;
	}
	
	pReport.handle = balanceProfileAttrTbl[BALANCEPROFILE_CHAR_TRX_VAL_INDEX].handle;
	
	pReport.len = len;
	
	osal_memcpy(pReport.value, pbuf, len);
	
	GATT_Notification(0, &pReport, FALSE);

	return len;
}

/*********************************************************************
*********************************************************************/
