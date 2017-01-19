#ifndef CLOCK_MANAGER_H
#define CLOCK_MANAGER_H


/**
 * @file clockManager.h
 *
 * @brief Interface to the clock manager peripheral.
 *
 * This peripheral controls various clocks in the BCM283x SoCs, for each of these clocks you can set a clock source and a frequency divider.\n
 * See the CLK_SRC_* macros for the available clock sources and their speed.
 */

//CLOCK SOURCES
/// Clock source: Ground: 0Hz
#define CLK_SRC_GND		0
/// Clock source: Oscillator: 19.2MHz
#define CLK_SRC_OSCILLATOR	1
/// Clock source: Testdebug0: 0Hz
#define CLK_SRC_TESTDBG0	2
/// Clock source: TestDebug1: 0Hz
#define CLK_SRC_TESTDBG1	3
/// Clock source: PLLA: 0Hz
#define CLK_SRC_PLLA		4
/// Clock source: PLLC: 1000MHz (changes with overclock)
#define CLK_SRC_PLLC		5
/// Clock source: PLLD: 500MHz
#define CLK_SRC_PLLD		6
/// Clock source: HDMI: 216MHZ
#define CLK_SRC_HDMI		7

int clkMan_init();

int clkMan_isInit();

int clkMan_setClock_gp0(int clockSrc, int divisorInt, int divisorFrac);

int clkMan_setClock_gp1(int clockSrc, int divisorInt, int divisorFrac);

int clkMan_setClock_gp2(int clockSrc, int divisorInt, int divisorFrac);

int clkMan_setClock_pcm(int clockSrc, int divisorInt, int divisorFrac);

int clkMan_setClock_pwm(int clockSrc, int divisorInt, int divisorFrac);



#endif
