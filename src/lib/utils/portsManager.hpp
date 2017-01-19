#ifndef PORTS_MANAGER_HPP
#define PORTS_MANAGER_HPP

typedef unsigned int Bcm_port;
int isPortUsed(Bcm_port bcmPort);

int acquirePort(Bcm_port bcmPort);

#endif
