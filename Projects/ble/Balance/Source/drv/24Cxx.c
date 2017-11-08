#include "comdef.h"
#include "hal_types.h"

#include "I2C.h"
#include "24Cxx.h"

void Init24Cxx()
{
	InitI2C();
}

static bool Write24CxxByteInternal(uint8 devAddr, uint16 addr, uint8 addrLen, uint8 dat)
{
	uint8 shift;
	uint8 timeout = 0xff;
	
	I2CStart();
	
	WriteI2CByte(devAddr);
	if (!CheckI2CAck())
	{
		I2CStop();
		return false;
	}
	
	shift = (addrLen - 1) * 8;
	while (addrLen--)
	{
		WriteI2CByte(addr >> shift);
		if (!CheckI2CAck())
		{
			I2CStop();
			return false;
		}
		
		shift -= 8;;
	}
	
	WriteI2CByte(dat);
	if (!CheckI2CAck())
	{
		I2CStop();
		return false;
	}
	
	I2CStop();

	//Polling
	while (true)
	{
		I2CStart();
		WriteI2CByte(devAddr);
		if (CheckI2CAck())
		{
			break;
		}
		
		timeout--;
		if (timeout == 0)
		{
			return false;
		}
	}
	
	return true;	
}

/****************************************************************
Description:  Write a byte at a specified address
Input:
addr :        Address to write at 
dat  : 		  Byte to write

Output:       true :success
			  false:failure
****************************************************************/
bool Write24CxxByte(uint16 addr, uint8 dat)
{
	uint8 devAddr = 0xa0;
	uint8 addrLen;
	
#if EEPROM_TYPE == EEPROM_TYPE_24C01 || EEPROM_TYPE == EEPROM_TYPE_24C02
	devAddr |= ((EEPROM_PIN_A2 << 3)
				|(EEPROM_PIN_A1 << 2)
				|(EEPROM_PIN_A0 << 1));
	
	addrLen = 1;
	
#elif EEPROM_TYPE == EEPROM_TYPE_24C04
	devAddr |= ((EEPROM_PIN_A2 << 3)
				|(EEPROM_PIN_A1 << 2));
	
	devAddr |= (((addr >> 8) & 0x01) << 1);
	
	addrLen = 1;
	
#elif EEPROM_TYPE == EEPROM_TYPE_24C08
	devAddr |= (EEPROM_PIN_A2 << 3);
	
	devAddr |= (((addr >> 8) & 0x03) << 1);
	
	addrLen = 1;
	
#elif EEPROM_TYPE == EEPROM_TYPE_24C16
	devAddr |= (((addr >> 8) & 0x07) << 1);
	
	addrLen = 1;
	
#elif EEPROM_TYPE == EEPROM_TYPE_24C32 || EEPROM_TYPE == EEPROM_TYPE_24C64
	devAddr |= ((EEPROM_PIN_A2 << 3)
				|(EEPROM_PIN_A1 << 2)
				|(EEPROM_PIN_A0 << 1));
	
	addrLen = 2;
	
#endif
	
	return Write24CxxByteInternal(devAddr, addr, addrLen, dat);	
}

static bool Read24CxxByteInternal(uint8 devAddr, uint16 addr, uint8 addrLen, uint8 *pData)
{
	uint8 shift;
		
	I2CStart();
	
	WriteI2CByte(devAddr);
	if (!CheckI2CAck())
	{
		I2CStop();
		return false;
	}
	
	shift = (addrLen - 1) * 8;
	while (addrLen--)
	{
		WriteI2CByte(addr >> shift);
		if (!CheckI2CAck())
		{
			I2CStop();
			return false;
		}
		
		shift -= 8;;
	}
	
	I2CStart();
	
	WriteI2CByte(devAddr | 0x01);
	if (!CheckI2CAck())
	{
		I2CStop();
		return false;
	}
	
	*pData = ReceiveI2CByte();

	SendI2CAck(false);
	
	I2CStop();
	
	return true;	
}

