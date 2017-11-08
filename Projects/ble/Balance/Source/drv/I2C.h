
#ifndef _I2C_H_
#define _I2C_H_

extern void InitI2C();

extern void I2CStart();

extern void I2CStop();

extern void WriteI2CByte(uint8 c);

extern bool CheckI2CAck();

extern uint8 ReceiveI2CByte();

extern void SendI2CAck(bool ack);

#endif

