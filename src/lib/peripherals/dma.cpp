/**
 * @file dma.cpp
 *
 * @brief	Interface to the dma peripheral
 *
 * If you haven't already, see dma.h
 * See documentation at "BCM2835 ARM peripherals" page 38
 */

#include <memoryManagement/addressing.h>
#include <CCRCodingUtils/include/errorManagement.h>
#include <CCRCodingUtils/include/utils.h>
#include <memoryManagement/physMemoryManagement.h>
#include <memoryManagement/cacheCoherentMemoryProvider.h>
#include <string.h>
#include <stdlib.h>
#include <peripherals/dma.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>//open




//We exclude channel 15
///The base physical address of the dma peripheral
#define DMA_BASE_ADDR_PHYS	(peripheralsBaseAddressPhys + 0x7000)
///Returns the offset from DMA_BASE_ADDR_PHYS of the register sets of the different channels
#define CHANNEL_REGISTER_SET_OFFSET(ch)	(0x100*ch)
///The size of the memory area dedicated to the dma peripheral
#define DMA_AREA_LEN		0xf00

//Registers offsets (from the register's set base)
///Offset from the set base address of the Control and Status register
#define REG_CONTROL_STATUS_OFF	0
///Offset from the set base address of the Control Block Address register (must be 32-byte aligned)
#define REG_CONTROL_BLOCK_ADDR_OFF	0x4
///Offset from the set base address of the Transefer Info register (writable only through control block)
#define REG_TRANSFER_INFO_OFF		0x8
///Offset from the set base address of the Source Address register (writable only through control block)
#define REG_SOURCE_ADDR_OFF		0xC
///Offset from the set base address of the Destination Address register (writable only through control block)
#define REG_DEST_ADDR_OFF		0x10
///Offset from the set base address of the Transfer Length register (writable only through control block)
#define REG_TRANSFER_LENGTH_OFF	0x14
///Offset from the set base address of the 2D Stride register (writable only through control block)
#define REG_2D_STRIDE_MODE_OFF	0x18
///Offset from the set base address of the Next Contorl Block Address register (writable only through control block)
#define REG_NEXT_BLOCK_ADDR_OFF	0x1C
///Offset from the set base address of the Debug register (writable only through control block)
#define REG_DEBUG_OFF			0x20

///Offset from the dma base address of the global interrupt status register
#define REG_GLOBAL_INT_STATUS_OFF	0xfe0
///Offset from the dma base address of the global enable register
#define REG_GLOBAL_ENABLE_OFF		0xff0

//:::Masks for the Control and Status Register:::::::::::::::::::::::
///Write 1 to reset the channel
#define CNTRL_STTS_RESET_MASK					(1<<31)

///Write 1 to abort the current Control Block. Will continue from the next
#define CNTRL_STTS_ABORT_MASK					(1<<30)

///Set to 1 to disable stopping on debug pause signal
#define CNTRL_STTS_DISDEBUG_MASK				(1<<29)

///If set will wait for write responses
#define CNTRL_STTS_WAIT_FOR_OUTSTANDING_WRITES_MASK	(1<<28)

//@{
///Sets the priority of panicking AXI bus transactions
#define CNTRL_STTS_PANIC_PRIORITY_OFF			(20)
#define CNTRL_STTS_PANIC_PRIORITY_MASK			(0xf<<CNTRL_STTS_PANIC_PRIORITY_OFF)
//@}
//@{
///Sets the priority of normal AXI bus transactions
#define CNTRL_STTS_PRIORITY_OFF				(16)
#define CNTRL_STTS_PRIORITY_MASK				(0xf<<CNTRL_STTS_PRIORITY_OFF)
//@}
///Indicates if DMA detected an error, see more in the DEBUG register
#define CNTRL_STTS_ERROR_MASK					(1<8)

///Indicates DMA is waiting for outstanding writes and isn't writing
#define CNTRL_STTS_WAITING_FOR_OUTSTANDING_WRITES_MASK (1<6)

///Indicates DMA is stopped due to the DREQ being active
#define CNTRL_STTS_DREQ_STOPS_DMA_MASK			(1<5)

///Indicates the DMA is paused and not transferring data
#define CNTRL_STTS_PAUSED_MASK				(1<4)

///Indicates Tte status of the DREQ signal selected by PERMAP in the transfer info
#define CNTRL_STTS_DREQ_MASK					(1<3)