/****************************************************************
Description:  Read a byte from a specified address
Input:
addr :        Address to read from 
pData: 		  Byte to read

Output:       true :success
			  false:failure
****************************************************************/
bool Read24CxxByte(uint16 addr, uint8 *pData)
{
	uint8 devAddr = 0xa0;
	uint8 addrLen;
	
#if EEPROM_TYPE == EEPROM_TYPE_24C01 || EEPROM_TYPE == EEPROM_TYPE_24C02
	devAddr |= ((EEPROM_PIN_A2 << 3)
				|(EEPROM_PIN_A1 << 2)
				|(EEPROM_PIN_A0 << 1));
	
	addrLen = 1;
	
#elif EEPROM_TYPE == EEPROM_TYPE_24C04
	devAddr |= ((EEPROM_PIN_A2 << 3)
				|(EEPROM_PIN_A1 << 2));
	
	devAddr |= (((addr >> 8) & 0x01) << 1);
	
	addrLen = 1;
	
#elif EEPROM_TYPE == EEPROM_TYPE_24C08
	devAddr |= (EEPROM_PIN_A2 << 3);
	
	devAddr |= (((addr >> 8) & 0x03) << 1);
	
	addrLen = 1;
	
#elif EEPROM_TYPE == EEPROM_TYPE_24C16
	devAddr |= (((addr >> 8) & 0x07) << 1);
	
	addrLen = 1;
	
#elif EEPROM_TYPE == EEPROM_TYPE_24C32 || EEPROM_TYPE == EEPROM_TYPE_24C64
	devAddr |= ((EEPROM_PIN_A2 << 3)
				|(EEPROM_PIN_A1 << 2)
				|(EEPROM_PIN_A0 << 1));
	
	addrLen = 2;
	
#endif
	
	return Read24CxxByteInternal(devAddr, addr, addrLen, pData);	
}

static bool Write24CxxDataPageInternal(uint8 devAddr, uint16 addr, uint8 addrLen, const uint8 *pData, uint8 size)
{
	uint8 shift;
	uint8 timeout = 0xff;
	
	I2CStart();
	
	WriteI2CByte(devAddr);
	if (!CheckI2CAck())
	{
		I2CStop();
		return false;
	}
	
	shift = (addrLen - 1) * 8;
	while (addrLen--)
	{
		WriteI2CByte(addr >> shift);
		if (!CheckI2CAck())
		{
			I2CStop();
			return false;
		}
		
		shift -= 8;;
	}
	
	while (size--)
	{
		WriteI2CByte(*pData++);
		if (!CheckI2CAck())
		{
			I2CStop();
			return false;
		}
	}
	
	I2CStop();

	//Polling
	while (true)
	{
		I2CStart();
		WriteI2CByte(devAddr);
		if (CheckI2CAck())
		{
			break;
		}
		
		timeout--;
		if (timeout == 0)
		{
			return false;
		}
	}
	
	return true;	
}

static bool Write24CxxDataPage(uint16 addr, const uint8 *pData, uint8 size)
{
	uint8 devAddr = 0xa0;
	uint8 addrLen;
	
#if EEPROM_TYPE == EEPROM_TYPE_24C01 || EEPROM_TYPE == EEPROM_TYPE_24C02
	devAddr |= ((EEPROM_PIN_A2 << 3)
				|(EEPROM_PIN_A1 << 2)
				|(EEPROM_PIN_A0 << 1));
	
	addrLen = 1;
	
#elif EEPROM_TYPE == EEPROM_TYPE_24C04
	devAddr |= ((EEPROM_PIN_A2 << 3)
				|(EEPROM_PIN_A1 << 2));
	
	devAddr |= (((addr >> 8) & 0x01) << 1);
	
	addrLen = 1;
	
#elif EEPROM_TYPE == EEPROM_TYPE_24C08
	devAddr |= (EEPROM_PIN_A2 << 3);
	
	devAddr |= (((addr >> 8) & 0x03) << 1);
	
	addrLen = 1;
	
#elif EEPROM_TYPE == EEPROM_TYPE_24C16
	devAddr |= (((addr >> 8) & 0x07) << 1);
	
	addrLen = 1;
	
#elif EEPROM_TYPE == EEPROM_TYPE_24C32 || EEPROM_TYPE == EEPROM_TYPE_24C64
	devAddr |= ((EEPROM_PIN_A2 << 3)
				|(EEPROM_PIN_A1 << 2)
				|(EEPROM_PIN_A0 << 1));
	
	addrLen = 2;
	
#endif
	
	return Write24CxxDataPageInternal(devAddr, addr, addrLen, pData, size);
}

