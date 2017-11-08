
#ifndef _HX711_H_
#define _HX711_H_

extern void InitHx711();

extern void StartHx711(void (*callback)(uint32), bool repeat);

extern void StopHx711();

extern void ProcessHx711Event();

#endif

