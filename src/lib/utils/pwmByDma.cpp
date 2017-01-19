
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <memoryManagement/cacheCoherentMemoryProvider.h>
#include <peripherals/dma.h>
#include <peripherals/pwm.h>
#include <peripherals/clockManager.h>
#include <peripherals/gpio.h>
#include <CCRCodingUtils/include/errorManagement.h>
#include <memoryManagement/addressing.h>
#include <CCRCodingUtils/include/utils.h>
#include <utils/pwmByDma.h>
#include "../peripherals/gpio_internal.h"
#include "../peripherals/pwm_internal.h"

/**
 * @file pwmByDma.cpp
 *
 * @brief Generates pwm outputs on the gpio pins using the dma engine.
 *
 * It works by writing periodically to the gpio set and clear registers.
 * Precise timing is achieved using the DREQ signal from the pwm: the dma on each cycle writes to the pwm fifo an so is blocked until the pwm consumes the data.
 * So on each cycle:
 *	- DMA writes to the gpio set register
 *	- DMA writes to the gpio clear register
 *	- DMA writes to the pwm FIFO input register
 * When the pwm consumes the data it raises DREQ and the next dma cycle starts
 *
 * To do so we have to allocate:
 *	- A chain of DMA control blocks
 *	- One word to write to the gpio set register for each control block
 *	- One word to write to the gpio clear register for each control block
 *  To the pwm FIFO we can write anything, for example the gpio set word.
 *
 * If you haven't already, see pwmByDma.h
 */


/**
 * @brief Describes the setup of a channel
*/
typedef struct channelSetup_struct
{
	/**
	 * This data area contains first the control blocks (in order), then the words to be copied to the set/clear registers of the gpio, set/clear words are alternated (eg: set0,clear0,set1,clear1,set2,clear2,...)
	 * So it can be divided in these memory regions:
	 * 0 							- pulsesPerCycle*sizeof(ControlBlock)*3	:	ControlBlocks
	 * pulsesPerCycle*sizeof(ControlBlock)	- pulsesPerCycle*32*2				:	Set/Clear words
	 *
	 * set this field to zero when resetting the channel to mark it as not initialized
	 */
	Ccmb_desc		dataArea;
	/**
	 * number of pulses in a period
	 */
	unsigned int	pulsesPerCycle;
} ChannelSetup;

///Array describing the setup of the 14 channels
static ChannelSetup	channelsInfo[14];

static int			isInitialized = 0;
static unsigned int	configured_pulseDuration_us=0;

/**
 * @brief Initialize: call this before any other function in this file. This sets up the pwm to use it as a timer, so don't mess with it after calling this function.
 *
 * @param pulseDuration_us The duration of a single pulse in microseconds, in other words: the granularity, the precision. All prime numbers over 4095 and all the numbers over 33546 aren't acceptable.
 */
int pwmDma_init(unsigned int pulseDuration_us)
{
	if(!pwm_isInit())
	{
		if(0>pwm_init())
			return ERROR("can't initialize pwm",-1);
	}
	if(!dma_isInit())
	{
		if(0>dma_init())
			return ERROR("can't initialize dma",-2);
	}
	if(!gpio_isInit())
	{
		if(0>gpio_init())
			return ERROR("can't initialize gpio",-3);
	}

	//Prepare the pwm

	pwm_setEnable(1,0);		//disable the pwm
	uint32_t range;

	//Search a divisor that gives a range between 3 and 4095 (under 3 strange things happen)
	//the possible divisors are the ones that are the products of prime factors of pulseDuration_us*500
	//All the numbers under 4096 are acceptable
	//All the prime numbers over 4095 are unacceptable
	//All the numbers over 33546 are unacceptable
	//The acceptable numbers could be extended using the fractional part of the divisor
	unsigned int divisor = 49;//minimum divisor 50
	unsigned int remainder;
	do
	{
		divisor++;
		range = pulseDuration_us*500/divisor;
		remainder = pulseDuration_us*500%divisor;
		if(divisor>4095)
			return ERROR("Can't set this period avoid numbers over 33546 and prime numers over 4095",-4);
	}while(range>4095 || remainder!=0);


	configured_pulseDuration_us = range*divisor/500;//should be equal to pulseDuration_us

	DBG_PRINTF("divisor = %u\nrange = %u\nconfigured_pulseDuration_us = %u\n",divisor,range,configured_pulseDuration_us);

      pwm_setClock(CLK_SRC_PLLD,divisor,0);	//set the clock divisor
      pwm_setRange(1,range);				//number of bits to send, this way I can multiply the period
								//So the period is divisor*range microseconds
      pwm_setMode(1,1);			//serializer mode
      pwm_setSerializerMode(1,1);	//use the fifo
      pwm_setRepeatLastWord(1,1);	//the peripheral sometimes does this anyway, at least this way we know for sure
      pwm_setDreqThreshold(1);	//throw dreq when there's less than 1 word in the fifo
      pwm_setDmaEnable(1);		//enable the dma signals, dreq and panic
	pwm_clearFifo();			//clears the fifo
	pwm_setEnable(1,1);		//enable the pwm

	memset(channelsInfo,0,14*sizeof(ChannelSetup));
	isInitialized=1;
}

