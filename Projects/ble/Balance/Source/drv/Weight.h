
#ifndef _WEIGHT_H_
#define _WEIGHT_H_

struct CalibrationItem;

extern void InitWeigh();

extern void StartWeigh(void (*callback)(uint16 integer, uint16 frac), bool repeat);

extern void SetTare(uint32 tareAdc);

extern void SetCalibration(const struct CalibrationItem *caliItem, uint8 caliItemCount);

extern void StartCalibrate(void (*callback)(uint16 integer, uint16 frac, uint32 adc), uint16 integer, uint16 frac);

extern void StopWeigh();

#endif

