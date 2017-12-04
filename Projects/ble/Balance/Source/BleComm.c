#include "comdef.h"
#include "hal_types.h"

#include "Parameter.h"

#include "Protocol.h"
#include "Packet.h"

#include "Weight.h"

#include "gatt.h"
#include "balanceGATTprofile.h"

#include "Balance.h"

#include "BleComm.h"

#include "npi.h"

typedef enum
{
	status_calibration_idle = 0,

	status_calibration_ready,

	status_calibrating,
	
} CalibrationStatus;

static CalibrationStatus s_caliStatus = status_calibration_idle;

static struct CalibrationItem s_caliItem[MAX_CALIBRATION_COUNT];
static uint8 s_caliItemCount = 0;

#define CALIBRATION_ADC_DELTA_THREASHOLD     0x100

#define BLE_MAX_BYTE_PER_PACKET       20ul

static void SendBleData(uint8 *buf, uint8 len)
{
	/*
#ifdef DEBUG_PRINT
	TRACE("send ble data:");
	for (i = 0; i < len; i++)
	{
		TRACE("0x%02X ", data[i]);
	}
	TRACE("\r\n");
#endif
	*/
	
	uint8 segCnt = (len + BLE_MAX_BYTE_PER_PACKET - 1) / BLE_MAX_BYTE_PER_PACKET;

	uint8 left = segCnt * BLE_MAX_BYTE_PER_PACKET - len;
	uint8 i;
	for (i = 0; i < left; i++)
	{
		buf[len + i] = 0x00;
	}
	
	for (i = 0; i < segCnt; i++)
	{
		BalanceProfile_Notify(buf + i * BLE_MAX_BYTE_PER_PACKET, BLE_MAX_BYTE_PER_PACKET);
	}
}

static void WeighCallback(uint16 integer, uint16 frac)
{
	TRACE("%d.%04dkg\r\n", integer, frac);

	DataItem item = 
	{
		.dec = integer,
		.frac = frac
	};
	
	uint8 buf[MAX_PACKET_LEN];
	uint8 len = BuildTestDataPacket(buf, sizeof(buf), &item);
	SendBleData(buf, len);
}

static void ProcessEnableTransfer(uint8 enable)
{
	uint8 result;
	uint8 reason;
	
	if (enable == TRANSFER_ENABLE)
	{
		if (g_caliItemCount == 0)
		{
			result = RESULT_FAILED;

			reason = REASON_NOT_CALIBRATED;
		}
		else
		{
			if (s_caliStatus == status_calibration_idle)
			{
				result = RESULT_OK;

				reason = REASON_NONE;
				
				SetTare(g_tare);

				TRACE("start weigh\r\n");
				StartWeigh(WeighCallback, true);
			}
			else
			{
				result = RESULT_FAILED;

				reason = REASON_CALIBRATE_STARTED;
			}
		}
	}
	else
	{
		result = RESULT_OK;

		reason = REASON_NONE;
				
		StopWeigh();
	}

	uint8 buf[MAX_PACKET_LEN];

	TRACE("result:%d,reason:%d\r\n", result, reason);
	
	uint8 len = BuildEnableTransferRetPacket(buf, sizeof(buf), enable, result, reason);
	SendBleData(buf, len);
}

static void ProcessStartCalibration(uint8 start)
{
	uint8 result;

	uint8 reason;
	
	if (start == CALIBRATION_START)
	{
		if (s_caliStatus == status_calibration_ready)
		{
			result = RESULT_FAILED;

			reason = REASON_CALIBRATE_STARTED;
		}
		else if (s_caliStatus == status_calibrating)
		{
			result = RESULT_FAILED;

			reason = REASON_BEING_CALIBRATED;
		}
		else
		{
			result = RESULT_OK;
			
			reason = REASON_NONE;
			
			s_caliItemCount = 0;

			s_caliStatus = status_calibration_ready;
		}
	}
	else
	{
		if (s_caliStatus == status_calibrating)
		{
			result = RESULT_FAILED;

			reason = REASON_BEING_CALIBRATED;
		}
		else if (s_caliStatus == status_calibration_idle)
		{
			result = RESULT_FAILED;

			reason = REASON_CALIBRATE_NOT_STARTED;
		}
		else
		{
			if (s_caliItemCount == MAX_CALIBRATION_COUNT)
			{
				//save
				uint8 i;
				for (i = 0; i < MAX_CALIBRATION_COUNT; i++)
				{	
					g_caliItem[i] = s_caliItem[i];
				}
				g_caliItemCount = s_caliItemCount;

				s_caliItemCount = 0;

				result = RESULT_OK;
			
				reason = REASON_NONE;
			
				SaveParameter();

				SetCalibration(g_caliItem, g_caliItemCount);
			}
			else
			{

				result = RESULT_FAILED;
			
				reason = REASON_CALIBRATION_POINT_NUM_WRONG;
			}

			s_caliStatus = status_calibration_idle;
		}
	}

	uint8 buf[MAX_PACKET_LEN];

	TRACE("result:%d,reason:%d\r\n", result, reason);
	
	uint8 len = BuildStartCalibrationRetPacket(buf, sizeof(buf), start, result, reason);
	SendBleData(buf, len);
}

