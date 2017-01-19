#include <CCRCodingUtils/include/utils.h>
#include <CCRCodingUtils/include/errorManagement.h>
#include <peripherals/gpio.h>
#include <peripherals/dma.h>
#include <peripherals/pwm.h>
#include <peripherals/clockManager.h>
#include <memoryManagement/addressing.h>
#include <memoryManagement/cacheCoherentMemoryProvider.h>
#include <utils/pwmByDma.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <signal.h>
#include "../lib/peripherals/gpio_internal.h"

/**
 * @file demo.c
 *
 * @brief This files provides some demo/test functions
 */

///This variable is used to signal that the seed for the random number generator has been set
int rndIntitialized = 0;

/**
 * This test tunrs on and off gpio port bcm9 output using the dma
 */
int dmaToGpioTest()
{
	printf("This test tunrs on and off gpio port bcm9 output using the dma.\ncontinue?(y/n)\n");
	char r;
	scanf(" %1c",&r);
	if(r!='y')
		return 0;

	gpio_init();
	dma_init();

	gpio_setFunction(9,PORT_FUNCTION_OUTPUT);

	Ccmb_desc srcArea;
	ccmp_malloc(4,&srcArea,1);

	*((uint32_t*)(srcArea.virt_address)) = 1<<9;

	int channel = 4;

	uintptr_t cbPhys;
	ControlBlock* cb;
	Ccmb_desc cbArea;
	dma_setChannelGlobalEnable(channel,1);
	//cbPhys = dma_allocControlBlock(src,dst,9,0,TI_SRC_INC_MASK | TI_DEST_INC_MASK,0xdeadbeef,0,0,&cb);
	dma_allocControlBlockPhys(srcArea.bus_address,PERIPHERAL_PHYS_TO_BUS_ADDR(GPSET0_ADDR_PHYS),4,0,TI_SRC_INC_MASK | TI_DEST_INC_MASK,0xdeadbeef,0,0,&cbArea);
	cbPhys = cbArea.bus_address;
	dma_channelResetInitDefault(channel);

	//dma_printRegisters(channel);

	//printf("deactivate...\n");
	dma_setChannelActive(channel, 0);
	//dma_printRegisters(channel);

	//printf("dma_setControlBlockAddr...\n");
	dma_setControlBlockAddr(channel,cbPhys);
	//dma_printRegisters(channel);

	//printf("dma_setChannelActive...\n");
	dma_setChannelActive(channel, 1);
	//dma_printRegisters(channel);
	printf("set (wait 3 seconds)\n");

	sleep(3);

	ccmp_free(&cbArea);
	dma_channelResetInitDefault(channel);
	dma_allocControlBlockPhys(srcArea.bus_address,PERIPHERAL_PHYS_TO_BUS_ADDR(GPCLEAR0_ADDR_PHYS),4,0,TI_SRC_INC_MASK | TI_DEST_INC_MASK,0xdeadbeef,0,0,&cbArea);
	cbPhys = cbArea.bus_address;
	dma_setControlBlockAddr(channel,cbPhys);
	//dma_printRegisters(channel);
	printf("cleared (wait 3 seconds)\n");
	sleep(3);

	ccmp_free(&cbArea);
	dma_channelResetInitDefault(channel);
	dma_allocControlBlockPhys(srcArea.bus_address,PERIPHERAL_PHYS_TO_BUS_ADDR(GPSET0_ADDR_PHYS),4,0,TI_SRC_INC_MASK | TI_DEST_INC_MASK,0xdeadbeef,0,0,&cbArea);
	cbPhys = cbArea.bus_address;
	dma_setControlBlockAddr(channel,cbPhys);
	//dma_printRegisters(channel);
	printf("set (wait 3 seconds)\n");

	sleep(3);

	ccmp_free(&cbArea);
	dma_channelResetInitDefault(channel);
	dma_allocControlBlockPhys(srcArea.bus_address,PERIPHERAL_PHYS_TO_BUS_ADDR(GPCLEAR0_ADDR_PHYS),4,0,TI_SRC_INC_MASK | TI_DEST_INC_MASK,0xdeadbeef,0,0,&cbArea);
	cbPhys = cbArea.bus_address;
	dma_setControlBlockAddr(channel,cbPhys);
	//dma_printRegisters(channel);
	printf("cleared (wait 3 seconds)\n");
	sleep(3);

	ccmp_free(&cbArea);
	dma_channelResetInitDefault(channel);
	dma_allocControlBlockPhys(srcArea.bus_address,PERIPHERAL_PHYS_TO_BUS_ADDR(GPSET0_ADDR_PHYS),4,0,TI_SRC_INC_MASK | TI_DEST_INC_MASK,0xdeadbeef,0,0,&cbArea);
	cbPhys = cbArea.bus_address;
	dma_setControlBlockAddr(channel,cbPhys);
	//dma_printRegisters(channel);
	printf("set (wait 3 seconds)\n");
	sleep(3);


	gpio_setFunction(9,PORT_FUNCTION_INPUT);
}