///Set when INTEN==1 and the transfer has ended, must be cleared (writing 1)
#define CNTRL_STTS_INT_MASK					(1<2)

///Set when the transfer of the current control block is complete
#define CNTRL_STTS_END_MASK					(1<1)

///Activates or deactivates the DMA. Autmatically set to 0 when it loads 0 from nextcontrolblock
#define CNTRL_STTS_ACTIVE_MASK				(1)






//:::Masks for the Debug Register:::::::::::::::::::::::
///Mask for the DMA Lite bit in the debug register
#define DBG_LITE_MASK				(1<<28)
///Mask for the DMA Version in the debug register
#define DBG_VERSION_MASK			(7<<25)
///Mask for the DMA State in the debug register
#define DBG_DMA_STATE_MASK			(511<<16)
///Mask for the DMA Axi id in the debug register
#define DBG_DMA_ID_MASK				(255<<8)
///Mask for the DMA outstanding writes counter in the debug register
#define DBG_OUTSTANDING_WRITES_MASK		(15<<4)
///Mask for the DMA read error bit in the debug register
#define DBG_READ_ERROR_MASK			(1<<2)
///Mask for the DMA FIFO error bit in the debug register
#define DBG_FIFO_ERROR_MASK			(1<<1)
///Mask for the DMA read last not set error bit in the debug register
#define DBG_READ_LAST_NOT_SET_ERROR_MASK	(0)

//:::Masks for the Interrupt status Register:::::::::::::::::::::::
///Offset of the interrupt status bit for channel 15 in the global interrupt status register
#define INT_STTS_INT15_OFF			(1<<15)
///Offset of the interrupt status bit for channel 14 in the global interrupt status register
#define INT_STTS_INT14_OFF			(1<<14)
///Offset of the interrupt status bit for channel 13 in the global interrupt status register
#define INT_STTS_INT13_OFF			(1<<13)
///Offset of the interrupt status bit for channel 12 in the global interrupt status register
#define INT_STTS_INT12_OFF			(1<<12)
///Offset of the interrupt status bit for channel 11 in the global interrupt status register
#define INT_STTS_INT11_OFF			(1<<11)
///Offset of the interrupt status bit for channel 10 in the global interrupt status register
#define INT_STTS_INT10_OFF			(1<<10)
///Offset of the interrupt status bit for channel 9 in the global interrupt status register
#define INT_STTS_INT9_OFF			(1<<9)
///Offset of the interrupt status bit for channel 8 in the global interrupt status register
#define INT_STTS_INT8_OFF			(1<<8)
///Offset of the interrupt status bit for channel 7 in the global interrupt status register
#define INT_STTS_INT7_OFF			(1<<7)
///Offset of the interrupt status bit for channel 6 in the global interrupt status register
#define INT_STTS_INT6_OFF			(1<<6)
///Offset of the interrupt status bit for channel 5 in the global interrupt status register
#define INT_STTS_INT5_OFF			(1<<5)
///Offset of the interrupt status bit for channel 4 in the global interrupt status register
#define INT_STTS_INT4_OFF			(1<<4)
///Offset of the interrupt status bit for channel 3 in the global interrupt status register
#define INT_STTS_INT3_OFF			(1<<3)
///Offset of the interrupt status bit for channel 2 in the global interrupt status register
#define INT_STTS_INT2_OFF			(1<<2)
///Offset of the interrupt status bit for channel 1 in the global interrupt status register
#define INT_STTS_INT1_OFF			(1<<1)
///Offset of the interrupt status bit for channel 0 in the global interrupt status register
#define INT_STTS_INT0_OFF			(1<<0)


