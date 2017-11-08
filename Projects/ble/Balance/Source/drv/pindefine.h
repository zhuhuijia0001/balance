
#ifndef _PINDEFINE_H_
#define _PINDEFINE_H_

#if defined( CC2541) || defined( CC2541S )
#include <ioCC2541.h>
#else // CC2540
#include <ioCC2540.h>
#endif // CC2541 || CC2541S


#include "gpio.h"

/* i2c pin */
#define PORT_PIN_SDA     0
#define OFFSET_PIN_SDA   6
#define DIR_PIN_SDA      PORT_DIR(PORT_PIN_SDA)
#define PIN_SDA          MAKE_GPIO(PORT_PIN_SDA, OFFSET_PIN_SDA)


#define PORT_PIN_SCL     0
#define OFFSET_PIN_SCL   5
#define DIR_PIN_SCL      PORT_DIR(PORT_PIN_SCL)
#define PIN_SCL          MAKE_GPIO(PORT_PIN_SCL, OFFSET_PIN_SCL)

//pin clk
#define PORT_PIN_SLK     1
#define OFFSET_PIN_SLK   2
#define DIR_PIN_SLK      PORT_DIR(PORT_PIN_SLK)
#define PIN_SLK          MAKE_GPIO(PORT_PIN_SLK, OFFSET_PIN_SLK)

//pin dt
#define PORT_PIN_DT      1
#define OFFSET_PIN_DT    3
#define DIR_PIN_DT       PORT_DIR(PORT_PIN_DT)
#define PIN_DT           MAKE_GPIO(PORT_PIN_DT, OFFSET_PIN_DT)

//pin led
#define PORT_PIN_LED     1
#define OFFSET_PIN_LED   0
#define DIR_PIN_LED      PORT_DIR(PIN_LED)
#define PIN_LED          MAKE_GPIO(PORT_PIN_LED, OFFSET_PIN_LED)

//pin vdd_e
#define PORT_PIN_VDD     1
#define OFFSET_PIN_VDD   1
#define DIR_PIN_VDD      PORT_DIR(PORT_PIN_VDD)
#define PIN_VDD          MAKE_GPIO(PORT_PIN_VDD, OFFSET_PIN_VDD)

//pin usb up
#define PORT_PIN_USB_UP    0
#define OFFSET_PIN_USB_UP  0
#define DIR_PIN_USB_UP     PORT_DIR(PORT_PIN_USB_UP)
#define PIN_USB_UP         MAKE_GPIO(PORT_PIN_USB_UP, OFFSET_PIN_USB_UP)

#endif

