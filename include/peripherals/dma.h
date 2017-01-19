#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <stdlib.h>
#include "../memoryManagement/cacheCoherentMemoryProvider.h"



/**
 * @file dma.h
 *
 * @brief Interface to the dma peripheral
 *
 * The dma peripheral is able to perform transfers in the memory indipendently from the CPU.\n
 * It uses the bus addressing and it's not connected on the same cache level of the CPU, so it's preferable
 * to use the cacheCoherentMemory provider for source and destionation memory areas and also for the Control Blocks.\n
 * See the documentation at "BCM2835 ARM Peripherals" page 38
 */

//::::::::::::::::::::::: Masks and offsets for the Transfer Info register ::::::::::::::::::::::::::::::::::::::::::::::::

	//@{
	///Transfer Info register: No 2 beat bursts, missing in channels over 6
	#define TI_NO_WIDE_BURSTS_OFF					26
	#define TI_NO_WIDE_BURSTS_MASK				(1<<TI_NO_WIDE_BURSTS_OFF)
	//@}
	//@{
	///Transfer Info register: number of dummy writes after read or write
	#define TI_WAITS_OFF						21
	#define TI_WAITS_MASK						(31<<TI_WAITS_OFF)
	//@}
	//@{
	///Transfer Info register: selects the peripheral from wich reading the DREQ
	#define TI_PERMAP_OFF						16
	#define TI_PERMAP_MASK						(31<<TI_PERMAP_OFF)
	//@}
	//@{
	///Transfer Info register: Burst length
	#define TI_BURST_LEN_OFF					12
	#define TI_BURST_LEN_MASK					(0xf<<TI_BURST_LEN_OFF)
	//@}
	//@{
	///Transfer Info register: Write only zeroes
	#define TI_SRC_IGNORE_OFF					11
	#define TI_SRC_IGNORE_MASK					(1<<TI_SRC_IGNORE_OFF)
	//@}
	//@{
	///Transfer Info register: Control src reads with dreq
	#define TI_SRC_DREQ_OFF						10
	#define TI_SRC_DREQ_MASK					(1<<TI_SRC_DREQ_OFF)
	//@}
	//@{
	///Transfer Info register: source transfer width: 1=128bit 0=32bit
	#define TI_SRC_WIDTH_OFF					9
	#define TI_SRC_WIDTH_MASK					(1<<TI_SRC_WIDTH_OFF)
	//@}
	//@{
	///Transfer Info register: 1=increment src_addr after each read
	#define TI_SRC_INC_OFF						8
	#define TI_SRC_INC_MASK						(1<<TI_SRC_INC_OFF)
	//@}
	//@{
	///Transfer Info register: don't write to destination
	#define TI_DEST_IGNORE_OFF					7
	#define TI_DEST_IGNORE_MASK					(1<<TI_DEST_IGNORE_OFF)
	//@}
	//@{
	///Transfer Info register: Control writes with dreq
	#define TI_DEST_DREQ_OFF					6
	#define TI_DEST_DREQ_MASK					(1<<TI_DEST_DREQ_OFF)
	//@}
	//@{
	///Transfer Info register: destination transfer width: 1=128bit 0=32bit
	#define TI_DEST_WIDTH_OFF					5
	#define TI_DEST_WIDTH_MASK					(1<<TI_DEST_WIDTH_OFF)
	//@}
	//@{
	///Transfer Info register: Increment dest address after each write
	#define TI_DEST_INC_OFF						4
	#define TI_DEST_INC_MASK					(1<<TI_DEST_INC_OFF)
	//@}
	//@{
	///Transfer Info register: wait write response
	#define TI_WAIT_RESP_OFF					3
	#define TI_WAIT_RESP_MASK					(1<<TI_WAIT_RESP_OFF)
	//@}
	//@{
	///Transfer Info register: 2D stride mode, missing in channels over 6
	#define TI_2DMODE_OFF						1
	#define TI_2DMODE_MASK						(1<<TI_2DMODE_OFF)
	//@}
	//@{
	///Transfer Info register: generate interrupt after the transfer of this block
	#define TI_INTEN_OFF						(0)
	#define TI_INTEN_MASK						(1)
	//@}

	//@{
	///Transfer Info register: Peripherals ids for the permap field
	#define DREQ_ALWAYS_ON		0
	#define DREQ_DSI			1
	#define DREQ_PCM_TX		2
	#define DREQ_PCM_RX		3
	#define DREQ_SMI			4
	#define DREQ_PWM			5
	#define DREQ_SPI_TX		6
	#define DREQ_SPI_RX		7
	//@}


