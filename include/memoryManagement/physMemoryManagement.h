#ifndef MEMORY_MANAGEMENT_H
#define MEMORY_MANAGEMENT_H

#include <stdint.h>
#include <stdlib.h>



/**
 * @file physMemoryManagement.h
 *
 * @brief Helper functions to map and unmap physical memory regions
 */
void* mapPhysMemArea(uintptr_t areaBaseAddrPhys, size_t size);
int unmapPhysMemArea(void* areaBaseAddrVirt, size_t size);


#endif
