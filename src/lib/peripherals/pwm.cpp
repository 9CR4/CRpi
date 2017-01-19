/**
 * @file pwm.cpp
 *
 * @brief 	Interface to the PWM peripheral
 *
 * If you haven't already, see pwm.h
 * See documentation at  "BCM2835 ARM peripherals" pag 138d
 */

#include <peripherals/clockManager.h>
#include <memoryManagement/physMemoryManagement.h>
#include <peripherals/pwm.h>
#include <memoryManagement/addressing.h>
#include <CCRCodingUtils/include/errorManagement.h>
#include <stdlib.h>
#include <unistd.h>

#include "pwm_internal.h"





static void* baseAddrVirt = 0;



/**
 * @brief Initializes this peripheral interface, to use before any other function in this file
 *
 * @return negative in case of error
 */
int pwm_init()
{
	if(baseAddrVirt)
	{
		return ERROR("pwm already initialized",-1);
	}
	if(!clkMan_isInit())
	{
		if(0>clkMan_init())
			return ERROR("can't initialize clkMan",-1);
	}
	baseAddrVirt = mapPhysMemArea(PWM_BASE_ADDR_PHYS,PWM_AREA_LEN);
	if(!baseAddrVirt)
		return ERROR("Can't map pwm peripheral",-2);
	//DBG_PRINTF("pwm baseAddrVirt = 0x%x\n",baseAddrVirt);
	pwm_setClock(CLK_SRC_OSCILLATOR,32,0);//600KHz
	return 0;
}

/**
 * @brief true if pwm_init has been called with success
 */
int pwm_isInit()
{
	return baseAddrVirt!=0;
}
/**
 * @brief sets the clock divisor for the pwm (the base frequency is 19.2 MHz)
 *
 * @param clkSrc		Clock source, see the macros in clockManager. Normally you want CLK_SRC_OSCILLATOR (19.2MHz)
 * @param divisorInt	Integer part of the divisor
 * @param divisorFrac	Fractional part of the divisor
 */
int pwm_setClock(int clkSrc, int divisorInt, int divisorFrac)
{
	if(!baseAddrVirt)
			return ERROR("PWM interface not initialized",-1);


	pwm_setEnable(1,0);
	pwm_setEnable(2,0);
	usleep(110);
	if(0>clkMan_setClock_pwm(clkSrc,divisorInt,divisorFrac))
		return ERROR("can't set the clock for the pwm",-2);
	pwm_setEnable(1,1);
	pwm_setEnable(2,1);
}


/**
 * @brief	Resets the specified channel to: disabled, pwm mode, don't repeat last, silence=0, normal polarity, don't use fifo, don't use M/S
 *		Warning: the pwm peripheral likes to ignore the CTL_RPTL1_MASK and repeat the last word even if you say it not to.
 *
 * @param channel	The channel
 *
 * @return negative in case of error
 */
int pwm_resetChannel(int channel)
{
	if(!baseAddrVirt)
		return ERROR("PWM interface not initialized",-1);

	volatile uint32_t* reg = (volatile uint32_t*)(baseAddrVirt + PWM_REG_CTL_OFF);
	switch(channel)
	{
		case 1:
			*reg = (*reg & ~(CTL_PWEN1_MASK | CTL_MODE1_MASK | CTL_RPTL1_MASK | CTL_SBIT1_MASK | CTL_POLA1_MASK | CTL_USEF1_MASK | CTL_MSEN1_MASK));
			break;
		case 2:
			*reg = (*reg & ~(CTL_PWEN2_MASK | CTL_MODE2_MASK | CTL_RPTL2_MASK | CTL_SBIT2_MASK | CTL_POLA2_MASK | CTL_USEF2_MASK | CTL_MSEN2_MASK));
			break;
		default:
			return ERROR("invalid channel",-2);
	}
	//DBG_PRINTF("PWM CTL set to 0x%x\n",*reg);
	return 0;
}

/**
 * @brief	Sets the repetition of the last word in the fifo on serializer mode
 *		Warning: the pwm peripheral likes to ignore this setting and repeat the last word even if you say it not to.
 *
 * @param channel	The channel to configure
 * @param value	0=don't repeat, 1=repeat
 *
 * @return	negative in case of error
 */
