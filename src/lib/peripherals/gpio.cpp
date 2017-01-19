/**
 * @file gpio.cpp
 *
 * @brief	Interface to the GPIO peripheral
 *
 * If you haven't already, see gpio.h
 * See documentation at "BCM2835 ARM peripherals" page 89
 */
#include <stdio.h>
#include <memoryManagement/addressing.h>
#include <memoryManagement/physMemoryManagement.h>
#include <CCRCodingUtils/include/errorManagement.h>
#include <peripherals/gpio.h>
#include "gpio_internal.h"

static void* baseAddrVirt = 0;

/**
 * @brief Initializes the interface, to be called before any other function in this file
 *
 * @return negative in case of error
 */
int gpio_init()
{
	if(baseAddrVirt)
	{
		return ERROR("gpio already initialized",-1);
	}
	baseAddrVirt = mapPhysMemArea(GPIO_BASE_ADDR_PHYS,GPIO_AREA_LEN);
	if(!baseAddrVirt)
		return ERROR("Can't map gpio peripheral",-2);
	//DBG_PRINTF("gpio baseAddrVirt = 0x%x\n",baseAddrVirt);
	return 0;
}

/**
 * @brief true if gpio_init has been called with success
 */
int gpio_isInit()
{
	return baseAddrVirt!=0;
}




/**
 * @brief Sets the function of the specified port
 *
 * @param bcmPort	The port bcm number
 * @param function	The desired function (PORT_FUNCTION_INPUT, PORT_FUNCTION_OUTPUT, PORT_FUNCTION_ALT_x)
 *
 * @return negative in case of error
 */
int gpio_setFunction(int bcmPort, int function)
{
	if(bcmPort<BCM_MIN || bcmPort>BCM_MAX)
		return ERROR("Invalid bcmPort",-1);
	if(function>7)
		return ERROR("Invalid function",-2);
	if(!baseAddrVirt)
		return ERROR("Gpio not initialized",-3);

	int gpfsel_n = bcmPort/10;
	int i	=	bcmPort%10;
	volatile uint32_t* reg = (volatile uint32_t*)(baseAddrVirt + GPFSELx_OFF(gpfsel_n));

	*reg = (*reg & (~(0x7<<(i*3)))) | ((function & 0x7)<<(i*3));
	//DBG_PRINTF("GPFSEL %d = 0x%x\n",gpfsel_n,*reg);
	return 0;
}

/**
 * @brief	Sets the output at the specified port. The port will output this value if set to the output function.
 * 		If you use this function with the port not set to output then the port will remember the value until you set it to the output function.
 *
 * @param bcmPort	The port bcm number
 * @param value		0 means low, any other number means high
 *
 * @return negative in case of error
 */
int gpio_setOutput(int bcmPort, int value)
{
	if(bcmPort<BCM_MIN || bcmPort>BCM_MAX)
		return ERROR("Invalid bcmPort",-1);
	if(!baseAddrVirt)
		return ERROR("Gpio not initialized",-2);

	volatile uint32_t* reg;
	if(value)
	{
		if(bcmPort<32)
		{
			reg = (volatile uint32_t*)(baseAddrVirt + GPSET0_OFF);//PERIPH_ADDR_PHYS_TO_VIRT(	GPSET0_ADDR_PHYS,	baseAddrVirt,	GPIO_BASE_ADDR_PHYS);
			*reg = 1<<bcmPort;
		}
		else
		{
			reg = (volatile uint32_t*)(baseAddrVirt + GPSET1_OFF);//PERIPH_ADDR_PHYS_TO_VIRT(	GPSET1_ADDR_PHYS,	baseAddrVirt,	GPIO_BASE_ADDR_PHYS);
			*reg = 1<<(bcmPort-32);
		}
	}
	else
	{
		if(bcmPort<32)
		{
			reg = (volatile uint32_t*)(baseAddrVirt + GPCLEAR0_OFF);//PERIPH_ADDR_PHYS_TO_VIRT(	GPCLEAR0_ADDR_PHYS,	baseAddrVirt,	GPIO_BASE_ADDR_PHYS);
			*reg = 1<<bcmPort;
		}
		else
		{
			reg = (volatile uint32_t*)(baseAddrVirt + GPCLEAR1_OFF);//PERIPH_ADDR_PHYS_TO_VIRT(	GPCLEAR1_ADDR_PHYS,	baseAddrVirt,	GPIO_BASE_ADDR_PHYS);
			*reg = 1<<(bcmPort-32);
		}
	}
	return 0;
}

/**
 * @brief sets all the specified gpios to the specified values
 *
 * @param set	The gpios to set to 1
 * @param clear	The values to set to 0
 *
 * @return negative in case of error
 */
int gpio_setOutputAll(uint64_t set, uint64_t clear)
{
	if(!baseAddrVirt)
		return ERROR("Gpio not initialized",-1);

	volatile uint32_t* reg;
	//ports 0-32
	reg = (volatile uint32_t*)(baseAddrVirt + GPSET0_OFF);
	*reg = (uint32_t)set;
	reg = (volatile uint32_t*)(baseAddrVirt + GPCLEAR0_OFF);
	*reg = (uint32_t)clear;
	//ports 32-53
	reg = (volatile uint32_t*)(baseAddrVirt + GPSET1_OFF);
	*reg = (uint32_t)(set>>32) & 0x3FFFFF;
	reg = (volatile uint32_t*)(baseAddrVirt + GPCLEAR1_OFF);
	*reg = (uint32_t)(clear>>32) & 0x3FFFFF;
}
/**
 * @brief	Gets the detected value of the port in input mode. Obviously you can use this only in input mode, you wold kill your rpi otherwise.
 *
 * @param bcmPort The port.
 *
 * @return The level: 1 or 0.
 */
int gpio_getInput(int bcmPort)
{
	if(bcmPort<BCM_MIN || bcmPort>BCM_MAX)
		return ERROR("Invalid bcmPort",-1);
	if(!baseAddrVirt)
		return ERROR("Gpio not initialized",-2);

	volatile uint32_t* reg;
	if(bcmPort<32)
	{
		reg = (volatile uint32_t*)(baseAddrVirt + GPLEV0_OFF);//PERIPH_ADDR_PHYS_TO_VIRT(	GPLEV0_ADDR_PHYS,	baseAddrVirt,	GPIO_BASE_ADDR_PHYS);
		return (*reg>>bcmPort) & 1;
	}
	else
	{
		reg = (volatile uint32_t*)(baseAddrVirt + GPLEV1_OFF);//PERIPH_ADDR_PHYS_TO_VIRT(	GPLEV1_ADDR_PHYS,	baseAddrVirt,	GPIO_BASE_ADDR_PHYS);
		return (*reg>>(bcmPort-32)) & 1;
	}
}





