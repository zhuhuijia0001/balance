
#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#define MAX_PACKET_LEN              40

//header 
#define PACKET_HEADER               0xA5

//protocol version
#define PROTOCOL_VERSION            1

//data type
#define TYPE_ENABLE_TRANSFER        1
#define TYPE_TRANSFER_DATA          2
#define TYPE_START_CALIBRATION      3
#define TYPE_CALIBRATE              4


//packet data len
#define SIZE_ENABLE_TRANSFER        2
#define SIZE_START_CALIBRATION      2
#define SIZE_CALIBRATE              5

#define TRANSFER_ENABLE             1
#define TRANSFER_DISABLE            0

#define CALIBRATION_START           1
#define CALIBRATION_END             0

#define RESULT_OK                   0
#define RESULT_FAILED               1

//reason
#define REASON_NONE                  0
#define REASON_NOT_CALIBRATED        1
#define REASON_CALIBRATE_STARTED     2
#define REASON_CALIBRATE_NOT_STARTED 3
#define REASON_BEING_CALIBRATED      4
#define REASON_CALIBRATION_POINT_NUM_WRONG  5
#define REASON_CALIBRATION_POINT_INVALID  6

#define REASON_OTHER                 0xff
       
#endif

