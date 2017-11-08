
#ifndef _BATTERY_H_
#define _BATTERY_H_

extern void InitBatteryDetect();

//unit of returned value is 100mV
extern uint8 GetBatteryVoltage();

#endif

