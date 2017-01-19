#ifndef GPIO_INTERNAL_H
#define GPIO_INTERNAL_H

/**
 * @file gpio_internal.h
 *
 * @brief Helper macros for the gpio peripheral. Used internally by the library.
 *
 */

///Physical base address of the gpio peripheral
#define GPIO_BASE_ADDR_PHYS (peripheralsBaseAddressPhys+0x200000)
///Size in bytes of the memory area dedicated to the gpio peripheral
#define GPIO_AREA_LEN	0xC0

//function select registers
//there's 6 of these register, each referring to 6 pins, except the last one
///Offset of the function select registers
#define GPFSEL_OFF 0
///Size in bytes of a function select register
#define GPFSEL_LEN 4
///Offset of the specified function select register
#define GPFSELx_OFF(n) (GPFSEL_OFF+GPFSEL_LEN*n)






//@{
///Set register 0 (ports 0-31): setting a bit to 1 sets the corresponding port to high
#define GPSET0_OFF		0x1c
#define GPSET0_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPSET0_OFF)
//@}
//@{
///Set register 0 (ports 32-53): setting a bit to 1 sets the corresponding port to high
#define GPSET1_OFF		0x20
#define GPSET1_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPSET1_OFF)
//@}

//@{
///Clear register 0 (ports 0-31): setting a bit to 1 sets the corresponding port to low
#define GPCLEAR0_OFF		0x28
#define GPCLEAR0_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPCLEAR0_OFF)
//@}
//@{
///Clear register 0 (ports 32-53): setting a bit to 1 sets the corresponding port to low
#define GPCLEAR1_OFF		0x2C
#define GPCLEAR1_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPCLEAR1_OFF)
//@}

//@{
///Level Register 0 (ports 0-31): each bit reflects the high/low status of the corresponding port in input mode
#define GPLEV0_OFF		0x34
#define GPLEV0_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPLEV0_OFF)
//@}
//@{
///Level Register 1 (ports 32-53): each bit reflects the high/low status of the corresponding port in input mode
#define GPLEV1_OFF		0x38
#define GPLEV1_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPLEV1_OFF)
//@}

//@{
///Event detect status register 0 (ports 0-31): each bit says if an event has been detectd on the corresponding port
#define GPEDS0_OFF		0x40
#define GPEDS0_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPEDS0_OFF)
//@}
//@{
///Event detect status register 1 (ports 32-53): each bit says if an event has been detectd on the corresponding port
#define GPEDS1_OFF		0x44
#define GPEDS1_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPEDS1_OFF)
//@}


//@{
///pin rising edge detect enable registers (ports 0-31)
#define GPREN0_OFF		0x4C
#define GPREN0_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPREN0_OFF)
//@}
//@{
///pin rising edge detect enable registers (ports 32-53)
#define GPREN1_OFF		0x50
#define GPREN1_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPREN1_OFF)
//@}

//@{
///pin falling edge detect enable registers (ports 0-31)
#define GPFEN0_OFF		0x58
#define GPFEN0_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPFEN0_OFF)
//@}
//@{
///pin falling edge detect enable registers (ports 32-53)
#define GPFEN1_OFF		0x5C
#define GPFEN1_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPFEN1_OFF)
//@}

//@{
///pin high detect registers (ports 0-31)
#define GPHEN0_OFF		0x64
#define GPHEN0_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPHEN0_OFF)
//@}
//@{
///pin high detect registers (ports 32-53)
#define GPHEN1_OFF		0x68
#define GPHEN1_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPHEN1_OFF)
//@}

//@{
///pin low detect registers (ports 0-31)
#define GPLEN0_OFF		0x70
#define GPLEN0_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPLEN0_OFF)
//@}
//@{
///pin low detect registers (ports 32-53)
#define GPLEN1_OFF		0x74
#define GPLEN1_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPLEN1_OFF)
//@}

//@{
///pin async rising edge detect registers (ports 0-31)
#define GPAREN0_OFF		0x7C
#define GPAREN0_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPAREN0_OFF)
//@}
//@{
///pin async rising edge detect registers (ports 32-53)
#define GPAREN1_OFF		0x80
#define GPAREN1_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPAREN1_OFF)
//@}

//@{
///pin async falling edge detect registers (ports 0-31)
#define GPAFEN0_OFF		0x88
#define GPAFEN0_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPAFEN0_OFF)
//@}
//@{
///pin async falling edge detect registers (ports 32-53)
#define GPAFEN1_OFF		0x8C
#define GPAFEN1_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPAFEN1_OFF)
//@}

//@{
///pullup pulldown enable registers (see documentation: BCM2835 arm peripherals page 101)
#define GPPUD_OFF		0x94
#define GPPUD_ADDR_PHYS		(GPIO_BASE_ADDR_PHYS+GPPUD_OFF)
//@}

//@{
///pullup pulldown enable clock registers (ports 0-31) (see documentation: BCM2835 arm peripherals page 101)
#define GPPUDCLK0_OFF		0x98
#define GPPUDCLK0_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPPUDCLK0_OFF)
//@}
//@{
///pullup pulldown enable clock registers (ports 32-53) (see documentation: BCM2835 arm peripherals page 101)
#define GPPUDCLK1_OFF		0x9C
#define GPPUDCLK1_ADDR_PHYS	(GPIO_BASE_ADDR_PHYS+GPPUDCLK1_OFF)
//@}

#endif
