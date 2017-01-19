
#A brief description

CRpi is a C library dedicated to the Raspberry Pi providing utilities and direct access to some hardware peripherals. It includes:

* PWM signal generation using the DMA (see pwmByDma.h)
* Interface to the GPIO peripheral (see gpio.h)
* Interface to the PWM peripheral (see pwm.h)
* Interface to the DMA peripheral (see dma.h)
* Interface to the Clock Manager peripheral (see clockManager.h)

It also includes some utilies, most notably cacheCoherentMemoryProvider.h

It currently supports **Raspberry Pi 2 and 3 model B**, it's tested only on the RPI 3 but should work also on the 2.

A more in-depth description can be found in the [**doxygen documentation**](https://9cr4.github.io/CRpi/html/index.html)