/**
 * @brief Tells if pwmByDma has been initialized successfully
 */
int pwmDma_isInit()
{
	return isInitialized;
}

/**
 * @brief Returns the duration in microseconds of a single pulse, you can set this duration in pwmDma_init(...)
 *
 * @return The duration, zero if pwmDma_init hasn't been called
 */
unsigned int pwmDma_get_pulseDuration_us()
{
	return configured_pulseDuration_us;
}

/**
 * @brief Tells if a channel is initialized
 *
 * @param channel The channel number
 *
 * @return If the channel has been initialized, 0 in case of error
 */
int pwmDma_isChannelInitialized(int channel)
{
	if(!isInitialized)
		return ERROR("pwmDma isn't initialized",0);
	if(channel<0 || channel>14)
		return ERROR("Invalid channel",0);
	return channelsInfo[channel].dataArea.virt_address==0;
}

/**
 * @brief Return the duration in microseconds of a cycle for the specified channel
 *
 * @param channel The channel number
 *
 * @return the duration, 0 in case of error
 */
int pwmDma_getChannelPeriodUs(int channel)
{
	if(!isInitialized)
		return ERROR("pwmDma isn't initialized",-1);
	if(channel<0 || channel>14)
		return ERROR("Invalid channel",-2);
	return channelsInfo[channel].pulsesPerCycle*configured_pulseDuration_us;
}

/**
 * @brief Gets the control block's array address
 *
 * @return Pointer to the first control block
 */
static ControlBlock* getControlBlockArray(ChannelSetup* channelSetup)
{
	return (ControlBlock*) channelSetup->dataArea.virt_address;
}

/**
 * @brief Gets the set/clear words array address
 *
 * @return Pointer to the first word
 */
static uint32_t* getWordArray(ChannelSetup* channelSetup)
{
	return (uint32_t*) (channelSetup->dataArea.virt_address + sizeof(ControlBlock)*channelSetup->pulsesPerCycle);//32 is the size of a control block
}

//The one channel at a time limitation could be extended to two channels adding PCM support, this is why the other functions ask for the channel number
//This is because if you have two channel they will steal the dreq signal each other
/**
 * @brief Sets up the channel to output pwm, you can use only a channel at a time
 *
 * @param channel			The channel to use
 * @param pulsesPerPeriod	Number of pulses in a cycle. The resulting period duration is: period_us = pulsesPerPeriod*pulseDuration_us. Where pulseDuration_us is specified in pwmDma_init(...)
 *
 * @return negative in case of error
 */
