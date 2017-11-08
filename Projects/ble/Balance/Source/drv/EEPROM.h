
#ifndef _EEPROM_H_
#define _EEPROM_H_

#include "24Cxx.h"

#define EEPROM_SIZE       (256)

#define WriteEEPROM(addr, dat)     Write24CxxByte(addr, dat)

#define ReadEEPROM(addr, pData)    Read24CxxByte(addr, pData)

#define WriteEEPROMData(addr, pData, len)  Write24CxxData(addr, pData, len)

#define ReadEEPROMData(addr, pData, len, pRetLen)  Read24CxxData(addr, pData, len, pRetLen)

#endif

