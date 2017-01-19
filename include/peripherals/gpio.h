#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>


/**
 * @file gpio.h
 *
 * @brief Interface to the GPIO peripheral
 *
 * This peripheral is able to set the function of each GPIO port and it's output status.\n
 * See documentation at "BCM2835 ARM peripherals" page 89
 */

///The lowest valid bcm port number
#define BCM_MIN 0
///The highest valid bcm port number
#define BCM_MAX 27

///Port function: Input. used in gpio_setFunction()
#define PORT_FUNCTION_INPUT	0
///Port function: Output. used in gpio_setFunction()
#define PORT_FUNCTION_OUTPUT	1
///Port function: Alternative function 0. used in gpio_setFunction()
#define PORT_FUNCTION_ALT_0	4
///Port function: Alternative function 1. used in gpio_setFunction()
#define PORT_FUNCTION_ALT_1	5
///Port function: Alternative function 2. used in gpio_setFunction()
#define PORT_FUNCTION_ALT_2	6
///Port function: Alternative function 3. used in gpio_setFunction()
#define PORT_FUNCTION_ALT_3	7
///Port function: Alternative function 4. used in gpio_setFunction()
#define PORT_FUNCTION_ALT_4	3
///Port function: Alternative function 5. used in gpio_setFunction()
#define PORT_FUNCTION_ALT_5	2



int gpio_init();

int gpio_isInit();

int gpio_setFunction(int bcmPort, int function);

int gpio_setOutput(int bcmPort, int value);

int gpio_setOutputAll(uint64_t set, uint64_t clear);


#endif
