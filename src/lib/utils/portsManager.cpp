#include "portsManager.hpp"
#include <peripherals/gpio.h>
#include <mutex>

/**
 * @file portsManager.cpp
 *
 * @brief utility to manage ports usage, and avoid using the same port for two different things.
 */

 uint64_t usedPorts;

 mutex mainMutex;

 int isPortUsed(Bcm_port bcmPort)
 {
	return (usedPorts >> bcmPort)&1;
 }

 int acquirePort(Bcm_port bcmPort)
 {
	if(bcmPort>BCM_MAX)
		throw runtime_error("Illegal bcm port, max is "+BCM_MAX);

	unique_lock<mutex> lock(mainMutex);

	if(isPortUsed(bcmPort))
	{
		lock.unlock();
		return -1;
	}
	usedPorts |= 1<<bcmPort;

	lock.unlock();
	return 0;
 }
