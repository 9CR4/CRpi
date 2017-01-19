#include <memoryManagement/addressing.h>
#include <memoryManagement/physMemoryManagement.h>
#include <CCRCodingUtils/include/errorManagement.h>
#include <peripherals/clockManager.h>
#include <CCRCodingUtils/include/utils.h>
#include <unistd.h>

/**
 * @file clockManager.cpp
 *
 * @brief 	Interface to the clock manager peripheral.
 *
 * If you haven't already, see clockManager.h
 * You can find the documentation on the BCM2835 at page 105.
 * The documentation about pwm and pcm clocks is missing in the official doc,
 * there's a pdf by G.J Van Loo with the missing info at https://www.scribd.com/doc/127599939/BCM2835-Audio-clocks
 *
 */

///Physical base address of the clock manager peripheral
#define CLOCK_MANAGER_BASE_ADDR_PHYS	(peripheralsBaseAddressPhys + 0x101000)
///Length in bytes of the memory area dedicated to the clock manager
#define CLOCK_MANAGER_AREA_LEN		0x100

///Offset to the general purpose clock 0 control register
#define REG_CM_GP0CTL_OFF (0x70)
///Offset to the general purpose clock 1 control register
#define REG_CM_GP1CTL_OFF (0x78)
///Offset to the general purpose clock 2 control register
#define REG_CM_GP2CTL_OFF (0x80)
///Offset to the PCM clock control register
#define REG_CM_PCMCTL_OFF (0x98)
///Offset to the PWM clock control register
#define REG_CM_PWMCTL_OFF (0xa0)

///Offset to the general purpose clock 0 divisor register
#define REG_CM_GP0DIV_OFF (0x74)
///Offset to the general purpose clock 1 divisor register
#define REG_CM_GP1DIV_OFF (0x7c)
///Offset to the general purpose clock 2 divisor register
#define REG_CM_GP2DIV_OFF (0x84)
///Offset to the PCM clock divisor register
#define REG_CM_PCMDIV_OFF (0x9c)
///Offset to the PWM clock divisor register
#define REG_CM_PWMDIV_OFF (0xa4)

///clock manager password. You must OR this to every write you make to the clock manager registers
#define CM_PASSWORD (0x5a<<24)

///Control register mask: mash filter level
#define CM_CTL_MASH_MASK	(3<<9)
///Control register mask: Invert the clock output
#define CM_CTL_FLIP_MASK	(1<<8)
///Control register mask: Clock generator is running
#define CM_CTL_BUSY_MASK	(1<<7)
///Control register mask: Kill the clock
#define CM_CTL_KILL_MASK	(1<<6)
///Control register mask: Enable the clock generator
#define CM_CTL_ENAB_MASK	(1<<4)
///Control register mask: Clock source
#define CM_CTL_SRC_MASK		(0xf)


///Virtual base address of the clock manager peripheral
static void* baseAddrVirt = 0;


/**
 * @brief Initializes this peripheral interface, to use before any other function in this file
 *
 * @return negative in case of error
 */
int clkMan_init()
{
	if(baseAddrVirt)
	{
		return ERROR("clkMan already initialized",-1);
	}
	baseAddrVirt = mapPhysMemArea(CLOCK_MANAGER_BASE_ADDR_PHYS,CLOCK_MANAGER_AREA_LEN);
	if(!baseAddrVirt)
		return ERROR("Can't map clock manager peripheral",-1);
//	DBG_PRINTF("clkMan baseAddrVirt = 0x%x\n",baseAddrVirt);
	return 0;
}


/**
 * @brief true if clkMan_init has been called with success
 */
int clkMan_isInit()
{
	return baseAddrVirt!=0;
}

static int clkMan_killClock(int controlRegOffset)
{
	volatile uint32_t* pwmClkCntlReg = (volatile uint32_t*)(baseAddrVirt + controlRegOffset);
	DBG_PRINTF_W("Killing clock at offset 0x%x\n",controlRegOffset);
	*pwmClkCntlReg = *pwmClkCntlReg | CM_CTL_KILL_MASK;
	usleep(215);
	*pwmClkCntlReg = *pwmClkCntlReg & ~CM_CTL_KILL_MASK;//Maybe it's useless
}
/**
 * @brief sets the source clock and the divisor on the specified peripheral clock
 *
 * @param controlRegOffset	Offset from the clock peripheral base to the peripheral control register
 * @param controlRegOffset	Offset from the clock peripheral base to the peripheral divisor register
 * @param clockSrc		The desired clock source (use the CLK_SRC_* macros)
 * @param divisorInt		Integer part of the divisor
 * @param divisorFrac		Fractional part of the divisor
 *
 * @return negative in case of error
 */