/**
 * This test turns a gpio output on and off using the dma and delays it's effect using the dreq signal from the pwm
 */
int dmaToGpioPacedTest()
{
	printf("This test turns a gpio output on and off using the dma and delays it's effect using the dreq signal from the pwm.\ncontinue?(y/n)\n");
	char r;
	scanf(" %1c",&r);
	if(r!='y')
		return 0;

	clkMan_init();
	gpio_init();
	dma_init();
	pwm_init();

      pwm_setRange(1,125000);
      pwm_setClock(CLK_SRC_PLLD,4000,0);//circa 8us
      //in totale con range e clock ottengo un secondo di periodo
      pwm_setMode(1,1);//serializer mode
      pwm_setSerializerMode(1,1);//use the fifo
      pwm_setRepeatLastWord(1,1);
      pwm_setDreqThreshold(1);
      pwm_setDmaEnable(1);
	pwm_setEnable(1,1);
	pwm_clearFifo();
	pwm_writeToFifo(0xffffffff);

	gpio_setFunction(18,PORT_FUNCTION_ALT_5);
	gpio_setFunction(9,PORT_FUNCTION_OUTPUT);

	//sets up the source area
	Ccmb_desc srcArea;
	ccmp_malloc(4,&srcArea,1);
	*((uint32_t*)(srcArea.virt_address)) = 1<<9;


	int channel = 4;
	uintptr_t cbPhys;
	ControlBlock* cb;
	Ccmb_desc cbArea;

	dma_setChannelGlobalEnable(channel,1);

	//dma gpio test
	printf("Testing without dreq pacing\n");
	dma_channelResetInitDefault(channel);
	dma_allocControlBlockPhys(	srcArea.bus_address,
							PERIPHERAL_PHYS_TO_BUS_ADDR(GPSET0_ADDR_PHYS),
							4,
							0,
							TI_SRC_INC_MASK | TI_DEST_INC_MASK,
							0xdeadbeef,
							0,
							0,
							&cbArea);
	cbPhys = cbArea.bus_address;
	dma_setChannelActive(channel, 0);
	dma_setControlBlockAddr(channel,cbPhys);
	dma_setChannelActive(channel, 1);
	printf("The pin should be on\n");
	dma_printRegisters(channel);
	sleep(1);
	ccmp_free(&cbArea);

	gpio_setOutput(9,0);
	printf("Pin turned off\n");
	sleep(1);


	//dma gpio paced test
	printf("Testing with dreq pacing\n");
	//dma_channelResetInitDefault(channel);
	dma_allocControlBlockPhys(	srcArea.bus_address,
							PERIPHERAL_PHYS_TO_BUS_ADDR(GPSET0_ADDR_PHYS),
							4,
							0,
							TI_SRC_INC_MASK | TI_DEST_INC_MASK | TI_DEST_DREQ_MASK | BLD_BITFIELD(DREQ_PWM,TI_PERMAP_OFF,TI_PERMAP_MASK),
							0xdeadbeef,
							0,
							0,
							&cbArea);
	cbPhys = cbArea.bus_address;
	//dma_setChannelActive(channel, 0);
	dma_setControlBlockAddr(channel,cbPhys);
	printf("In about 4 seconds the pin should turn on\n");
	printf("waiting 10 seconds...\n");
	//4 secondi
	pwm_writeToFifo(0xff);
	pwm_writeToFifo(0xffffffff);
	pwm_writeToFifo(0xffffffff);
	pwm_writeToFifo(0xffffffff);
	dma_setChannelActive(channel, 1);
	dma_printRegisters(channel);
	sleep(10);
	dma_printRegisters(channel);





	ccmp_free(&srcArea);
	ccmp_free(&cbArea);
	gpio_setOutput(9,0);
	gpio_setFunction(9,PORT_FUNCTION_INPUT);
	gpio_setFunction(18,PORT_FUNCTION_INPUT);
}

