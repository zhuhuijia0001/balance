#include "hal_types.h"
#include "comdef.h"

#include "Protocol.h"

#include "Packet.h"

#include "npi.h"

static uint8 s_recvBuf[MAX_PACKET_LEN];

typedef enum
{
	recv_status_idle = 0,
	recv_status_version,
	recv_status_length,
	recv_status_data,
	recv_status_sum,
} BleReceiveStatus;

static BleReceiveStatus s_recvStatus = recv_status_idle;

static uint8 s_dataRecvIndex = 0;

static uint8 BuildPacket(uint8 *buf, uint8 maxBufLen, const uint8 *data, uint8 len)
{
	if (1 + 1 + 2 + len + 1 > maxBufLen)
	{
		return 0xff;
	}

	uint8 index = 0;

	buf[index++] = PACKET_HEADER;

	buf[index++] = PROTOCOL_VERSION;
	
	buf[index++] = (len + 1) >> 8;
	buf[index++] = (len + 1);

	uint8 sum = 0;
	uint8 i;
	for (i = 0; i < len; i++)
	{
		buf[index++] = *data;

		sum ^= *data;

		data++;
	}

	buf[index++] = sum;

	return index;
}

uint8 BuildEnableTransferRetPacket(uint8 *buf, uint8 maxBufLen, uint8 enable, uint8 result, uint8 reason)
{
	uint8 dataBuf[MAX_PACKET_LEN];
		
	uint8 index = 0;
	dataBuf[index++] = TYPE_ENABLE_TRANSFER;

	dataBuf[index++] = enable;
	dataBuf[index++] = result;
	dataBuf[index++] = reason;

	return BuildPacket(buf, maxBufLen, dataBuf, index);
}

uint8 BuildTestDataPacket(uint8 *buf, uint8 maxBufLen, const DataItem *data)
{
	uint8 dataBuf[MAX_PACKET_LEN];
	
	uint8 index = 0;
	dataBuf[index++] = TYPE_TRANSFER_DATA;

	dataBuf[index++] = data->dec >> 8;
	dataBuf[index++] = data->dec & 0xff;

	dataBuf[index++] = data->frac >> 8;
	dataBuf[index++] = data->frac & 0xff;
	
	return BuildPacket(buf, maxBufLen, dataBuf, index);
}

uint8 BuildStartCalibrationRetPacket(uint8 *buf, uint8 maxBufLen, uint8 start, uint8 reault, uint8 reason)
{
	uint8 dataBuf[MAX_PACKET_LEN];
		
	uint8 index = 0;
	dataBuf[index++] = TYPE_START_CALIBRATION;

	dataBuf[index++] = start;
	dataBuf[index++] = reault;
	dataBuf[index++] = reason;
	
	return BuildPacket(buf, maxBufLen, dataBuf, index);
}

uint8 BuildCalibrateRetPacket(uint8 *buf, uint8 maxBufLen, const DataItem *data, uint8 reault, uint8 reason)
{
	uint8 dataBuf[MAX_PACKET_LEN];
	
	uint8 index = 0;
	dataBuf[index++] = TYPE_CALIBRATE;

	dataBuf[index++] = data->dec >> 8;
	dataBuf[index++] = data->dec & 0xff;

	dataBuf[index++] = data->frac >> 8;
	dataBuf[index++] = data->frac & 0xff;

	dataBuf[index++] = reault;
	dataBuf[index++] = reason;
	
	return BuildPacket(buf, maxBufLen, dataBuf, index);
}

uint8 ParsePacket(const uint8 *buf, uint8 len, uint8 *parsedLen)
{
	static uint16 length = 0;
	static uint16 dataLeft = 0;

	static uint8 sum = 0;
	
	uint8 i;

	uint8 count = 0;
	
	uint8 res = PACKET_NOT_COMPLETE;
	
	for (i = 0; i < len; i++)
	{
		uint8 dat = *buf++;
		count++;
		
		switch (s_recvStatus)
		{
		case recv_status_idle:
			if (dat == PACKET_HEADER)
			{
				s_recvStatus = recv_status_version;
			}
			
			break;
	
		case recv_status_version:
			if (dat == PROTOCOL_VERSION)
			{
				s_recvStatus = recv_status_length;

				dataLeft = 2;
			}
			else
			{
				res = PACKET_INVALID;

				s_recvStatus = recv_status_idle;
			}
			
			break;
	
		case recv_status_length:
			length <<= 8;
			length |= dat;
			dataLeft--;
			if (dataLeft == 0)
			{
				if (length > MAX_PACKET_LEN - 1)
				{
					res = PACKET_INVALID;

					TRACE("packet invalid\r\n");
				}
				else
				{
					s_recvStatus = recv_status_data;

					dataLeft = length - 1;

					s_dataRecvIndex = 0;

					sum = 0x00;
				}
			}
			
			break;

		case recv_status_data:
			s_recvBuf[s_dataRecvIndex++] = dat;
			dataLeft--;
			sum ^= dat;
			
			if (dataLeft == 0)
			{
				s_recvStatus = recv_status_sum;
			}
	
			break;

		case recv_status_sum:
			if (sum == dat)
			{
				res = PACKET_OK;

				TRACE("packet OK\r\n");
			}
			else
			{
				res = PACKET_INVALID;

				TRACE("packet invalid\r\n");
			}

			s_recvStatus = recv_status_idle;
			
			break;
			
		}

		if (res == PACKET_OK || res == PACKET_INVALID)
		{
			break;
		}
	}

	if (parsedLen != NULL)
	{
		*parsedLen = count;
	}
	
	return res;
}

void ResetParsePacket()
{
	s_recvStatus = recv_status_idle;
	
	s_dataRecvIndex = 0;
}

const uint8 *GetPacketData()
{
	return s_recvBuf;
}

uint8 GetPacketType()
{
	return s_recvBuf[0];
}

uint8 GetPacketDataLen()
{
	return s_dataRecvIndex;
}

bool ParseEnableTransferPacket(const uint8 *data, uint8 len, uint8 *enable)
{
	if (len != SIZE_ENABLE_TRANSFER)
	{
		return false;
	}
	
	uint8 index = 0;

	uint8 dat;
	
	index++;

	dat = data[index++];
	if (enable != NULL)
	{
		*enable = dat;
	}

	return true;
}

bool ParseStartCalibrationPacket(const uint8 *data, uint8 len, uint8 *start)
{
	if (len != SIZE_START_CALIBRATION)
	{
		return false;
	}

	uint8 index = 0;

	uint8 dat;

	index++;
	dat = data[index++];
	if (start != NULL)
	{
		*start = dat;
	}
	
	return true;
}

bool ParseCalibratePacket(const uint8 *data, uint8 len, DataItem *item)
{
	if (len != SIZE_CALIBRATE)
	{
		return false;
	}
	
	uint8 index = 0;
	index++;

	uint16 val;
	val = data[index++];
	val <<= 8;
	val |= data[index++];
	
	if (item != NULL)
	{
		item->dec = val;
	}

	val = data[index++];
	val <<= 8;
	val |= data[index++];
	if (item != NULL)
	{
		item->frac = val;
	}
	
	return true;
}

