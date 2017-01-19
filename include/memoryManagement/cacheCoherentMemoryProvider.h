#ifndef CACHE_COHERENT_MEMORY_PROVIDER_H
#define CACHE_COHERENT_MEMORY_PROVIDER_H

#include <stdint.h>
#include <stddef.h>//for size_t



/**
 * @file cacheCoherentMemoryProvider.h
 *
 * @brief Provides a way to easily allocate memory areas that are coherent between CPU and DMA.
 *
 * This files provides functions to get blocks of memory with known associated physical address that are cache coherent.
 * This is useful when interfacing with peripherals that access memory directly, bypassing
 * the L1 or L2 caches, like the DMA.
 */


/**
 * @brief Describes a Cache Coherent Memory Block allocated by ccmp_malloc()
 */
typedef struct ccmb_desc_struct
{
	///A hande to the allocated block
	uint32_t	mem_alloc_handle;	//return value of mem_alloc(...) from mailbox.c
	///The bus address of the block
	uintptr_t	bus_address;	//from mem_lock(...)
	///The virtual address of the block
	void*		virt_address;	//from mmap
	///The size of the block in bytes
	size_t	size;
}Ccmb_desc;

int	ccmp_malloc(size_t reqSize, Ccmb_desc* allocatedAreaDesc, unsigned int byteAlignment);

int	ccmp_free(Ccmb_desc* allocatedAreaDesc);

uintptr_t ccmp_virtAddrToBusAddr(Ccmb_desc* desc, void* virtAddr);



#endif