static void CalibrateCallback(uint16 integer, uint16 frac, uint32 adc)
{	
	uint8 result = RESULT_OK;

	uint8 reason = REASON_NONE;
	
	if (s_caliItemCount == 0)
	{
		s_caliItem[s_caliItemCount].kgInt = integer;
		s_caliItem[s_caliItemCount].kgFrac = frac;
		s_caliItem[s_caliItemCount].adc = adc;
		
		s_caliItemCount++;
	}
	else if (s_caliItemCount == 1)
	{
		if (adc <= s_caliItem[0].adc + CALIBRATION_ADC_DELTA_THREASHOLD
			&& adc + CALIBRATION_ADC_DELTA_THREASHOLD >= s_caliItem[0].adc)
		{
			//invalid calibration point
			result = RESULT_FAILED;

			reason = REASON_CALIBRATION_POINT_INVALID;
		}
		else
		{
			s_caliItem[s_caliItemCount].kgInt = integer;
			s_caliItem[s_caliItemCount].kgFrac = frac;
			s_caliItem[s_caliItemCount].adc = adc;
			
			s_caliItemCount++;
		}
	}

	s_caliStatus = status_calibration_ready;
	
	uint8 buf[MAX_PACKET_LEN];
	DataItem item;
	item.dec = integer;
	item.frac = frac;
	
	uint8 len = BuildCalibrateRetPacket(buf, sizeof(buf), &item, result, reason);
	SendBleData(buf, len);
}

static void ProcessCalibrate(const DataItem *item)
{
	uint8 result;

	uint8 reason;
	
	TRACE("calibration:%d.%04dkg\r\n", item->dec, item->frac);
	
	if (s_caliStatus == status_calibration_idle)
	{
		result = RESULT_FAILED;
		
		reason = REASON_CALIBRATE_NOT_STARTED;
	}
	else if (s_caliStatus == status_calibrating)
	{
		result = RESULT_FAILED;

		reason = REASON_BEING_CALIBRATED;
	}
	else
	{
		if (s_caliItemCount < MAX_CALIBRATION_COUNT)
		{
			result = RESULT_OK;
			
			reason = REASON_NONE;

			s_caliStatus = status_calibrating;

			StartCalibrate(CalibrateCallback, item->dec, item->frac);
		}
		else
		{
			result = RESULT_FAILED;
			
			reason = REASON_CALIBRATION_POINT_NUM_WRONG;
		}
	}
	
	if (result != RESULT_OK)
	{
		uint8 buf[MAX_PACKET_LEN];

		TRACE("result:%d,reason:%d\r\n", result, reason);
		
		uint8 len = BuildCalibrateRetPacket(buf, sizeof(buf), item, result, reason);
		SendBleData(buf, len);
	}
}

void ProcessBleCom(const uint8 *buf, uint8 len)
{
	uint8 parsedLen;

	uint8 res = ParsePacket(buf, len, &parsedLen);
	if (res == PACKET_OK)
	{
		Balance_StopTimer(BALANCE_PARSE_PACKET_TIMEOUT_EVT);
		
		uint8 type = GetPacketType();
		uint8 dataLen = GetPacketDataLen();
		const uint8 *data = GetPacketData();

		TRACE("type:%d,dataLen:%d\r\n", type, dataLen);
		
		switch (type)
		{
		case TYPE_ENABLE_TRANSFER:
			{
				uint8 enable;
				if (ParseEnableTransferPacket(data, dataLen, &enable))
				{
					TRACE("parse enable transfer ok,enable:0x%02X\r\n", enable);

					ProcessEnableTransfer(enable);
				}
				else
				{
					TRACE("parse enable transfer failed\r\n");
				}
			}

			break;

		case TYPE_START_CALIBRATION:
			{
				uint8 start;
				if (ParseStartCalibrationPacket(data, dataLen, &start))
				{
					TRACE("parse start calibration ok, start:0x%02X\r\n", start);

					ProcessStartCalibration(start);
				}
				else
				{
					TRACE("parse start calibration failed\r\n");
				}
			}
			
			break;

		case TYPE_CALIBRATE:
			{
				DataItem item;
				if (ParseCalibratePacket(data, dataLen, &item))
				{
					TRACE("parse calibrate ok,%d.%04dkg\r\n", item.dec, item.frac);

					ProcessCalibrate(&item);
				}
				else
				{
					TRACE("parse calibrate failed\r\n");
				}
			}
			
			break;	
		}
	}
	else if (res == PACKET_INVALID)
	{
		TRACE("invalid packet\r\n");
		Balance_StopTimer(BALANCE_PARSE_PACKET_TIMEOUT_EVT);
	}
	else
	{
		//incomplete
		TRACE("incomplete packet\r\n");
		Balance_StartTimer(BALANCE_PARSE_PACKET_TIMEOUT_EVT, 5000ul, false);
	}
}

void ProcessBleDisconnected()
{
	StopWeigh();
	
	s_caliStatus = status_calibration_idle;
}

void ProcessBleParsePacketTimeout()
{
	ResetParsePacket();
}

