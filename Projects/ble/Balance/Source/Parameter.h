
#ifndef _PARAMETER_H_
#define _PARAMETER_H_

#define MAX_BLE_NAME_LEN     20

#define FIRMWARE_VER_LEN  8
#define FIRMWARE_VER      "0.00.001"

#define WEB_SITE          "https://pcb-layout.taobao.com"

#define MAX_CALIBRATION_COUNT   2

struct CalibrationItem
{
	uint16 kgInt;
	uint16 kgFrac;

	uint32 adc;
};

typedef struct 
{
	struct CalibrationItem caliItem[MAX_CALIBRATION_COUNT];
	uint8   caliItemCount;

	uint32  tare;
} Parameter;


typedef struct
{
	uint8  header;

	uint16 size;

	Parameter parameter;

	uint8  sum;
} StoreVector;

extern StoreVector g_storeVector;

#define g_caliItem           g_storeVector.parameter.caliItem
#define g_caliItemCount      g_storeVector.parameter.caliItemCount
#define g_tare               g_storeVector.parameter.tare

extern bool LoadParameter();
extern void LoadDefaultParameter();

extern bool SaveParameter();

#endif