/**
 * This test uses te dma to copy a memory region
 */
int dmaTest()
{
	printf("This test uses te dma to copy a memory region.\ncontinue?(y/n)\n");
	char resp;
	scanf(" %1c",&resp);
	if(resp!='y')
		return 0;

      if(0>dma_init())
		return -1;

	if(!rndIntitialized)
	{
		srand(time(NULL));
		rndIntitialized = 1;
	}
	int rnd = rand()/(RAND_MAX/1000);
	//printf("rnd = %d\n",rnd);
	void* p = malloc(10*rnd);//alloc a memory area of random length, to change the addresses a bit between different runs



	Ccmb_desc area;

	ccmp_malloc(18,&area,1);

	char* src = (char*)area.virt_address;// aligned_alloc(32,9);
	strcpy(src,"test dma");
	char* dst = (char*)(area.virt_address + 9);// aligned_alloc(32,9);
	strcpy(dst,"XXXXXXXX");

	printf("src = %s\n",src);
	printf("dst = %s\n",dst);



	int channel = 4;

	uintptr_t cbPhys;
	ControlBlock* cb;
	Ccmb_desc cbArea;
	dma_setChannelGlobalEnable(channel,1);
	dma_allocControlBlockPhys(area.bus_address,area.bus_address+9,9,0,TI_SRC_INC_MASK | TI_DEST_INC_MASK,0xdeadbeef,0,0,&cbArea);
	cbPhys = cbArea.bus_address;

	//printf("reset...\n");
	dma_channelResetInitDefault(channel);

	//dma_printRegisters(channel);

	//printf("deactivate...\n");
	dma_setChannelActive(channel, 0);
	//dma_printRegisters(channel);

	//printf("dma_setControlBlockAddr...\n");
	dma_setControlBlockAddr(channel,cbPhys);
	//dma_printRegisters(channel);

	//printf("dma_setChannelActive...\n");
	dma_setChannelActive(channel, 1);
	//dma_printRegisters(channel);

	usleep(1);

	//dma_printRegisters(channel);


	printf("src = %s\n",src);
	printf("dst = %s\n",dst);
	int r= strcmp(src,dst);
	//addrVirtToPhys(cb,&cbPhys);
	//printf("cbPhys = 0x%x\n",cbPhys);
	munlock((void*)cb,sizeof(ControlBlock));
	free(p);
	ccmp_free(&area);
	ccmp_free(&cbArea);
	munlock(src,9);
	munlock(dst,9);
	//printf("end\n");
	return r;
}

/**
 * This test turns the gpio port 9 on and the disables it
 */
int gpioTest()
{
	printf("This test turns the gpio port 9 on and the disables it.\ncontinue?(y/n)\n");
	char r;
	scanf(" %1c",&r);
	if(r!='y')
		return 0;

	if(0>gpio_init())
		return ERROR("gpio init failed",-1);

	gpio_setFunction(9,PORT_FUNCTION_OUTPUT);

	gpio_setOutput(9,1);

	sleep(3);
	printf("waiting 3 seconds...\n");
	//setPortOutputValue(18,0);
	gpio_setFunction(9,PORT_FUNCTION_INPUT);

}