int pwmDma_initChannel_periodInPulses(int channel, unsigned int pulsesPerPeriod)
{
	if(!isInitialized)
		return ERROR("pwmDma isn't initialized",-1);
	if(channel<0 || channel>14)
		return ERROR("Invalid channel",-2);
	if(!(dma_getUsableChannels() & (1<<channel)))
		return ERROR("Channel used by system",-3);
	if(channelsInfo[channel].dataArea.virt_address!=0)
		return ERROR("Channel already initialized",-4);

	//checks if there are already initialized channels
	for(int i=0;i<15;i++)
	{
		if(channelsInfo[channel].dataArea.virt_address!=0)
			return ERROR("You can use only one channel at a time",-5);
	}
	ChannelSetup* cs = &(channelsInfo[channel]);

	cs->pulsesPerCycle = pulsesPerPeriod;
	int r = ccmp_malloc(pulsesPerPeriod*(sizeof(ControlBlock)*3 + 4*2)+4*10,&(cs->dataArea),32);
	if(r<0)
		return ERROR("Can't allocate memory",-6);

	//DBG_PRINTF("cs->dataArea.bus_address = 0x%x\n",cs->dataArea.bus_address);

	//this dummy data is useful for debugging
	uint32_t*		dummy			= (uint32_t*)(cs->dataArea.virt_address + pulsesPerPeriod*(sizeof(ControlBlock)*3 + 4*2));
	*dummy					= 0x03000000;//the top 8 bits
	*(dummy+1)					= 0x0c000000;//the top 8 bits
	*(dummy+2)					= 0x0f000000;//the top 8 bits
	*(dummy+3)					= 0x30000000;//the top 8 bits
	*(dummy+4)					= 0x33000000;//the top 8 bits
	*(dummy+5)					= 0x3c000000;//the top 8 bits
	*(dummy+6)					= 0x3f000000;//the top 8 bits
	*(dummy+7)					= 0xc0000000;//the top 8 bits
	*(dummy+8)					= 0xc3000000;//the top 8 bits
	*(dummy+9)					= 0xcf000000;//the top 8 bits
	ControlBlock*	controlBlock0	= (ControlBlock*)(cs->dataArea.virt_address);
	uint32_t*		setWord0		= (uint32_t*)((uintptr_t)controlBlock0 + 	sizeof(ControlBlock)*pulsesPerPeriod*3);


	//Fill the control blocks area
	ControlBlock*	cb		= controlBlock0;
	uintptr_t	setWord_busAddr	= ccmp_virtAddrToBusAddr(&(cs->dataArea), setWord0);
	uintptr_t	clrWord_busAddr	= ccmp_virtAddrToBusAddr(&(cs->dataArea), setWord0+1);
	uintptr_t	nextCb_busAddr		= ccmp_virtAddrToBusAddr(&(cs->dataArea),(void*)(cb+1));
	uintptr_t	dummy_busAddr	= ccmp_virtAddrToBusAddr(&(cs->dataArea), dummy);
	//DBG_PRINTF("dummy_busAddr = 0x%x\n",dummy_busAddr);

	for(int p=0;p<pulsesPerPeriod;p++)
	{
		//set control block
	/*	dma_writeControlBlock(	cb,			//virtual address of the control block
						setWord_busAddr,	//bus address of the source address
						PERIPHERAL_PHYS_TO_BUS_ADDR(GPSET0_ADDR_PHYS),//bus address of the destination address
						4,			//length of the transfer in bytes
						nextCb_busAddr,	//bus address of the next control block
						TI_SRC_INC_MASK | TI_DEST_INC_MASK | TI_DEST_DREQ_MASK | BLD_BITFIELD(DREQ_PWM,TI_PERMAP_OFF,TI_PERMAP_MASK) | TI_WAIT_RESP_MASK | TI_NO_WIDE_BURSTS_MASK | TI_2DMODE_MASK,//Transfer info
						(REG_2D_STRIDE_DST_STRIDE_MASK & (0xc<<REG_2D_STRIDE_DST_STRIDE_OFF)) | 4,
						4,
						2);		//infos for the 2Dstride mode
		cb++;*/
		//set control block
		dma_writeControlBlock(	cb,			//virtual address of the control block
						setWord_busAddr,	//bus address of the source address
						PERIPHERAL_PHYS_TO_BUS_ADDR(GPSET0_ADDR_PHYS),//bus address of the destination address
						4,			//length of the transfer in bytes
						nextCb_busAddr,	//bus address of the next control block
						TI_SRC_INC_MASK | TI_DEST_INC_MASK | /*TI_DEST_DREQ_MASK | BLD_BITFIELD(DREQ_PWM,TI_PERMAP_OFF,TI_PERMAP_MASK) |*/ TI_WAIT_RESP_MASK | TI_NO_WIDE_BURSTS_MASK,//Transfer info
						0,0,0);		//infos for the 2Dstride mode
		cb++;
		nextCb_busAddr += sizeof(ControlBlock);
		//clear control block
		dma_writeControlBlock(	cb,			//virtual address of the control block
						clrWord_busAddr,	//bus address of the source address
						PERIPHERAL_PHYS_TO_BUS_ADDR(GPCLEAR0_ADDR_PHYS),//bus address of the destination address
						4,			//length of the transfer in bytes
						nextCb_busAddr,	//bus address of the next control block
						TI_SRC_INC_MASK | TI_DEST_INC_MASK | /*TI_DEST_DREQ_MASK | BLD_BITFIELD(DREQ_PWM,TI_PERMAP_OFF,TI_PERMAP_MASK) |*/ TI_WAIT_RESP_MASK | TI_NO_WIDE_BURSTS_MASK,//Transfer info
						0,0,0);		//infos for the 2Dstride mode
		cb++;
		if(p+1==pulsesPerPeriod)//if this is the last pulse
			nextCb_busAddr = ccmp_virtAddrToBusAddr(&(cs->dataArea),(void*)controlBlock0);//this is the last control block, link it to the first
		else
			nextCb_busAddr += sizeof(ControlBlock);
		//fifo control block
		dma_writeControlBlock(	cb,			//virtual address of the control block
						dummy_busAddr + 4*(p%10),	//dummy data
						PERIPHERAL_PHYS_TO_BUS_ADDR(PWM_REG_FIF1_ADDR_PHYS),//bus address of the destination address
						4,			//length of the transfer in bytes
						nextCb_busAddr,	//bus address of the next control block
						TI_SRC_INC_MASK | TI_DEST_INC_MASK | TI_DEST_DREQ_MASK | BLD_BITFIELD(DREQ_PWM,TI_PERMAP_OFF,TI_PERMAP_MASK) | TI_WAIT_RESP_MASK | TI_NO_WIDE_BURSTS_MASK,//Transfer info, potrebbe mettere anche TI_SRC_IGNORE_MASK
						0,0,0);		//infos for the 2Dstride mode
		cb++;
		nextCb_busAddr += sizeof(ControlBlock);
		setWord_busAddr += 8;//32bit
		clrWord_busAddr += 8;//32bit
	}
	memset(setWord0,0,4*2*pulsesPerPeriod);//fill all the words to zero
	gpio_setOutputAll(0, 0xffffffffffffffff);//clear all the gpios

	dma_channelResetInitDefault(channel);
	dma_setControlBlockAddr(channel,cs->dataArea.bus_address);
	pwmDma_startChannel(channel);
//	dma_printRegisters(channel);
}