//:::Masks for the Enable Register:::::::::::::::::::::::
///Offset of the enable bit for channel 15 in the global enable register
#define INT_STTS_EN15_OFF			(1<<15)
///Offset of the enable bit for channel 14 in the global enable register
#define INT_STTS_EN14_OFF			(1<<14)
///Offset of the enable bit for channel 13 in the global enable register
#define INT_STTS_EN13_OFF			(1<<13)
///Offset of the enable bit for channel 12 in the global enable register
#define INT_STTS_EN12_OFF			(1<<12)
///Offset of the enable bit for channel 11 in the global enable register
#define INT_STTS_EN11_OFF			(1<<11)
///Offset of the enable bit for channel 10 in the global enable register
#define INT_STTS_EN10_OFF			(1<<10)
///Offset of the enable bit for channel 9 in the global enable register
#define INT_STTS_EN9_OFF			(1<<9)
///Offset of the enable bit for channel 8 in the global enable register
#define INT_STTS_EN8_OFF			(1<<8)
///Offset of the enable bit for channel 7 in the global enable register
#define INT_STTS_EN7_OFF			(1<<7)
///Offset of the enable bit for channel 6 in the global enable register
#define INT_STTS_EN6_OFF			(1<<6)
///Offset of the enable bit for channel 5 in the global enable register
#define INT_STTS_EN5_OFF			(1<<5)
///Offset of the enable bit for channel 4 in the global enable register
#define INT_STTS_EN4_OFF			(1<<4)
///Offset of the enable bit for channel 3 in the global enable register
#define INT_STTS_EN3_OFF			(1<<3)
///Offset of the enable bit for channel 2 in the global enable register
#define INT_STTS_EN2_OFF			(1<<2)
///Offset of the enable bit for channel 1 in the global enable register
#define INT_STTS_EN1_OFF			(1<<1)
///Offset of the enable bit for channel 0 in the global enable register
#define INT_STTS_EN0_OFF			(1<<0)



///Virtual base address of the dma peripheral
static void*	baseAddrVirt;

/**
 * @brief Initializes the interface, to be called before any other function in this file
 *
 * @return negative in case of error
 */
int dma_init()
{
	if(baseAddrVirt)
	{
		return ERROR("dma already initialized",-1);
	}
	baseAddrVirt = mapPhysMemArea(DMA_BASE_ADDR_PHYS,DMA_AREA_LEN);
	if(!baseAddrVirt)
		return ERROR("Can't map dma peripheral",-1);
//	DBG_PRINTF("dma baseAddrVirt = 0x%x\n",baseAddrVirt);
	return 0;
}

/**
 * @brief Says if dma_init has been called with success
 *
 * @return true if dma_init has been called with success
 */
int dma_isInit()
{
	return baseAddrVirt!=0;
}

/**
 * @brief For debug purposes: prints the registers' contents to standard output
 *
 * @param channel	The channel
 *
 * @return negative in case of error
 */
int dma_printRegisters(int channel)
{
	char s[50*11];
	if(0>dma_dumpRegisters(channel,s))
		return ERROR("Error printing registers",-1);
	printf("%s",s);
}
/**
 * @brief For debug purposes: prints the registers' contents to standard output, in a single line
 *
 * @param channel	The channel
 *
 * @return negative in case of error
 */
int dma_printRegistersDense(int channel)
{
	char s[50*11];
	if(0>dma_dumpRegistersDense(channel,s))
		return ERROR("Error printing registers",-1);
	printf("%s",s);
}

/**
 * @brief For debug purposes: writes the register's contents on the provided string
 *
 * @param channel		The channel
 * @param s			The registers' contents will be written on this string (550 is an appropriate length)
 */
int dma_dumpRegisters(int channel, char* s)
{
	if(!baseAddrVirt)
		return ERROR("Dma interface not initialized\n",0);
	if(channel>14)
		return ERROR("Invalid channel",0);

	volatile uint32_t* cntrlSttsReg = 		(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_CONTROL_STATUS_OFF);
	volatile uint32_t* cntrlBlkAddrReg = 	(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_CONTROL_BLOCK_ADDR_OFF);
	volatile uint32_t* tiReg = 			(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_TRANSFER_INFO_OFF);
	volatile uint32_t* srcReg = 			(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_SOURCE_ADDR_OFF);
	volatile uint32_t* dstReg = 			(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_DEST_ADDR_OFF);
	volatile uint32_t* tlReg = 			(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_TRANSFER_LENGTH_OFF);
	volatile uint32_t* _2DReg = 			(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_2D_STRIDE_MODE_OFF);
	volatile uint32_t* nextReg = 			(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_NEXT_BLOCK_ADDR_OFF);
	volatile uint32_t* dbgReg = 			(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_DEBUG_OFF);

	return sprintf(s, "\
:::::DMA Register Dump::::::::::::::::::::::\n\
cntrlSttsReg =\t\t0x%x\n\
cntrlBlkAddrReg =\t0x%x\n\
tiReg =\t\t\t0x%x\n\
srcReg =\t\t0x%x\n\
dstReg =\t\t0x%x\n\
tlReg =\t\t\t0x%x\n\
_2DReg =\t\t0x%x\n\
nextReg =\t\t0x%x\n\
dbgReg =\t\t0x%x\n\
::::::::::::::::::::::::::::::::::::::::::::\n",*cntrlSttsReg,*cntrlBlkAddrReg,*tiReg,*srcReg,*dstReg,*tlReg,*_2DReg,*nextReg,*dbgReg);
}

