
#include "comdef.h"
#include "hal_types.h"

#include "PinDefine.h"

#include "Usb.h"

void EnableUsb(bool enable)
{
	SET_GPIO_OUTPUT(PIN_USB_UP);

	if (enable)
	{
		SET_GPIO_BIT(PIN_USB_UP);
	}
	else
	{
		CLR_GPIO_BIT(PIN_USB_UP);
	}
}