/**
 * @brief Sets up the channel to output pwm
 *
 * @param channel			The channel to use
 * @param period_us		Duration of a cycle in microseconds
 *
 * @return negative in case of error
 */
int pwmDma_initChannel(int channel, unsigned int period_us)
{
	if(period_us%configured_pulseDuration_us!=0)
		return ERROR("Invalid period_us, not a multiple of pulseDuration",-1);
	return 10*pwmDma_initChannel_periodInPulses(channel,period_us/configured_pulseDuration_us);
}


/**
 * @brief Frees a channel initialized with pwmDma_initChannel or pwmDma_initChannel_periodInPulses
 *
 * @param channel The channel number
 *
 * @return negative in case of error
 */
int pwmDma_freeChannel(int channel)
{
	if(!isInitialized)
		return ERROR("pwmDma isn't initialized",-1);
	if(channel<0 || channel>14)
		return ERROR("Invalid channel",-2);
	if(channelsInfo[channel].dataArea.virt_address==0)
		return ERROR("Channel isn't initialized",-3);

	pwmDma_stopChannelWaitCycleEnd(channel);

	ChannelSetup* cs = &(channelsInfo[channel]);

	if(0>ccmp_free(&(cs->dataArea)))
		DBG_PRINTF_E("Error freeing cache coherent memory for channel %d\n",channel);

	cs->dataArea.virt_address = 0;//to mark it as free
	return 0;
}