/**
 * @brief For debug purposes: writes the register's contents on the provided string, in a single line
 *
 * @param channel		The channel
 * @param s			The registers' contents will be written on this string (550 is an appropriate length)
 */
int dma_dumpRegistersDense(int channel, char* s)
{
	if(!baseAddrVirt)
		return ERROR("Dma interface not initialized\n",0);
	if(channel>14)
		return ERROR("Invalid channel",0);

	volatile uint32_t* cntrlSttsReg = 		(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_CONTROL_STATUS_OFF);
	volatile uint32_t* cntrlBlkAddrReg = 	(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_CONTROL_BLOCK_ADDR_OFF);
	volatile uint32_t* tiReg = 			(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_TRANSFER_INFO_OFF);
	volatile uint32_t* srcReg = 			(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_SOURCE_ADDR_OFF);
	volatile uint32_t* dstReg = 			(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_DEST_ADDR_OFF);
	volatile uint32_t* tlReg = 			(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_TRANSFER_LENGTH_OFF);
	volatile uint32_t* _2DReg = 			(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_2D_STRIDE_MODE_OFF);
	volatile uint32_t* nextReg = 			(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_NEXT_BLOCK_ADDR_OFF);
	volatile uint32_t* dbgReg = 			(volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_DEBUG_OFF);

	return sprintf(s, "\
0x%x\t0x%x\t0x%x\t0x%x\t0x%x\t0x%x\t0x%x\t0x%x\t0x%x\n",*cntrlSttsReg,*cntrlBlkAddrReg,*tiReg,*srcReg,*dstReg,*tlReg,*_2DReg,*nextReg,*dbgReg);
}


/**
 * @brief For debug purposes: prints the control block's contents on standard output
 *
 * @param cb		The control block
 */
int dma_printControlBlock(ControlBlock* cb)
{
	char s[50*6];
	dma_controlBlockToString(cb,s);
	printf("%s",s);
}
/**
 * @brief For debug purposes: writes the control block's contents on a string
 *
 * @param cb		The control block
 * @param s			The control block's will be written on this string (300 is an appropriate length)
 */
int dma_controlBlockToString(ControlBlock* cb, char* s)
{
	return sprintf(s,"\
:::::ControlBlock at 0x%x::::::::::::::::::::::\n\
transferInfo			= 0x%x\n\
srcAddrPhys				= 0x%x\n\
dstAddrPhys				= 0x%x\n\
transferLength			= 0x%x\n\
_2DStrideMode			= 0x%x\n\
nextControlBlockAddrPhys	= 0x%x\n\
:::::::::::::::::::::::::::::::::::::::::::::::\n",cb,cb->transferInfo,	cb->srcAddrPhys,	cb->dstAddrPhys,	cb->transferLength,	cb->_2DStrideMode,	cb->nextControlBlockAddrPhys);
}

/**
 * @brief Resets the channel and sets it to default values
 *
 * @param channel	The channel
 */
int dma_channelResetInitDefault(int channel)
{
	if(!baseAddrVirt)
		return ERROR("Dma interface not initialized\n",-1);
	if(channel>14)
		return ERROR("Invalid channel",-2);
	volatile uint32_t* cntrlSttsReg = (volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_CONTROL_STATUS_OFF);
	*cntrlSttsReg = CNTRL_STTS_RESET_MASK;	//reset the channel and set all to zero
	usleep(100);
	*cntrlSttsReg = CNTRL_STTS_INT_MASK | CNTRL_STTS_END_MASK | BLD_BITFIELD(8,CNTRL_STTS_PANIC_PRIORITY_OFF,CNTRL_STTS_PANIC_PRIORITY_MASK) | BLD_BITFIELD(8,CNTRL_STTS_PRIORITY_OFF,CNTRL_STTS_PRIORITY_MASK);//clear flags

	volatile uint32_t* dbgReg = (volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_DEBUG_OFF);

	*dbgReg = 7;//clear any error flag

	usleep(1000);
	return 0;
}