static int clkMan_setClock(int controlRegOffset, int divisorRegOffset, int clockSrc, int divisorInt, int divisorFrac)
{
	if(controlRegOffset>CLOCK_MANAGER_AREA_LEN)
		return ERROR("invalid control register",-1);
	if(divisorRegOffset>CLOCK_MANAGER_AREA_LEN)
		return ERROR("invalid divisor register",-2);
	if(clockSrc>0xf)//4 bit
		return ERROR("invalid clockSrc",-3);
	if(divisorInt>4095)//12 bit
		return ERROR("invalid divisorInt",-4);
	if(divisorFrac>4095)//12 bit
		return ERROR("invalid divisorFrac",-5);
	if(!baseAddrVirt)
		return ERROR("Clock manager not initialized",-6);

	//DBG_PRINTF("setting clock (controlRegOff=0x%x)\n",controlRegOffset);

	volatile uint32_t* pwmClkCntlReg = (volatile uint32_t*)(baseAddrVirt + controlRegOffset);
	volatile uint32_t* pwmClkDivReg = (volatile uint32_t*)(baseAddrVirt + divisorRegOffset);
	//DBG_PRINTF("divisorInt = %d. CM_PASSWORD | (divisorInt<<12) = 0x%x\n",divisorInt,CM_PASSWORD | (divisorInt<<12));
	//DBG_PRINTF("was *pwmClkCntlReg = 0x%x\n",*pwmClkCntlReg);
	//DBG_PRINTF("was *pwmClkDivReg = 0x%x\n",*pwmClkDivReg);
	//DBG_PRINTF("writing 0x%x\n",CM_PASSWORD | clockSrc);
	*pwmClkCntlReg = CM_PASSWORD | clockSrc;//disables the clock and sets the new source
	//now i have to wait for the clock to stop, I could be using the busy flag only, but the source of wiringpi says strange things may happen
	//DBG_PRINTF("*pwmClkCntlReg = 0x%x\n",*pwmClkCntlReg);
	usleep(110);
	//DBG_PRINTF("*pwmClkCntlReg = 0x%x\n",*pwmClkCntlReg);
	uint64_t t0 = getTimeNanoMonotonic();
	int killed = 0;
	while(*pwmClkCntlReg & CM_CTL_BUSY_MASK)//I wait for busy to clear
	{
		usleep(1);
		if(killed)
		{
			return ERROR("Can't stop clock",-7);
		}
		if(getTimeNanoMonotonic() - t0 > 215000)//215us should be the max period possible using the oscillator clock, other clocks are faster
		{
			clkMan_killClock(controlRegOffset);
			killed++;
		}
	}

	*pwmClkDivReg = CM_PASSWORD | (divisorInt<<12) | divisorFrac;

	*pwmClkCntlReg = CM_PASSWORD | clockSrc | CM_CTL_ENAB_MASK;

	//DBG_PRINTF("*pwmClkCntlReg = 0x%x\n",*pwmClkCntlReg);
	//DBG_PRINTF("*pwmClkDivReg = 0x%x\n",*pwmClkDivReg);
	//DBG_PRINTF("clock set\n",controlRegOffset);

	return 0;
}


/**
 * @brief sets the clock for the general purpouse clock 0
 *
 * @param clockSrc	The desired clock source (use the CLK_SRC_* macros)
 * @param divisorInt	Integer part of the divisor
 * @param divisorFrac	Fractional part of the divisor
 *
 * @return negative in case of error
 */
int clkMan_setClock_gp0(int clockSrc, int divisorInt, int divisorFrac)
{
	return clkMan_setClock(REG_CM_GP0CTL_OFF,REG_CM_GP0DIV_OFF, clockSrc, divisorInt,divisorFrac);
}

/**
 * @brief sets the clock for the general purpouse clock 1
 *
 * @param clockSrc	The desired clock source (use the CLK_SRC_* macros)
 * @param divisorInt	Integer part of the divisor
 * @param divisorFrac	Fractional part of the divisor
 *
 * @return negative in case of error
 */
int clkMan_setClock_gp1(int clockSrc, int divisorInt, int divisorFrac)
{
	return clkMan_setClock(REG_CM_GP1CTL_OFF,REG_CM_GP1DIV_OFF, clockSrc, divisorInt,divisorFrac);
}

/**
 * @brief sets the clock for the general purpouse clock 2
 *
 * @param clockSrc	The desired clock source (use the CLK_SRC_* macros)
 * @param divisorInt	Integer part of the divisor
 * @param divisorFrac	Fractional part of the divisor
 *
 * @return negative in case of error
 */
int clkMan_setClock_gp2(int clockSrc, int divisorInt, int divisorFrac)
{
	return clkMan_setClock(REG_CM_GP2CTL_OFF,REG_CM_GP2DIV_OFF, clockSrc, divisorInt,divisorFrac);
}

/**
 * @brief sets the clock for the pcm peripheral
 *
 * @param clockSrc	The desired clock source (use the CLK_SRC_* macros)
 * @param divisorInt	Integer part of the divisor
 * @param divisorFrac	Fractional part of the divisor
 *
 * @return negative in case of error
 */
int clkMan_setClock_pcm(int clockSrc, int divisorInt, int divisorFrac)
{
	return clkMan_setClock(REG_CM_PCMCTL_OFF,REG_CM_PCMDIV_OFF, clockSrc, divisorInt,divisorFrac);
}

/**
 * @brief sets the clock for the pwm peripheral
 *
 * @param clockSrc	The desired clock source (use the CLK_SRC_* macros)
 * @param divisorInt	Integer part of the divisor
 * @param divisorFrac	Fractional part of the divisor
 *
 * @return negative in case of error
 */
int clkMan_setClock_pwm(int clockSrc, int divisorInt, int divisorFrac)
{
	return clkMan_setClock(REG_CM_PWMCTL_OFF,REG_CM_PWMDIV_OFF, clockSrc, divisorInt,divisorFrac);
}