/**
 * @brief Starts a stopped channel
 *
 * @param channel The channel number
 *
 * @return negative in case of error
 */
int pwmDma_startChannel(int channel)
{
	if(!isInitialized)
		return ERROR("pwmDma isn't initialized",-1);
	if(channel<0 || channel>14)
		return ERROR("Invalid channel",-2);
	if(channelsInfo[channel].dataArea.virt_address==0)
		return ERROR("Channel isn't initialized",-3);

	dma_setChannelActive(channel,1);

	return 0;
}

/**
 * @brief Stops channel waiting for the current cycle to finish
 *
 * @param channel The channel number
 *
 * @return negative in case of error
 */
int pwmDma_stopChannelWaitCycleEnd(int channel)
{
	if(!isInitialized)
		return ERROR("pwmDma isn't initialized",-1);
	if(channel<0 || channel>14)
		return ERROR("Invalid channel",-2);
	if(channelsInfo[channel].dataArea.virt_address==0)
		return ERROR("Channel isn't initialized",-3);

	if(!dma_isChannelActive(channel))//if it's already stopped
		return 0;

	ChannelSetup* cs = &(channelsInfo[channel]);
	ControlBlock*	lastControlBlock	= ((ControlBlock*)cs->dataArea.virt_address) + cs->pulsesPerCycle*3;

	lastControlBlock->nextControlBlockAddrPhys = 0;//This way the dma will stop the next time it completes a cycle

	usleep(cs->pulsesPerCycle*configured_pulseDuration_us);//wait one period

	if(dma_isChannelActive(channel))//If it didn't stop we stop it anyway
		DBG_PRINTF_E("Killing dma channel %d\n",channel);

	pwmDma_stopChannel(channel);//disable the channel

	return 0;
}

/**
 * @brief Stops channel
 *
 * @param channel The channel number
 *
 * @return negative in case of error
 */
int pwmDma_stopChannel(int channel)
{
	if(!isInitialized)
		return ERROR("pwmDma isn't initialized",-1);
	if(channel<0 || channel>14)
		return ERROR("Invalid channel",-2);
	if(channelsInfo[channel].dataArea.virt_address==0)
		return ERROR("Channel isn't initialized",-3);

	dma_setChannelActive(channel,0);//disable the channel

	return 0;
}

/**
 * @brief	Sets a pulse on the specified channel and gpio.
 *
 * @param channel 	The channel number
 * @param bcmPort		The port broadcom number, only ports 0-31 are available
 * @param start_us	The start time of the pulse in microseconds, has to be a multiple of the pulse duration, and between 0 and the period duration of this channel
 * @param length_us	The length of the pulse in microseconds, has to be a multiple of the pulse duration
 *
 * @return negative in case of error
 */
