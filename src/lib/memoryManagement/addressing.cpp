#include <memoryManagement/addressing.h>
#include <CCRCodingUtils/include/errorManagement.h>
#include <unistd.h>
#include <fcntl.h>//open
#include <inttypes.h>

/**
 * @file addressing.cpp
 *
 * @brief Provides helper macros for address management
 *
 * see BCM 2835 pag 4 and following.
 *
 * See also the doc at addressing.h for some infos
 *
 */

///Physical base address of the peripherals memory area
uintptr_t peripheralsBaseAddressPhys = PERIPHERAL_BASE_ADDR_PHYS_RPI2;

/**
 * @brief Gets the physical address corresponding to the provided virtual address
 *
 * @param virtAddr	The virtuall address to convert
 * @param physAddrRet	The physical address is rturned here
 *
 * @return 0 for success, negative for error, 1 if the virtual address isn't in ram
 */
int addrVirtToPhys(void* virtAddr, uintptr_t* physAddrRet)
{
	// /proc/self/pagemap is a uint64_t array where the index represents the virtual page number and the value at that index represents the physical page number.
	//So if virtual address is 0x1000000, read the value at *array* index 0x1000000/PAGE_SIZE and multiply that by PAGE_SIZE to get the physical address.
	//because files are bytestreams, one must explicitly multiply each byte index by 8 to treat it as a uint64_t array.

	int fd = open("/proc/self/pagemap", O_RDONLY);
	if(fd==-1)
		return ERROR("Can't open /proc/self/pagemap",-1);
	uintptr_t pageSize = getpagesize();
	uintptr_t pageNum = ((uintptr_t)virtAddr)/pageSize;
//	DBG_PRINTF("virtAddr = 0x%x\n",virtAddr);
//	DBG_PRINTF("pageSize = 0x%x\n",pageSize);
//	DBG_PRINTF("pageNum = 0x%x\n",pageNum);
	uint64_t physPage=0;
	int c=0;
	int r=0;
	do
	{
		if(r>20)
			return ERROR("can't read from /proc/self/pagemap",-2);
		int e = lseek(fd,pageNum*8,SEEK_SET);//8 bytes per page
		if(e!=pageNum*8)
		{
			ERROR("Can't seek in /proc/self/pagemap",-3);
			close(fd);
			return -3;
		}
		c = read(fd,&physPage,8);
		r++;
	}while(c!=8);

	if(!(physPage && (((uint64_t)1)<<63)))//page isn't in ram
	{
		return 1;
	}
	close(fd);
	physPage&=0x40000000000000-1;//take away the flags
//	DBG_PRINTF("*physAddrRet=0x%x\n",*physAddrRet);
//	DBG_PRINTF("virtAddr=0x%x\n",virtAddr);
//	DBG_PRINTF("physPage=0x%"PRIx64"\n",physPage);

	uintptr_t low = (((uintptr_t)virtAddr) & (pageSize-1));
	uintptr_t high = (physPage*pageSize);
//	DBG_PRINTF("low=0x%x\n",low);
//	DBG_PRINTF("high=0x%x\n",high);
//	DBG_PRINTF("high | low=0x%x\n",high | low) ;
	*physAddrRet = high | low | 0x40000000;
//	DBG_PRINTF("*physAddrRet=0x%x\n",*physAddrRet);
	return 0;
}