/**
 * This test outputs a PWM signal at 4688Hz from pin 18
 */
int pwmTest()
{
	printf("This test outputs a PWM signal at 4688Hz from pin 18.\ncontinue?(y/n)\n");
	char r;
	scanf(" %1c",&r);
	if(r!='y')
		return 0;

	if(gpio_init()<0)
		return -1;
	printf("gpio_init completed\n");
	//getchar();
	if(clkMan_init()<0)
		return -3;
	printf("clkMan_init completed\n");
	//getchar();
	if(pwm_init()<0)
		return -2;
	printf("pwm_init completed\n");
	//getchar();

	pwm_resetChannel(1);
	pwm_setClock(CLK_SRC_OSCILLATOR,4095,0);//19.2MHz/4095 = 4688 Hz
	pwm_setEnable(1,0);
	pwm_setRange(1,32);
	//usleep(200);
	pwm_setData(1,1);
	//usleep(200);
	pwm_setMSenable(1,1);
	//usleep(200);
	pwm_setEnable(1,1);
	gpio_setFunction(18,PORT_FUNCTION_ALT_5);
	sleep(3);
	printf("waiting 3 seconds...\n");
	gpio_setFunction(18,PORT_FUNCTION_INPUT);
}

/**
 * This test outputs the 32 bit word 0x53 from port bcm18 serially at 37,5KHz
 */
int pwmSerializerTest()
{
	printf("This test outputs the 32 bit word 0x53 from port bcm18 serially at 37,5KHz.\ncontinue?(y/n)\n");
	char r;
	scanf(" %1c",&r);
	if(r!='y')
		return 0;

	if(gpio_init()<0)
		return -1;
	printf("gpio_init completed\n");
	//getchar();
	if(clkMan_init()<0)
		return -3;
	printf("clkMan_init completed\n");
	//getchar();
	if(pwm_init()<0)
		return -2;
	printf("pwm_init completed\n");
	//getchar();

	pwm_resetChannel(1);
	gpio_setFunction(18,PORT_FUNCTION_ALT_5);
	pwm_setEnable(1,1);
	pwm_setClock(CLK_SRC_OSCILLATOR,512,0);
	pwm_setRange(1,32);
	pwm_clearFifo();
	pwm_setMode(1,1);
	pwm_setSerializerMode(1,0);

	pwm_setData(1,0x53);
	printf("waiting 3 seconds\n");
	sleep(3);
	pwm_setEnable(1,0);

	gpio_setFunction(18,PORT_FUNCTION_INPUT);
}

/**
 * This test outputs a pulse of around 1 millisecond from port 18
 */
int simplePulseTest()
{
	printf("This test outputs a pulse of around 1 millisecond from port 18.\ncontinue?(y/n)\n");
	char r;
	scanf(" %1c",&r);
	if(r!='y')
		return 0;

	gpio_init();
	gpio_setOutput(18,0);
	gpio_setFunction(18,PORT_FUNCTION_OUTPUT);


	gpio_setOutput(18,1);
	usleep(1000);
	gpio_setOutput(18,0);
	gpio_setFunction(18,PORT_FUNCTION_INPUT);
}

/**
 * This function outputs a pwm signal with the requested precision and duty cycle 10%, genereted with the DMA
 */
int pwmByDmaTest(unsigned int p)
{
	printf("This function outputs a pwm signal with the requested precision and duty cycle 10%, genereted with the DMA.\ncontinue?(y/n)\n");
	char r;
	scanf(" %1c",&r);
	if(r!='y')
		return 0;

	printf("pwmByDmaTest(%u)\n",p);
	getchar();
	gpio_init();
	if(0>pwmDma_init(p))//precision at p
		return ERROR("can't initialize pwmByDma",-1);

	gpio_setFunction(9,PORT_FUNCTION_OUTPUT);
	gpio_setFunction(18,PORT_FUNCTION_ALT_5);
	printf("port 18 pwm enabled. press enter to continue...");
	getchar();

	int channel = 5;

	pwmDma_initChannel(channel,pwmDma_get_pulseDuration_us()*10);//period of 10*p us
	pwmDma_printControlBlocks(channel);
	pwmDma_addPulse(channel,9,0,						pwmDma_get_pulseDuration_us());//pulse p us long

	pwmDma_printWords(channel);

	printf("Press enter to continue\n");
	getchar();
	pwmDma_freeChannel(channel);
	printf("channel freed. press enter to continue...\n");
	getchar();
	gpio_setFunction(9,PORT_FUNCTION_INPUT);
	gpio_setFunction(18,PORT_FUNCTION_INPUT);
}