int pwm_setRepeatLastWord(int channel, unsigned int value)
{
	if(!baseAddrVirt)
		return ERROR("PWM interface not initialized",-1);

	value = (value)?~0:0;

	volatile uint32_t* reg = (volatile uint32_t*)(baseAddrVirt + PWM_REG_CTL_OFF);
	switch(channel)
	{
		case 1:
			*reg = (*reg & ~CTL_RPTL1_MASK) | (value & CTL_RPTL1_MASK);
			break;
		case 2:
			*reg = (*reg & ~CTL_RPTL2_MASK) | (value & CTL_RPTL2_MASK);
			break;
		default:
			return ERROR("invalid channel",-2);
	}
	return 0;
}


/**
 * @brief clears the fifo
 *
 * @return negative in case of error
 */
int pwm_clearFifo()
{
	if(!baseAddrVirt)
		return ERROR("PWM interface not initialized",-1);

	volatile uint32_t* reg = (volatile uint32_t*)(baseAddrVirt + PWM_REG_CTL_OFF);
	*reg = *reg | CTL_CLRF_MASK;
	return 0;
}

/**
 * @brief Sets the enabled status of the specified channel. After the dma completes a transfer (a chain of control blocks) and 0 gets loaded as control block this is resetted to 0.
 *
 * @param	channel	The channel: 1 or 2
 * @param	value		It's a boolean: 0=disabled, 1=enabled
 *
 * @return negative in case of error
 */
int pwm_setEnable(int channel,unsigned int value)
{
	if(!baseAddrVirt)
		return ERROR("PWM interface not initialized",-1);

	value = (value)?~0:0;

	volatile uint32_t* reg = (volatile uint32_t*)(baseAddrVirt + PWM_REG_CTL_OFF);
	switch(channel)
	{
		case 1:
			*reg = (*reg & ~CTL_PWEN1_MASK) | (value & CTL_PWEN1_MASK);
			break;
		case 2:
			*reg = (*reg & ~CTL_PWEN2_MASK) | (value & CTL_PWEN2_MASK);
			break;
		default:
			return ERROR("invalid channel",-2);
	}
	return 0;
}


/**
 * @brief Sets the mode of the specified channel to PWM or Serializer
 *
 * @param channel	The channel: 1 or 2
 * @param mode	The mode: 0=PWM, 1=Serializer
 *
 * @return negative in case of error
 */
int pwm_setMode(int channel,unsigned int mode)
{
	if(!baseAddrVirt)
		return ERROR("PWM interface not initialized",-1);

	mode = (mode)?~0:0;

	volatile uint32_t* reg = (volatile uint32_t*)(baseAddrVirt + PWM_REG_CTL_OFF);
	switch(channel)
	{
		case 1:
			*reg = (*reg & ~CTL_MODE1_MASK) | (mode & CTL_MODE1_MASK);
			break;
		case 2:
			*reg = (*reg & ~CTL_MODE2_MASK) | (mode & CTL_MODE2_MASK);
			break;
		default:
			return ERROR("invalid channel",-2);
	}
	return 0;
}


/**
 * @brief Set the serialized mode to use the fifo or the data register
 *
 * @param channel	The channel: 1 or 2
 * @param mode	The mode: 0=use data register, 1=use fifo
 *
 * @return negative in case of error
 */
int pwm_setSerializerMode(int channel,unsigned int mode)
{
	if(!baseAddrVirt)
		return ERROR("PWM interface not initialized",-1);

	mode = (mode)?~0:0;

	volatile uint32_t* reg = (volatile uint32_t*)(baseAddrVirt + PWM_REG_CTL_OFF);
	switch(channel)
	{
		case 1:
			*reg = (*reg & ~CTL_USEF1_MASK) | (mode & CTL_USEF1_MASK);
			break;
		case 2:
			*reg = (*reg & ~CTL_USEF2_MASK) | (mode & CTL_USEF2_MASK);
			break;
		default:
			return ERROR("invalid channel",-2);
	}
	return 0;
}