/**
 * @brief Set the global enable status for the specified channel
 *
 * @param channel		The affected channel
 * @param value		0=disabled; anything else= enabled
 *
 * @return negative in case of error
 */
int dma_setChannelGlobalEnable(int channel, int value)
{
	if(!baseAddrVirt)
		return ERROR("Dma interface not initialized\n",-1);
	if(channel>14)
		return ERROR("Invalid channel",-2);

	volatile uint32_t* globalEnableReg = (volatile uint32_t*)(baseAddrVirt + REG_GLOBAL_ENABLE_OFF);
	if(value)
		*globalEnableReg = *globalEnableReg | 1<<channel;
	else
		*globalEnableReg = *globalEnableReg & ~(1<<channel);
	return 0;
}

/**
 * @brief Sets the channel active bit, this is automatically cleared at the end of each control block chain.
 *
 * @param channel		The affected channel
 * @param value		0=disabled; anything else= enabled
 *
 * @return negative in case of error
 */
int dma_setChannelActive(int channel, int value)
{
	if(!baseAddrVirt)
		return ERROR("Dma interface not initialized\n",-1);
	if(channel>14)
		return ERROR("Invalid channel",-2);

	value = (value)?1:0;
	volatile uint32_t* cntrlSttsReg = (volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_CONTROL_STATUS_OFF);
//	DBG_PRINTF("cntrlSttsReg = 0x%x\n",cntrlSttsReg);
//	DBG_PRINTF("value = 0x%x\n",value);
//	DBG_PRINTF("*cntrlSttsReg = 0x%"PRIx32"\n",*cntrlSttsReg);
	uint32_t v = (*cntrlSttsReg & ~CNTRL_STTS_ACTIVE_MASK) | value;
//	DBG_PRINTF("v = 0x%"PRIx32"\n",v);
	*cntrlSttsReg = v;
//	DBG_PRINTF("*cntrlSttsReg = 0x%"PRIx32"\n",*cntrlSttsReg);
}

/**
 * @brief Gets the channel active bit, this is automatically cleared at the end of each control block chain.
 *
 * @param channel		The channel
 *
 * @return negative in case of error
 */
int dma_isChannelActive(int channel)
{
	if(!baseAddrVirt)
		return ERROR("Dma interface not initialized\n",-1);
	if(channel>14)
		return ERROR("Invalid channel",-2);

	volatile uint32_t* cntrlSttsReg = (volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_CONTROL_STATUS_OFF);
	return *cntrlSttsReg & CNTRL_STTS_ACTIVE_MASK;
}



/**
 * @brief Sets the address of the control block to be loaded in the dma engine, to start the transfer you then have to set the active bit.
 *
 * @param channel				The channel
 * @param controlBlockAddrPhys	The physical address of the control block
 *
 * @return negative in case of error
 */
int dma_setControlBlockAddr(int channel, uint32_t controlBlockAddrPhys)
{
	if(!baseAddrVirt)
		return ERROR("Dma interface not initialized\n",-1);
	if(channel>14)
		return ERROR("Invalid channel",-2);

	volatile uint32_t* cntrlBlkAddrReg = (volatile uint32_t*)(baseAddrVirt + CHANNEL_REGISTER_SET_OFFSET(channel) + REG_CONTROL_BLOCK_ADDR_OFF);

	*cntrlBlkAddrReg = controlBlockAddrPhys;

	return 0;
}

/**
 * Writes the provided values to the provided control block
 *
 * @param cb	The contorl block (virtual) address
 * @param srcAddrPhys			Source adddress
 * @param dstAddrPhys			Destination address
 * @param transferLength		Number of bytes to copy
 * @param nextControlBlockPhys	The next control block to process
 * @param transferInfo			Use the TI_* macros
 * @param _2DStrideModeInfo		See bcm2835 datasheet
 * @param _2DStrideTransferLenX	See bcm2835 datasheet
 * @param _2DStrideTransferLenY	See bcm2835 datasheet
 *
 * @return Negative in case of error
 */
