#ifndef PWM_BY_DMA_H
#define PWM_BY_DMA_H



/**
 * @file pwmByDma.h
 *
 * @brief Generates pwm outputs on the gpio pins using the dma engine.
 *
 * To generate a pwm output you first have to call pwmDma_init().\n
 * Then you can initilize a channel with pwmDma_initChannel().\n
 * After this you can add pulses to the channel using pwmDma_addPulse(), pwmDma_addMultiplePulse() or pwmDma_setMSRatio().\n
 * You can clear the output on a port using pwmDma_clearPort()\n
 * When you finish you HAVE TO call pwmDma_freeChannel() on the channel you initialized.\n
 *
 * See pwmByDma.c for more details.
 */

int pwmDma_init(unsigned int pulseDuration_us);

unsigned int pwmDma_get_pulseDuration_us();

int pwmDma_isChannelInitialized(int channel);

int pwmDma_getChannelPeriodUs(int channel);

int pwmDma_initChannel_periodInPulses(int channel, unsigned int pulsesPerPeriod);

int pwmDma_initChannel(int channel, unsigned int period_us);

int pwmDma_freeChannel(int channel);

int pwmDma_startChannel(int channel);

int pwmDma_stopChannelWaitCycleEnd(int channel);

int pwmDma_stopChannel(int channel);

int pwmDma_addPulse(int channel, int bcmPort, unsigned int start_us, unsigned int length_us);

int pwmDma_printWords(int channel);

int pwmDma_printControlBlocks(int channel);

int pwmDma_setMSRatio(int channel, int bcmPort, double ratio);

int pwmDma_clearPort(int channel, int bcmPort);

int pwmDma_addMultiplePulse(int channel, int bcmPort, unsigned int start_us, unsigned int length_us, unsigned int subPeriod_us);




#endif