int pwmDma_addPulse(int channel, int bcmPort, unsigned int start_us, unsigned int length_us)
{
	//DBG_PRINTF("setting pulse %u : %u\n",start_us,length_us);
	if(!isInitialized)
		return ERROR("pwmDma isn't initialized",-1);
	if(channel<0 || channel>14)
		return ERROR("Invalid channel",-2);
	if(start_us%configured_pulseDuration_us!=0)
		return ERROR("Invalid start_us, not a multiple of pulseDuration",-3);
	if(length_us%configured_pulseDuration_us!=0)
		return ERROR("Invalid length_us, not a multiple of pulseDuration",-4);
	if(channelsInfo[channel].dataArea.virt_address==0)
		return ERROR("Channel isn't initialized",-5);
	if(bcmPort<0 || 32<bcmPort)
		return ERROR("Invalid bcmPort, only ports 0-31 are available",-6);
	ChannelSetup* cs = &(channelsInfo[channel]);
	if(length_us==0)
		return 0;
	unsigned int startPulse = start_us/configured_pulseDuration_us;
	unsigned int pulsesNum = length_us/configured_pulseDuration_us;
	if(pulsesNum>cs->pulsesPerCycle)
		return ERROR("length_us exceeds the period\n",-7);
	unsigned int endPulse = (startPulse + pulsesNum)%cs->pulsesPerCycle;//loops back if it reaches the end of the cycle

	if(startPulse>cs->pulsesPerCycle)
		return ERROR("start_us exceeds the period",-8);

	uint32_t*		setWord0		= (uint32_t*)((uintptr_t)cs->dataArea.virt_address + 	sizeof(ControlBlock)*cs->pulsesPerCycle*3);

	uint32_t*	setWordStart =	setWord0 + startPulse*2;
	uint32_t*	clearWordStart =	setWordStart + 1;
	uint32_t*	setWordEnd =	setWord0 + endPulse*2;
	uint32_t*	clearWordEnd =	setWordEnd + 1;

	//DBG_PRINTF("setWordStart = 0x%x \t setWordEnd = 0x%x\n",setWordStart,setWordEnd);

	//Set the gpio status at the start pulse
	*setWordStart	= *setWordStart | 1<<bcmPort;//turn on
	*clearWordStart	= *clearWordStart & ~(1<<bcmPort);//don't turn off

	//ensure the pulses in the middle don't do anything
	for(int p=startPulse+1;p<startPulse+pulsesNum;p++)
	{

		uint32_t* sw = setWord0 + (p%cs->pulsesPerCycle)*2;//loops back if it reaches the end of the cycle
		uint32_t* cw = sw+1;
		*sw = *sw & ~(1<<bcmPort);//don't turn on
		*cw = *cw & ~(1<<bcmPort);//don't turn off
	}

	//Unset the gpio status at the end pulse
	if(pulsesNum!=cs->pulsesPerCycle)//if it is long as the period then don't turn it off
	{
		*setWordEnd		= *setWordEnd & ~(1<<bcmPort);//don't turn on
		*clearWordEnd	= *clearWordEnd | 1<<bcmPort;//turn off
	}


	return 0;
}

/**
 * @brief Removes any pulse on this port at the specified channel
 *
 * @param channel	The channel
 * @param bcmPort The port
 *
 * @return negative in case of error
 */
int pwmDma_clearPort(int channel, int bcmPort)
{
	if(!isInitialized)
		return ERROR("pwmDma isn't initialized",-1);
	if(channel<0 || channel>14)
		return ERROR("Invalid channel",-2);
	if(channelsInfo[channel].dataArea.virt_address==0)
		return ERROR("Channel isn't initialized",-3);
	if(bcmPort<0 || 32<bcmPort)
		return ERROR("Invalid port, only ports 0-31 are available",-4);

	ChannelSetup* cs = &(channelsInfo[channel]);

	uint32_t*		setWord0		= (uint32_t*)((uintptr_t)cs->dataArea.virt_address + 	sizeof(ControlBlock)*cs->pulsesPerCycle*3);

	DBG_PRINTF("clearing from %d to %d\n",0,cs->pulsesPerCycle);
	for(int p=0; p<(cs->pulsesPerCycle); p++)
	{
		//DBG_PRINTF("clearing %d\n",p);
		uint32_t* sw = setWord0 + p*2;
		uint32_t* cw = sw+1;
		*sw = *sw & ~(1<<bcmPort);//don't turn on
		*cw = *cw & ~(1<<bcmPort);//don't turn off
	}

	if(0>gpio_setOutput(bcmPort,0))
		return ERROR("Couldn't set the port to low",-5);

	return 0;
}
/**
 * @brief For debugging purposes: prints all the set/clear words
 *
 * @return negative in case of error
 */
int pwmDma_printWords(int channel)
{
	if(!isInitialized)
		ERROR("pwmDma isn't initialized",-1);
	if(channel<0 || channel>14)
		ERROR("Invalid channel",-2);
	if(channelsInfo[channel].dataArea.virt_address==0)
		ERROR("Channel isn't initialized",-3);

	uint32_t*		setWord0		= (uint32_t*)((uintptr_t)channelsInfo[channel].dataArea.virt_address + 	sizeof(ControlBlock)*channelsInfo[channel].pulsesPerCycle*3);

	for(int i=0;i<channelsInfo[channel].pulsesPerCycle;i++)
	{
		char s[33];
		char c[33];
		uint32_tToBitString(*(setWord0 +i*2),s);
		uint32_tToBitString(*(setWord0+1 +i*2),c);
		printf("[%d]:\tset=%s\tclear=%s\n",i,s,c);
	}
}


