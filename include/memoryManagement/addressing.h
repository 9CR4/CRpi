#ifndef ADDRESSING_H
#define ADDRESSING_H

#include <stdint.h>

/**
 * @file addressing.h
 *
 * @brief Provides helper macros for address management
 *
 * See the documentation at "BCM 2835 ARM Peripherals" pag 4 and following.
 *
 * In the BCM 2835 (also BCM2836 and BCM2837) there are three different addressing modes, so we have:
 *	- Virtual address
 *	- Physical address
 *	- Bus address
 *
 * Virtual addresses span on the whole address space (0x0-0xffffffff)
 * Physical addresses from 0x0 to 0x3fffffff (1GB)
 * Bus addresses on the whole address space (0x0-0xffffffff)
 *
 * In bus addressing the RPI memory is mapped at different addresses in different ways.
 * These areas are called aliases:
 * 	- 0x00000000 - 0x3fffffff		'0' alias: L1 and L2 cached
 * 	- 0x40000000 - 0x7fffffff		'4' alias: L2 cache coherent (not allocating)
 * 	- 0x80000000 - 0xbfffffff		'8' alias: L2 cached only
 * 	- 0xc0000000 - 0xffffffff		'C' alias: direct uncached
 *
 *
 * Peripherals are mapped to physical memory at address 0x20000000 in rpi1 and 0x3f000000 in rpi2 and rpi3.
 * And so are also mapped to bus address in the various aliases.
 *
 */

///Physical base address of the peripherals memory area on the raspberry 2
#define PERIPHERAL_BASE_ADDR_PHYS_RPI2 0x3f000000
///Physical base address of the peripherals memory area on the raspberry 1
#define PERIPHERAL_BASE_ADDR_PHYS_RPI1 0x20000000
///Bus base address of the peripherals memory area
#define PERIPHERAL_BASE_ADDR_BUS       0x7e000000

extern uintptr_t peripheralsBaseAddressPhys;//defined in addressing.c

int addrVirtToPhys(void* virtAddr, uintptr_t* physAddrRet);


/**
 * @brief Converts physical memory from any alias to alias 0 memory. So it works to convert bus memory to physical memory.
 */
#define TO_PHYS_MEM(addr)		(addr & ~0xc0000000)

/**
 * @brief Converts a peripheral's physical address to it's bus address
 */
#define PERIPHERAL_PHYS_TO_BUS_ADDR(addr)	(addr - PERIPHERAL_BASE_ADDR_PHYS_RPI2 + PERIPHERAL_BASE_ADDR_BUS)



#endif
