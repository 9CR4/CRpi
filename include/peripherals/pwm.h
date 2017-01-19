#ifndef PWM_H
#define PWM_H




/**
 * @file pwm.h
 *
 * @brief 	Interface to the PWM peripheral
 *
 * This peripheral is able to generate PWM signals. It has 3 modes:\n
 *	- Serializer mode: 	it seriallly outputs the bits of 32bit words, you can set the input to the data register or the fifo
 *	- M/S PWM:			it outputs a pwm signal  generated this way: it increments ciclically a counter, while it's lower to the setted threshold the output is high, while higher it's low
 *	- not-M/S PWM:		it mantains the same duty cycle as M/S but it spreads the high part in little pulses
 * See documentation at  "BCM2835 ARM peripherals" pag 138
 */


int pwm_init();

int pwm_isInit();

int pwm_resetChannel(int channel);

int pwm_clearFifo();

int pwm_setEnable(int channel, unsigned int value);

int pwm_setMode(int channel, unsigned int mode);

int pwm_setMSenable(int channel, unsigned int mode);

int pwm_setRange(int channel, uint32_t range);

int pwm_setData(int channel, uint32_t data);

int pwm_setClock(int clkSrc, int divisorInt, int divisorFrac);

int pwm_writeToFifo(unsigned int data);

int pwm_setDreqThreshold(unsigned int threshold);

int pwm_setSerializerMode(int channel,unsigned int mode);

int pwm_setDmaEnable(unsigned int value);

int pwm_setRepeatLastWord(int channel, unsigned int value);






#endif
