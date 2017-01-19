/**
 * @file cacheCoherentMemoryProvider.cpp
 *
 * @brief 	Provides a way to easily allocate memory areas that are coherent between CPU and DMA.
 *
 * See cacheCoherentMemoryProvider.h
 *
 * This file uses the "mailbox property interface". I didn't find a complete and clear documentation but only vague and
 * sparse informations. So I'll use the files mailbox.c and mailbox.h taken from
 * https://github.com/raspberrypi/userland/blob/master/host_applications/linux/apps/hello_pi/hello_fft/
 * And some information taken from the sources of servoblaster and piblaster.
 * Thanks to servoblaster, piblaster, the raspberry foundation and broadcom.
 */
#include <stdint.h>
#include <stdlib.h>
#include "../notMine/mailbox.h"
#include <CCRCodingUtils/include/errorManagement.h>
#include <memoryManagement/physMemoryManagement.h>
#include <memoryManagement/addressing.h>
#include <memoryManagement/cacheCoherentMemoryProvider.h>

/**
 * @brief Provides a way to easily allocate memory areas that are coherent between CPU and DMA.
 *
 * @param reqSize			The size of the block to be allocated.
 * @param allocatedAreaDesc	Here will be returned the descriptor for the allocated block
 * @param byteAlignment		The allocated block will be aligned to this value
 *
 * @return negative in case of error
 */
int	ccmp_malloc(size_t reqSize, Ccmb_desc* allocatedAreaDesc, unsigned int byteAlignment)
{
	int fd = mbox_open();

	allocatedAreaDesc->mem_alloc_handle = mem_alloc(fd,reqSize,byteAlignment,0xc);//for rpi1 the flag should be 0x4, maybe
	if(!allocatedAreaDesc->mem_alloc_handle)
		return ERROR("Cant' allocate memory from mailbox property interface",-1);
	allocatedAreaDesc->bus_address	= mem_lock(fd,allocatedAreaDesc->mem_alloc_handle);
	if(!allocatedAreaDesc->bus_address)
		return ERROR("Cant' lock memory from mailbox property interface",-2);
	allocatedAreaDesc->virt_address	= mapPhysMemArea(TO_PHYS_MEM(allocatedAreaDesc->bus_address),reqSize);
	if(!allocatedAreaDesc->virt_address)
		return ERROR("Cant' mmap memory from mailbox property interface",-3);
	allocatedAreaDesc->size = reqSize;
	mbox_close(fd);
}

/**
 * @brief Frees a block allocated with ccmp_malloc()
 *
 * @return negative in case of error
 */
int ccmp_free(Ccmb_desc* allocatedAreaDesc)
{
	int fd = mbox_open();
	int r = unmapPhysMemArea(allocatedAreaDesc->virt_address, allocatedAreaDesc->size);
	r |= mem_unlock(	fd,	allocatedAreaDesc->mem_alloc_handle);
	r |= mem_free(	fd,	allocatedAreaDesc->mem_alloc_handle);
	mbox_close(fd);


	if(r)
		return ERROR("error freeing memory from mailbox property interface",-1);
	return 0;
}

/**
 * @brief Converts a virtual address inside the block described by desc to it's bus address
 *
 * @return the bus address
 */
uintptr_t ccmp_virtAddrToBusAddr(Ccmb_desc* desc, void* virtAddr)
{
	return (uintptr_t)virtAddr - (uintptr_t)desc->virt_address + desc->bus_address;
}