/**
 * @brief Sets the pwm mode: M/S or the algorithm (see datasheet pag 139)
 *
 * @param channel	The channel: 1 or 2
 * @param mode	The mode: 0=algorithm, 1=M/S
 *
 * @return negative in case of error
 */
int pwm_setMSenable(int channel, unsigned int mode)
{
	if(!baseAddrVirt)
		return ERROR("PWM interface not initialized",-1);

	mode = (mode)?~0:0;

	volatile uint32_t* reg = (volatile uint32_t*)(baseAddrVirt + PWM_REG_CTL_OFF);
	switch(channel)
	{
		case 1:
			*reg = (*reg & ~CTL_MSEN1_MASK) | (mode & CTL_MSEN1_MASK);
			break;
		case 2:
			*reg = (*reg & ~CTL_MSEN2_MASK) | (mode & CTL_MSEN2_MASK);
			break;
		default:
			return ERROR("invalid channel",-2);
	}
	return 0;
}


/**
 * @brief Sets the range of the specified channel counter
 *
 * @param channel	The channel: 1 or 2
 * @param range	Range, it's the number of possible pulses in a period.
 *
 * @return negative in case of error
 */
int pwm_setRange(int channel, uint32_t range)
{
	if(!baseAddrVirt)
		return ERROR("PWM interface not initialized",-1);

	volatile uint32_t* reg;
	switch(channel)
	{
		case 1:
			reg = (volatile uint32_t*)(baseAddrVirt + PWM_REG_RNG1_OFF);
			break;
		case 2:
			reg = (volatile uint32_t*)(baseAddrVirt + PWM_REG_RNG2_OFF);
			break;
		default:
			return ERROR("invalid channel",-2);
	}

	*reg = range;

	return 0;
}



/**
 * @brief Sets the data for the specified channel: in Serializer mode (if not using fifo) it will be sent as pulses, in PWM mode it's the number of pulses in a period
 *
 * @param channel	The channel: 1 or 2
 * @param data	The data value
 *
 * @return negative in case of error
 */
int pwm_setData(int channel, uint32_t data)
{
	if(!baseAddrVirt)
		return ERROR("PWM interface not initialized",-1);

	volatile uint32_t* reg;
	switch(channel)
	{
		case 1:
			reg = (volatile uint32_t*)(baseAddrVirt + PWM_REG_DAT1_OFF);
			break;
		case 2:
			reg = (volatile uint32_t*)(baseAddrVirt + PWM_REG_DAT2_OFF);
			break;
		default:
			return ERROR("invalid channel",-2);
	}

	*reg = data;

	return 0;
}


/**
 * @brief Write the provided data word to the fifo
 *
 * @param data	The data
 *
 * @return negative in case of error
 */
int pwm_writeToFifo(unsigned int data)
{
	if(!baseAddrVirt)
		return ERROR("PWM interface not initialized",-1);

	volatile uint32_t* reg = (volatile uint32_t*)(baseAddrVirt + PWM_REG_FIF1_OFF);
	*reg = data;
	return 0;
}


/**
 * @brief Sets the threshold under which the dreq signal is raised
 *
 * @param threshold	The threshold
 *
 * @return negative in case of error
 */
int pwm_setDreqThreshold(unsigned int threshold)
{
	if(!baseAddrVirt)
		return ERROR("PWM interface not initialized",-1);

	volatile uint32_t* reg = (volatile uint32_t*)(baseAddrVirt + PWM_REG_DMAC_OFF);
	*reg = (*reg & ~DMAC_DREQ_MASK) | (threshold & DMAC_DREQ_MASK);
	return 0;
}


/**
 * @brief	Enables or disables the generation of the dma signals DREQ and PANIC
 *
 * @param	value		0=disabled, any other value=enabled
 */
int pwm_setDmaEnable(unsigned int value)
{
	if(!baseAddrVirt)
		return ERROR("PWM interface not initialized",-1);

	value = value? ~0:0;

	volatile uint32_t* reg = (volatile uint32_t*)(baseAddrVirt + PWM_REG_DMAC_OFF);
	*reg = (*reg & ~DMAC_ENAB_MASK) | (value & DMAC_ENAB_MASK);
	//DBG_PRINTF("dmac = 0x%x\n",*reg);
	return 0;
}




