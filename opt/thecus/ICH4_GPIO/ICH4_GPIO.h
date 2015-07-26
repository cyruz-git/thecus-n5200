/*
 *  Copyright (C) 2006 Thecus Technology Corp. 
 *
 *      Written by Y.T. Lee (yt_lee@thecus.com)
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Driver for ICH4 GPIO and LED/Button contrl of Thecus N5200
 */
/* PCI registers */

#define GPIOBASE 0x58
#define GPIO_CNTL 0x5c
#define GPIO_CNTL_EN 4

#define PMBASE 0x40
#define GPI_ROUT 0xb8

#define GPE0_STS 0x28
#define GPE0_EN 0x2c
#define GPIO_PM_CON_2 0xa2
#define GPIO_PM_CON_3 0xa4

/* IO registers */
#define GPIO_USE_SEL 0x0
#define GP_IO_SEL 0x4
#define GP_LVL 0xc

#define GPO_BLINK 0x18

#define GPIO_USE_SEL2 0x30
#define GP_IO_SEL2 0x34
#define GP_LVL2 0x38

#define GPIO_IO_PORTS 64

#define ICH4_IN_COPY_BTN 0x4
#define ICH4_IN_DB_DET 0x7

#define LED_OFF 1
#define LED_ON 0


#define Copy_BTN 36
#define DB_DET 7

#define LED_Copy 32
#define LED_Fail 17
#define LED_Busy 35

#define RST_INT 37
#define RST_51 38

#define GP25 25
#define GP27 27
#define GP33 33
#define GP34 34
// ------------------------- for Sata Disk Fail Led
#define GP39 39
#define GP40 40
#define GP41 41
#define GP42 42
#define GP43 43
#define GP44 44
// ------------------------------------------------

#define GPE0_STS 0x28

#define PM1_STS 0x0
#define PM1_EN 0x2
#define PM1_CNT 0x4
#define PWR_BTN_BIT 8

/* SMSC IO defines below */
#define	REG	0x2e	/* The register to read/write */
#define	VAL	0x2f	/* The value to read/write */
#define SUPERIO_REG_ACT		0x30
#define SUPERIO_REG_BASE	0x60
#define SUPERIO_REG_DEVID	0x20

/* Logical device registers (0x800~0x83f)*/
#define ICH4_LPC_SIO 0x2E
#define ICH4_LPC_SIO_PORTS 2
#define SMSC_47m182_DEV_ID 0x74
#define SMSC_EXTENT		0x40
#define REG_LDNUM 0x29
#define LDNUM_0 0x0
#define REG_LD_SEL 0x7
#define GPIO_LD 0x7
#define PM_LD 0x4

#define SMSC_CNF_START 0x55
#define SMSC_CNF_END 0xAA

#define REG_GPIO16 0x6
#define REG_GPIO17 0x7

#define PME_Enable 0x5

#define TACH1_LSB 0x12
#define TACH1_MSB 0x13
#define TACH2_LSB 0x14
#define TACH2_MSB 0x15