int	dma_writeControlBlock(	ControlBlock* cb,					uintptr_t srcAddrPhys,	uintptr_t dstAddrPhys,		size_t transferLength,
					uintptr_t nextControlBlockPhys,		uint32_t transferInfo,	uint32_t _2DStrideModeInfo,	size_t _2DStrideTransferLenX, size_t _2DStrideTransferLenY)
{
	if(!cb)
		return ERROR("cb == 0",-1);
	//fill with 0
	memset((void*)cb,0,sizeof(ControlBlock));

	cb->srcAddrPhys			= srcAddrPhys;
	cb->dstAddrPhys			= dstAddrPhys;
	cb->nextControlBlockAddrPhys  = nextControlBlockPhys;
	cb->transferInfo			= transferInfo;
	cb->_2DStrideMode			= _2DStrideModeInfo;

	if(transferInfo & TI_2DMODE_MASK)//se si usa la modalità 2D stride
		cb->transferLength = (_2DStrideTransferLenX & TL_X_LEN_MASK) | BLD_BITFIELD(_2DStrideTransferLenY,TL_Y_LEN_OFF,TL_Y_LEN_MASK);
	else
		cb->transferLength = transferLength;
	return 0;
}
/**
 * @brief Builds a control block with the specified characteristics
 *
 * @param srcAddrPhys			Source adddress
 * @param dstAddrPhys			Destination address
 * @param transferLength		Number of bytes to copy
 * @param nextControlBlockPhys	The next control block to process
 * @param transferInfo			Use the TI_* macros
 * @param _2DStrideModeInfo		See bcm2835 datasheet
 * @param _2DStrideTransferLenX	See bcm2835 datasheet
 * @param _2DStrideTransferLenY	See bcm2835 datasheet
 * @param area				The cacheCoherentMemoryProvider descriptor will be returned here, you'll have to free it using ccmp_free(area). You can get the bus and virtual address of the control block from here
 *
 * @return negative in case of error
 */
int	dma_allocControlBlockPhys(	uintptr_t srcAddrPhys,		uintptr_t dstAddrPhys,		size_t transferLength,		uintptr_t nextControlBlockPhys,		uint32_t transferInfo,
							uint32_t _2DStrideModeInfo,	size_t _2DStrideTransferLenX, size_t _2DStrideTransferLenY,	Ccmb_desc* area)
{
	if(transferLength & ~TL_LEN_MASK)
		return ERROR("transferLength too big",-1);
	if(_2DStrideTransferLenX & ~TL_X_LEN_MASK)
		return ERROR("_2DStrideTransferLenX too big",-2);
	if(BLD_BITFIELD(_2DStrideTransferLenY,TL_Y_LEN_OFF,TL_Y_LEN_MASK) & ~TL_Y_LEN_MASK)
		return ERROR("_2DStrideTransferLenY too big",-3);
	if(!area)
		return ERROR("area==0",-4);

	//allocate the space for the control block, cache coherent, aligned to 32 bytes
	ccmp_malloc(sizeof(ControlBlock),area,32);
	ControlBlock* cb = (ControlBlock*)area->virt_address;
	if(cb==NULL)
		return ERROR("Can't allocate the control block",-5);

	//write the provided values to the control block
	dma_writeControlBlock(	cb,srcAddrPhys,
					dstAddrPhys,
					transferLength,
					nextControlBlockPhys,
					transferInfo,
					_2DStrideModeInfo,
					_2DStrideTransferLenX,
					_2DStrideTransferLenY);

	return 0;
}

/**
 * @brief Returns a bit mask indicating the usable channels with 1
 *
 * @return the bit mask, 0xf0000000 in case of error
 */
uint32_t dma_getUsableChannels()
{
	int fd = open("/sys/module/dma/parameters/dmachans",O_RDONLY);

	int bufSize = 10;
	char buf[bufSize];
	memset(buf,0,10);
	int c=0;
	uint64_t t0 = getTimeNanoMonotonic();
	//read until you reach EOF (in this case read returns zero) or you fill the buffer or you get a timeout
	do
	{
		c = read(fd,buf + c,bufSize-c);
	}while(c!=0 && getTimeNanoMonotonic()-t0<100000000);//100ms timeout
	if(c!=0)//if so it reached the timeout
		return ERROR("can't read from /sys/module/dma/parameters/dmachans",0xf0000000);
	//printf("buf=%s\n",buf);
	for(int i=0;i<bufSize;i++)
	{
		if(buf[i]=='\n')
			buf[i]=0;
	}
	uint32_t value	= strtoul(buf,NULL,10);
	value &= ~(1 | (1<<2));//channels 0 and 2 are used by arm

	return value;
}