/**
 * This function outputs a pwm signal with the requested precision and duty cycle 10%, genereted with the DMA
 */
int pwmByDmaTestVisible()
{
	printf("This function outputs a pwm signal with a period of 10s, genereted with the DMA.\ncontinue?(y/n)\n");
	char r;
	scanf(" %1c",&r);
	if(r!='y')
		return 0;


	gpio_init();
	if(0>pwmDma_init(25000))//precision at 25ms
		return ERROR("can't initialize pwmByDma",-1);

	gpio_setFunction(9,PORT_FUNCTION_OUTPUT);
	gpio_setFunction(18,PORT_FUNCTION_ALT_5);

	int channel = 5;

	pwmDma_initChannel(channel,pwmDma_get_pulseDuration_us()*400);//period of 10 s

	double dc = 0;
	int s = 0;
	int e = 0;
	while(s>=0 || e>=0)
	{
		printf("enter the duty cycle (-1 -1 to quit) (-1 0 to reset)\n");
		scanf(" %d %d",&s,&e);
		printf("s=%d e=%d\n",s,e);
		if(s>=0 && e>=0)
		{
			pwmDma_addPulse(channel,9,s,	e);//pulse p us long
		}
		else if(s==-1 && e==0)
		{
			pwmDma_clearPort(channel,9);
		}
	}

	printf("closing...\n");
	sleep(1);
	pwmDma_freeChannel(channel);
	gpio_setFunction(9,PORT_FUNCTION_INPUT);
	gpio_setFunction(18,PORT_FUNCTION_INPUT);
}


/**
 * This test controls a servomotor on port bcm26 using a pwm signal generated with the DMA
 */
int pwmByDmaTestServo()
{
	printf("This test controls a servomotor on port bcm26 using a pwm signal generated with the DMA.\ncontinue?(y/n)\n");
	char r;
	scanf(" %1c",&r);
	if(r!='y')
		return 0;

	//getchar();
	gpio_init();
	if(0>pwmDma_init(1))//precision at 1 us
		return ERROR("can't initialize pwmByDma",-1);

	int port = 26;
	gpio_setFunction(port,PORT_FUNCTION_OUTPUT);

	int channel = 5;

	pwmDma_initChannel(channel,15000);//period of 15ms
	double a=1;
	while(a>=0)
	{
		printf("Enter the ratio (-1 to exit)\n");
		scanf("%lf",&a);
		if(a>=0)
			pwmDma_addPulse(channel,port,0,1000 + 1000*a);
	}

	getchar();
	pwmDma_freeChannel(channel);
	gpio_setFunction(port,PORT_FUNCTION_INPUT);
}


/**
 * This test controls a servomotor and a motor (in both directions). This can control an rc car, not very comfortably but it can.
 */
