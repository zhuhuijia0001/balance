
#ifndef _BLECOMM_H_
#define _BLECOMM_H_

extern void ProcessBleCom(const uint8 *buf, uint8 len);

extern void ProcessBleDisconnected();

extern void ProcessBleParsePacketTimeout();

#endif