/**
 * @brief For debugging purposes: prints all the control blocks
 *
 * @return negative in case of error
 */
int pwmDma_printControlBlocks(int channel)
{
	if(!isInitialized)
		ERROR("pwmDma isn't initialized",-1);
	if(channel<0 || channel>14)
		ERROR("Invalid channel",-2);
	if(channelsInfo[channel].dataArea.virt_address==0)
		ERROR("Channel isn't initialized",-3);

	ChannelSetup* cs = &(channelsInfo[channel]);

	ControlBlock*	controlBlock0	= (ControlBlock*)(cs->dataArea.virt_address);

	for(int i=0;i<channelsInfo[channel].pulsesPerCycle*3;i++)
	{
		dma_printControlBlock(controlBlock0+i);
	}
}

/**
 * @brief Set the port to output a square wave with the specified duty cycle. Will be high for the first ratio*period microseconds and then low
 *
 * @param channel The channel to use
 * @param bcmPort The port to use
 * @param ratio	The duty cycle ratio
 *
 * @return negative in case of error
 */
int pwmDma_setMSRatio(int channel, int bcmPort, double ratio)
{
	int r;
	r=pwmDma_clearPort(channel, bcmPort);
	if(r<0)
		return ERROR("Error clearing port",10*r-1);
	r=pwmDma_addPulse(channel, bcmPort, 0, channelsInfo[channel].pulsesPerCycle*configured_pulseDuration_us*ratio);
	if(r<0)
		return ERROR("Error clearing port",10*r-2);
}


/**
 * @brief	Adds multiple pulses to the channel at the specified port. The pulse will be repeated every subPeriod_us microseconds.
 *		In this way you can simulate a channel initialized with period=subPeriod_us
 *
 * @param channel 	The channel number
 * @param bcmPort		The port broadcom number, only ports 0-31 are available
 * @param start_us	The start time of the pulse in microseconds, has to be a multiple of the pulse duration, and between 0 and the subPeriod_us
 * @param length_us	The length of the pulse in microseconds, has to be a multiple of the pulse duration and not bigger than subPeriod_us
 * @param subPeriod_us	The period at which the pulse will be repeated. Has to be a divisor of the period of the channel
 */
int pwmDma_addMultiplePulse(int channel, int bcmPort, unsigned int start_us, unsigned int length_us, unsigned int subPeriod_us)
{
	if(!isInitialized)
		return ERROR("pwmDma isn't initialized",-1);
	if(channel<0 || channel>14)
		return ERROR("Invalid channel",-2);
	if(start_us%configured_pulseDuration_us!=0)
		return ERROR("Invalid start_us, not a multiple of pulseDuration",-3);
	if(length_us%configured_pulseDuration_us!=0)
		return ERROR("Invalid length_us, not a multiple of pulseDuration",-4);
	if(channelsInfo[channel].dataArea.virt_address==0)
		return ERROR("Channel isn't initialized",-5);
	if(bcmPort<0 || 32<bcmPort)
		return ERROR("Invalid bcmPort, only ports 0-31 are available",-6);
	if( (channelsInfo[channel].pulsesPerCycle*configured_pulseDuration_us)%subPeriod_us != 0)
		return ERROR("Invalid subPeriod_us: channel's period must be a multiple of subPeriod_us",-7);
	if(start_us>=subPeriod_us)
		return ERROR("Invalid start_us: must be less than subPeriod_us",-8);
	if(length_us>subPeriod_us)
		return ERROR("Invalid start_us: must be less or equal to subPeriod_us",-9);

	if(length_us==subPeriod_us)
		pwmDma_addPulse(channel,bcmPort,0,(channelsInfo[channel].pulsesPerCycle*configured_pulseDuration_us));//always on

	int bigPulsesNum = (channelsInfo[channel].pulsesPerCycle*configured_pulseDuration_us)/subPeriod_us;
	for(int i=0;i<bigPulsesNum;i++)
	{
		pwmDma_addPulse(channel,bcmPort,subPeriod_us*i,length_us);
	}

	return 0;
}