int pwmByDmaTestServoAndMotor()
{
	printf("This test controls a servomotor and a motor (in both directions). This can control an rc car, not very comfortably but it can..\ncontinue?(y/n)\n");
	char r;
	scanf(" %1c",&r);
	if(r!='y')
		return 0;

	gpio_init();
	if(0>pwmDma_init(1))
		return ERROR("can't initialize pwmByDma",-1);

	int servoPort = 26;
	int motorPort1 = 17;
	int motorPort2 = 27;
	gpio_setFunction(servoPort,PORT_FUNCTION_OUTPUT);
	gpio_setFunction(motorPort1,PORT_FUNCTION_OUTPUT);
	gpio_setFunction(motorPort2,PORT_FUNCTION_OUTPUT);
	//gpio_setFunction(18,PORT_FUNCTION_ALT_5);;

	int channel = 5;

	pwmDma_initChannel(channel,15000);//period of 10*256us
	char sm;
	while(1)
	{
		double a;
		printf("'m [ratio]' to control the motor 's [ratio]' for the servo 'q' to exit\n");
		char s[21] = {0};
		fgets(s,20,stdin);//scanf would stop at the first whitespace
		int n = sscanf(s," %c %lf",&sm,&a);
		if(n<2)
			a = -100;
		if(n<1)
		{
			a = -100;
			sm = '*';
		}
		printf("[%c %f]\n",sm,a);
		if(sm=='m')
		{
			//printf("motor: enter the ratio\n");
			//scanf("%lf",&a);
			if(a>=0 && a<=1)
			{
				pwmDma_clearPort(channel,motorPort2);
				pwmDma_clearPort(channel,motorPort1);
				//pwmDma_printWords(channel);
				pwmDma_addMultiplePulse(channel,motorPort1,0,100*a,100);
				//pwmDma_printWords(channel);
			}
			else if(a>=-1 && a<0)
			{
				a = -a;
				pwmDma_clearPort(channel,motorPort1);
				pwmDma_clearPort(channel,motorPort2);

				pwmDma_addMultiplePulse(channel,motorPort2,0,100*a,100);
			}
			else if(a==-100)
			{
				pwmDma_clearPort(channel,motorPort1);
				pwmDma_clearPort(channel,motorPort2);
			}
		}
		else if(sm=='s')
		{
			//printf("servo: enter the ratio\n");
			//scanf("%lf",&a);
			if(a>=0 && a<=1)
			{
				pwmDma_clearPort(channel,servoPort);
				pwmDma_addPulse(channel,servoPort,0,1000 + 1000*a);
			}
			else if(a==-100)
			{
				pwmDma_clearPort(channel,servoPort);
			}
			else
			{
				printf("invalid ratio");
			}
		}
		else if(sm=='*')
		{
			pwmDma_clearPort(channel,motorPort1);
			pwmDma_clearPort(channel,motorPort2);
			pwmDma_clearPort(channel,servoPort);
		}
		else if(sm=='q')
		{
			break;
		}
	}

	//getchar();
	pwmDma_freeChannel(channel);
	gpio_setFunction(servoPort,PORT_FUNCTION_INPUT);
	gpio_setFunction(motorPort1,PORT_FUNCTION_INPUT);
	gpio_setFunction(motorPort2,PORT_FUNCTION_INPUT);
	//gpio_setFunction(18,PORT_FUNCTION_INPUT);;

}

/**
 * @brief Similar to pwmByDmaTestServoAndMotor but wit a predefined sequence of commands
 */