/****************************************************************
Description:  Write data from a specified address
Input:
addr :        Address to write from
pData:        Data to write
size : 		  Count of data in byte

Output:       true :success
			  false:failure
****************************************************************/
bool Write24CxxData(uint16 addr, const uint8 *pData, uint16 size)
{
	uint16 left = size;
	
	uint16 pageOffset = addr % EEPROM_PAGE_SIZE;

	uint16 pageRemain = EEPROM_PAGE_SIZE - pageOffset;
	
	if (pageRemain > left)
	{
		pageRemain = left;
	}

	if (pageRemain < EEPROM_PAGE_SIZE)
	{
		if (!Write24CxxDataPage(addr, pData, pageRemain))
		{
			return false;
		}
		
		left -= pageRemain;
		pData += pageRemain;
		addr += pageRemain;
	}
	
	while (left >= EEPROM_PAGE_SIZE)
	{
		if (!Write24CxxDataPage(addr, pData, EEPROM_PAGE_SIZE))
		{
			return false;
		}
			
		pData += EEPROM_PAGE_SIZE;
		addr += EEPROM_PAGE_SIZE;
		
		left -= EEPROM_PAGE_SIZE;
	}
	
	if (left > 0)
	{
		if (!Write24CxxDataPage(addr, pData, left))
		{
			return false;
		}
	}
	
	return true;	
}

static bool Read24CxxDataInternal(uint8 devAddr, uint16 addr, uint8 addrLen, uint8 *pData, uint16 size, uint16 *pSizeRet)
{
	uint8 shift;
	
	uint16 count = 0;
	
	I2CStart();
	
	WriteI2CByte(devAddr);
	if (!CheckI2CAck())
	{
		I2CStop();
		
		return false;
	}
	
	shift = (addrLen - 1) * 8;
	while (addrLen--)
	{
		WriteI2CByte(addr >> shift);
		if (!CheckI2CAck())
		{
			I2CStop();
			
			return false;
		}
		
		shift -= 8;;
	}
	
	I2CStart();
	WriteI2CByte(devAddr | 0x01);
	if (!CheckI2CAck())
	{
		I2CStop();
		
		return false;
	}
	
	while (size--)
	{
		*pData++ = ReceiveI2CByte();
		SendI2CAck(size > 0);
		
		count++;
	}
	
	I2CStop();
	
	if (pSizeRet != NULL)
	{
		*pSizeRet = count;
	}
	
	return true;	
}

/****************************************************************
Description:  Read data from a specified address
Input:
addr :        Address to read from
pData:        Buffer to read data
size : 		  Count of data in byte to read
pSizeRet:     Count of data that has read

Output:       true :success
			  false:failure
****************************************************************/
bool Read24CxxData(uint16 addr, uint8 *pData, uint16 size, uint16 *pSizeRet)
{
	uint8 devAddr = 0xa0;
	uint8 addrLen;
	
#if (EEPROM_TYPE == EEPROM_TYPE_24C01) || (EEPROM_TYPE == EEPROM_TYPE_24C02)
	devAddr |= ((EEPROM_PIN_A2 << 3)
				|(EEPROM_PIN_A1 << 2)
				|(EEPROM_PIN_A0 << 1));
	
	addrLen = 1;
	
#elif EEPROM_TYPE == EEPROM_TYPE_24C04
	devAddr |= ((EEPROM_PIN_A2 << 3)
				|(EEPROM_PIN_A1 << 2));
	
	devAddr |= (((addr >> 8) & 0x01) << 1);
	
	addrLen = 1;
	
#elif EEPROM_TYPE == EEPROM_TYPE_24C08
	devAddr |= (EEPROM_PIN_A2 << 3);
	
	devAddr |= (((addr >> 8) & 0x03) << 1);
	
	addrLen = 1;
	
#elif EEPROM_TYPE == EEPROM_TYPE_24C16
	devAddr |= (((addr >> 8) & 0x07) << 1);
	
	addrLen = 1;
	
#elif (EEPROM_TYPE == EEPROM_TYPE_24C32) || (EEPROM_TYPE == EEPROM_TYPE_24C64)
	devAddr |= ((EEPROM_PIN_A2 << 3)
				|(EEPROM_PIN_A1 << 2)
				|(EEPROM_PIN_A0 << 1));
	
	addrLen = 2;
	
#endif
	
	return 	Read24CxxDataInternal(devAddr, addr, addrLen, pData, size, pSizeRet);	
}

