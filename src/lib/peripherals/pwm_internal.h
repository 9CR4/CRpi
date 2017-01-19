#ifndef PWM_INTERNAL_H
#define PWM_INTERNAL_H

/**
 * @file pwm_internal.h
 *
 * @brief Helper macros for the pwm peripheral. Used internally by the library.
 *
 * See documentation at  "BCM2835 ARM peripherals" pag 138
 */

///Physical base address of the pwm peripheral
#define PWM_BASE_ADDR_PHYS	(peripheralsBaseAddressPhys+0x20c000)
///Size in bytes of the memory area dedicated to the pwm peripheral
#define PWM_AREA_LEN		0x28


///Control register offset
#define PWM_REG_CTL_OFF			0x0
///Control register physical address
#define PWM_REG_CTL_ADDR_PHYS		(PWM_REG_CTL_OFF + PWM_BASE_ADDR_PHYS)

///Status register offset
#define PWM_REG_STA_OFF			0x4
///Status register physical address
#define PWM_REG_STA_ADDR_PHYS		(PWM_REG_STA_OFF + PWM_BASE_ADDR_PHYS

///DMA configuration register offset
#define PWM_REG_DMAC_OFF		0x8
///DMA configuration register
#define PWM_REG_DMAC_ADDR_PHYS	(PWM_REG_DMAC_OFF + PWM_BASE_ADDR_PHYS)

///Channel 1 range register offset
#define PWM_REG_RNG1_OFF		0x10
///Channel 1 range register physical address
#define PWM_REG_RNG1_ADDR_PHYS	(PWM_REG_RNG1_OFF + PWM_BASE_ADDR_PHYS)

///Channel 1 data register offset
#define PWM_REG_DAT1_OFF		0x14
///Channel 1 data register physical address
#define PWM_REG_DAT1_ADDR_PHYS	(PWM_REG_DAT1_OFF + PWM_BASE_ADDR_PHYS)

///Fifo input register offset
#define PWM_REG_FIF1_OFF		0x18
///Fifo input register physical address
#define PWM_REG_FIF1_ADDR_PHYS	(PWM_REG_FIF1_OFF + PWM_BASE_ADDR_PHYS)

///Channel 2 range register offset
#define PWM_REG_RNG2_OFF		0x20
///Channel 2 range register physical address
#define PWM_REG_RNG2_ADDR_PHYS	(PWM_REG_RNG2_OFF + PWM_BASE_ADDR_PHYS)

///Channel 2 data register offset
#define PWM_REG_DAT2_OFF		0x24
///Channel 2 data register physical address
#define PWM_REG_DAT2_ADDR_PHYS	(PWM_REG_DAT2_OFF + PWM_BASE_ADDR_PHYS)



//::::REG_CTL : Control::::::::::::::::::::::::::::::::::::
//----Channel 1----
///Channel 1 Control Register mask: pwm enable
#define CTL_PWEN1_MASK	0x1
///Channel 1 Control Register mask: mode (0: pwm mode; 1:serializer mode)
#define CTL_MODE1_MASK	(1<<1)
///Channel 1 Control Register mask: Repeat Last Data (1=Transmission interrupts when fifo is empty; 0=Last data is retransmitter until fifo is not empty)
#define CTL_RPTL1_MASK	(1<<2)
///Channel 1 Control Register mask: Silence bit (state of the output when not transmitting)
#define CTL_SBIT1_MASK	(1<<3)
///Channel 1 Control Register mask: polarity (0: 0=low, 1=high; 1: 0=high 1=low)
#define CTL_POLA1_MASK	(1<<4)
///Channel 1 Control Register mask: Use Fifo (0:data register is transmitted ; 1: fifi is transmitted)
#define CTL_USEF1_MASK	(1<<5)
///Channel 1 Control Register mask: Clear Fifo (write 1 clears fifo; write 0 does nothing; read is always 0)
#define CTL_CLRF_MASK	(1<<6)
///Channel 1 Control Register mask: M/S Enable (0: use pwm algorithm; 1:use M/S transmission)
#define CTL_MSEN1_MASK	(1<<7)
//----Channel 2----
///Channel 2 Control Register mask: pwm enable
#define CTL_PWEN2_MASK	(1<<8)
///Channel 2 Control Register mask: mode (0: pwm mode; 1:serializer mode)
#define CTL_MODE2_MASK	(1<<9)
///Channel 2 Control Register mask: Repeat Last Data (1=Transmission interrupts when fifo is empty; 0=Last data is retransmitter until fifo is not empty)
#define CTL_RPTL2_MASK	(1<<10)
///Channel 2 Control Register mask: Silence bit (state of the output when not transmitting)
#define CTL_SBIT2_MASK	(1<<11)
///Channel 2 Control Register mask: polarity (0: 0=low, 1=high; 1: 0=high 1=low)
#define CTL_POLA2_MASK	(1<<12)
///Channel 2 Control Register mask: Use Fifo (0:data register is transmitted ; 1: fifi is transmitted)
#define CTL_USEF2_MASK	(1<<13)
///Channel 2 Control Register mask: M/S Enable (0: use pwm algorithm; 1:use M/S transmission)
#define CTL_MSEN2_MASK	(1<<15)