int demo1()
{
	gpio_init();
	if(0>pwmDma_init(1))
		return ERROR("can't initialize pwmByDma",-1);

	int servoPort = 26;
	int motorPort1 = 17;
	int motorPort2 = 27;
	gpio_setFunction(servoPort,PORT_FUNCTION_OUTPUT);
	gpio_setFunction(motorPort1,PORT_FUNCTION_OUTPUT);
	gpio_setFunction(motorPort2,PORT_FUNCTION_OUTPUT);
	//gpio_setFunction(18,PORT_FUNCTION_ALT_5);;

	int channel = 5;

	pwmDma_initChannel(channel,15000);//period of 10*256us
	char sm;

	int cmd=0;
	char* commands[] = {"s 0.4\n",
	"m 0.1\n",
	"m\n",
	"s 0.05\n",
	"m 0.1\n",
	"m\n",
	"s 0.8\n",
	"m -0.1\n",
	"m\n",
	"s 0.4\n",
	"m -0.1\n",
	"m\n",
	"m 0.4\n",
	"\n",
	"q\n"};

	while(1)
	{
		double a;
		getchar();
		char*s = commands[cmd++];
		int n = sscanf(s," %c %lf",&sm,&a);
		if(n<2)
			a = -100;
		if(n<1)
		{
			a = -100;
			sm = '*';
		}
		printf("[%c %f]\n",sm,a);
		if(sm=='m')
		{
			//printf("motor: enter the ratio\n");
			//scanf("%lf",&a);
			if(a>=0 && a<=1)
			{
				pwmDma_clearPort(channel,motorPort2);
				pwmDma_clearPort(channel,motorPort1);
				//pwmDma_printWords(channel);
				pwmDma_addMultiplePulse(channel,motorPort1,0,100*a,100);
				//pwmDma_printWords(channel);
			}
			else if(a>=-1 && a<0)
			{
				a = -a;
				pwmDma_clearPort(channel,motorPort1);
				pwmDma_clearPort(channel,motorPort2);

				pwmDma_addMultiplePulse(channel,motorPort2,0,100*a,100);
			}
			else if(a==-100)
			{
				pwmDma_clearPort(channel,motorPort1);
				pwmDma_clearPort(channel,motorPort2);
			}
		}
		else if(sm=='s')
		{
			//printf("servo: enter the ratio\n");
			//scanf("%lf",&a);
			if(a>=0 && a<=1)
			{
				pwmDma_clearPort(channel,servoPort);
				pwmDma_addPulse(channel,servoPort,0,1000 + 1000*a);
			}
			else if(a==-100)
			{
				pwmDma_clearPort(channel,servoPort);
			}
			else
			{
				printf("invalid ratio");
			}
		}
		else if(sm=='*')
		{
			pwmDma_clearPort(channel,motorPort1);
			pwmDma_clearPort(channel,motorPort2);
			pwmDma_clearPort(channel,servoPort);
		}
		else if(sm=='q')
		{
			break;
		}
	}

	//getchar();
	pwmDma_freeChannel(channel);
	gpio_setFunction(servoPort,PORT_FUNCTION_INPUT);
	gpio_setFunction(motorPort1,PORT_FUNCTION_INPUT);
	gpio_setFunction(motorPort2,PORT_FUNCTION_INPUT);
}


/**
 * the starting point for the demos
 */
int main(int argc, char *argv[])
{
	//printf("DMA usable channels: 0x%x\n",dma_getUsableChannels());

	int c=1;
	while(c)
	{
		int d = 0;
		printf("\
1:	dmaTest()\n\
2:	dmaToGpioTest()\n\
3:	dmaToGpioPacedTest()\n\
4:	pwmSerializerTest()\n\
5:	pwmTest()\n\
6:	simplePulseTest()\n\
7:	pwmByDmaTest(4)\n\
8:	pwmByDmaTestServo()\n\
9:	pwmByDmaTestServoAndMotor()\n\
10:	gpioTest()\n\
11:	demo1()\n\
12:	pwmByDmaTestVisible()\n\
13:	exit without running a demo\n");
		printf("Which demo do you want to try?\n");
		scanf(" %d",&d);

		switch(d)
		{
		case 1:
			dmaTest();
			c = 0;
			break;
		case 2:
			dmaToGpioTest();
			c = 0;
			break;
		case 3:
			dmaToGpioPacedTest();
			c = 0;
			break;
		case 4:
			pwmSerializerTest();
			c = 0;
			break;
		case 5:
			pwmTest();
			c = 0;
			break;
		case 6:
			simplePulseTest();
			c = 0;
			break;
		case 7:
			pwmByDmaTest(4);
			c = 0;
			break;
		case 8:
			pwmByDmaTestServo();
			c = 0;
			break;
		case 9:
			pwmByDmaTestServoAndMotor();
			c = 0;
			break;
		case 10:
			gpioTest();
			c = 0;
			break;
		case 11:
			demo1();
			c = 0;
			break;
		case 12:
			pwmByDmaTestVisible();
			c = 0;
			break;
		case 13:
			c = 0;
			break;
		default:
			printf("invalid option\n");
		}
	}
	return 0;
}
