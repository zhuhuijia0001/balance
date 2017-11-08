
#ifndef _PACKET_H_
#define _PACKET_H_

typedef struct
{
	uint16 dec;

	uint16 frac;
} DataItem;

#define PACKET_OK              0
#define PACKET_NOT_COMPLETE    1
#define PACKET_INVALID         2

extern uint8 BuildEnableTransferRetPacket(uint8 *buf, uint8 maxBufLen, uint8 enable, uint8 result, uint8 reason);

extern uint8 BuildTestDataPacket(uint8 *buf, uint8 maxBufLen, const DataItem *data);

extern uint8 BuildStartCalibrationRetPacket(uint8 *buf, uint8 maxBufLen, uint8 start, uint8 reault, uint8 reason);

extern uint8 BuildCalibrateRetPacket(uint8 *buf, uint8 maxBufLen, const DataItem *data, uint8 reault, uint8 reason);

extern uint8 ParsePacket(const uint8 *buf, uint8 len, uint8 *parsedLen);

extern void ResetParsePacket();

extern const uint8 *GetPacketData();

extern uint8 GetPacketType();
extern uint8 GetPacketDataLen();

extern bool ParseEnableTransferPacket(const uint8 *data, uint8 len, uint8 *enable);

extern bool ParseStartCalibrationPacket(const uint8 *data, uint8 len, uint8 *start);

extern bool ParseCalibratePacket(const uint8 *data, uint8 len, DataItem *item);

#endif