//:::::::::::::::::::::::::::::: Masks and offsets for the Transfer Length register :::::::::::::::::::::::::::::::::::::::

	//@{
	///Transfer Length register: To be used only in 2D stride mode: y length in bytes
	#define TL_Y_LEN_OFF	16
	#define TL_Y_LEN_MASK	(0x3fff<<TL_Y_LEN_OFF)
	//@}
	//@{
	///Transfer Length register: To be used only in 2D stride mode: x length in bytes
	#define TL_X_LEN_OFF	0
	#define TL_X_LEN_MASK	(0xffff)
	//@}
	//@{
	///Transfer Length register: To be used only in normal mode (not 2D stride): transfer length in bytes
	#define TL_LEN_OFF	0
	#define TL_LEN_MASK	(0x3fffffff)
	//@}


//::::::::::::::::::::::::::::::: Masks and offsets for the 2DStride Register :::::::::::::::::::::::::::::::::::::::::::::

	//@{
	///Signed increment to apply to the destination address after each row
	#define REG_2D_STRIDE_DST_STRIDE_OFF	16
	#define REG_2D_STRIDE_DST_STRIDE_MASK	(0xffff<<REG_2D_STRIDE_DST_STRIDE_OFF)
	//@}
	//@{
	///Signed increment to apply to the source address after each row
	#define REG_2D_STRIDE_SRC_STRIDE_OFF	0
	#define REG_2D_STRIDE_SRC_STRIDE_MASK	(0xffff)
	//@}



/**
 * @brief Describes a control block, contorl blocks are needed to load the registers of the DMA engine and start a transfer
 */
typedef volatile struct ControlBlock_struct
{
	///The tranfer info register: fill using the TI_* macros
	uint32_t	transferInfo;
	///The physical source address
	uint32_t	srcAddrPhys;
	///The physical destination address
	uint32_t	dstAddrPhys;
	///The transfer length register, fill using the TL_* macros
	uint32_t	transferLength;
	///The 2DStride register, fill using the REG_2D_* macros
	uint32_t	_2DStrideMode;
	///The physical address of the next control block, 0 if there isn't a next control block
	uint32_t	nextControlBlockAddrPhys;
	///This must be zero
	uint32_t	zero1;
	///This must be zero
	uint32_t	zero2;
} ControlBlock;



int dma_init();

int dma_isInit();

int dma_channelResetInitDefault(int channel);

int dma_setControlBlockAddr(int channel, uint32_t controlBlockAddrPhys);

int dma_writeControlBlock(ControlBlock* cb,uintptr_t srcAddrPhys,uintptr_t dstAddrPhys,size_t transferLength,uintptr_t nextControlBlockPhys,uint32_t transferInfo,uint32_t _2DStrideModeInfo,size_t _2DStrideTransferLenX,size_t _2DStrideTransferLenY);

int dma_allocControlBlockPhys(uintptr_t srcAddrPhys,uintptr_t dstAddrPhys,size_t transferLength,uintptr_t nextControlBlockPhys,uint32_t transferInfo,uint32_t _2DStrideModeInfo,size_t _2DStrideTransferLenX,size_t _2DStrideTransferLenY,Ccmb_desc* area);

int dma_dumpRegisters(int channel, char* s);

int dma_printRegisters(int channel);

int dma_printControlBlock(ControlBlock* cb);

int dma_controlBlockToString(ControlBlock* cb, char* s);

int dma_setChannelActive(int channel, int value);

int dma_setChannelGlobalEnable(int channel, int value);

uint32_t dma_getUsableChannels();

int dma_isChannelActive(int channel);

int dma_dumpRegistersDense(int channel, char* s);


//this tells doxygen to ignore the static assert
/// @cond
static_assert(sizeof(ControlBlock)==32, "ControlBlock has wrong size");
/// @endcond


#endif
