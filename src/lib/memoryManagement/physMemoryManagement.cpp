/**
 * @file physMemoryManagement.cpp
 *
 * @brief Helper functions to map and unmap physical memory regions
 */

#include <fcntl.h>//open
#include <unistd.h>//close
#include <stdio.h>//perror
#include <sys/mman.h>//mmap
#include <CCRCodingUtils/include/errorManagement.h>
#include <memoryManagement/physMemoryManagement.h>

/**
 * @brief Maps the specified physical memory area to a virtual memory area
 *
 * @param areaBaseAddrPhys	The physical base address of the area
 * @param size			The size of the area to map
 *
 * @return	The generated virtual address, or zero in case of error
 */
void* mapPhysMemArea(uintptr_t areaBaseAddrPhys, size_t size)
{
	uintptr_t off = areaBaseAddrPhys % getpagesize();
	areaBaseAddrPhys -= off;
	size+=off;

	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	void * vaddr;
	if (fd < 0)
	{
		perror("Can't open /dev/mem: ");
		return (void*)ERROR("Can't open /dev/mem",0);
	}

	vaddr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, areaBaseAddrPhys);
	if (vaddr == MAP_FAILED)
	{
		perror("Failed to mmap peripheral: ");
		return (void*)ERROR("Failed to mmap peripheral",0);
	}
	close(fd);

	return vaddr + off;
}

/**
 * @brief Unmaps a physical memory area mapped using mapPhysMemArea()
 *
 * @param areaBaseAddrVirt	The virtual base address of the peripheral
 * @param size			The size of the mapped area
 *
 * @return negative in case of error
 */
int unmapPhysMemArea(void* areaBaseAddrVirt, size_t size)
{
	uintptr_t off = ((uintptr_t)areaBaseAddrVirt) % getpagesize();
	areaBaseAddrVirt -= off;
	int r = munmap(areaBaseAddrVirt,size);
	if(r)
		return ERROR("Error unmapping memory (munmap())",-1);
	return 0;
}