//::::REG_STA : Status::::::::::::::::::::::::::::::::::::::
///Status register mask: Fifo Full
#define STA_FULL1_MASK	0x1
///Status register mask: Fifo empty
#define STA_EMPT1_MASK	(1<<1)
///Status register mask: "Write when full" error
#define STA_WERR1_MASK	(1<<2)
///Status register mask: "Read when empty" error
#define STA_RERR1_MASK	(1<<3)
///Status register mask: Channel 1: Gap Occurred between transmission of two data in the fifo
#define STA_GAPO1_MASK	(1<<4)
///Status register mask: Channel 2: Gap Occurred between transmission of two data in the fifo
#define STA_GAPO2_MASK	(1<<5)
///Status register mask: Channel 3: Gap Occurred between transmission of two data in the fifo
#define STA_GAPO3_MASK	(1<<6)
///Status register mask: Channel 4: Gap Occurred between transmission of two data in the fifo
#define STA_GAPO4_MASK	(1<<7)
///Status register mask: Bus error
#define STA_BERR_MASK	(1<<8)
///Status register mask: Channel 1 state: 1=transmitting , 0=not transmitting
#define STA_STA1_MASK	(1<<9)
///Status register mask: Channel 2 state: 1=transmitting , 0=not transmitting
#define STA_STA2_MASK	(1<<10)
///Status register mask: Channel 3 state: 1=transmitting , 0=not transmitting
#define STA_STA3_MASK	(1<<11)
///Status register mask: Channel 4 state: 1=transmitting , 0=not transmitting
#define STA_STA4_MASK	(1<<12)


//::::REG_DMAC : DMA Configuration::::::::::::::::::::::::::
///Dma configuartion register mask: DMA Threshold for DREQ signal
#define DMAC_DREQ_MASK	(0xFF)
///Dma configuartion register mask: DMA Threshold for PANIC signal
#define DMAC_PANIC_MASK	(0xFF<<8)
///Dma configuartion register mask: DMA Enable
#define DMAC_ENAB_MASK	(0x1<<31)


//::::REG_RNG1 : Channel 1 Range::::::::::::::::::::::::::::
///DMA Channel 1 Range register mask: number of possible pulses in a period
#define RNG1_PWM_RNG1_MASK 0xFFFFFFFF

//::::REG_DAT1 : Channel 1 Data:::::::::::::::::::::::::::::
/**
 * @brief DMA Channel 1 Data register:
 *
 * Serializer mode: 	Data to be sent when USEF1=0
 * pwm mode:		Number of pulses in a period
 */
#define PWM_DAT1_MASK	0xFFFFFFFF


//::::REG_FIF1 : Fifo input:::::::::::::::::::::::::::::::::
///DMA FIFO input: input for the fifo, write here to insert into fifo
#define FIF1_PWM_FIFO_MASK 0xFFFFFFFF


//::::REG_RNG2 : Channel 2 Range::::::::::::::::::::::::::::
///DMA Channel 2 Range register mask: number of possible pulses in a period
#define RNG1_PWM_RNG2_MASK 0xFFFFFFFF

//::::REG_DAT2 : Channel 2 Data:::::::::::::::::::::::::::::
/**
 * @brief DMA Channel 2 Data register:
 *
 * Serializer mode: 	Data to be sent when USEF1=0
 * pwm mode:		Number of pulses in a period
 */
#define PWM_DAT2_MASK	0xFFFFFFFF


#endif // PWM_INTERNAL_H
