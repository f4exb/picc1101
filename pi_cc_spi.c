/* --COPYRIGHT--,BSD
 * Copyright (c) 2011, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
//------------------------------------------------------------------------------
//  Description:  This file contains functions that allow the MSP430 device to
//  access the SPI interface of the CC1100/CC2500.  There are multiple
//  instances of each function; the one to be compiled is selected by the
//  system variable TI_CC_RF_SER_INTF, defined in "TI_CC_hardware_board.h".
//
//  MSP430/CC1100-2500 Interface Code Library v1.1
//
//  W. Goh
//  Texas Instruments, Inc.
//  December 2009
//  IAR Embedded Workbench v4.20
//------------------------------------------------------------------------------
// Change Log:
//------------------------------------------------------------------------------
// Version:  1.1
// Comments: Fixed several bugs where it is stuck in a infinite while loop
//           Added support for 5xx
//
// Version:  1.00
// Comments: Initial Release Version
//------------------------------------------------------------------------------

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>


#include "pi_cc_spi.h"
#include "pi_cc_cc1100-cc2500.h"

//------------------------------------------------------------------------------
//  void TI_CC_SPISetup(void)
//
//  DESCRIPTION:
//  Configures the assigned interface to function as a SPI port and
//  initializes it.
//------------------------------------------------------------------------------
//  void TI_CC_SPIWriteReg(char addr, char value)
//
//  DESCRIPTION:
//  Writes "value" to a single configuration register at address "addr".
//------------------------------------------------------------------------------
//  void TI_CC_SPIWriteBurstReg(char addr, char *buffer, char count)
//
//  DESCRIPTION:
//  Writes values to multiple configuration registers, the first register being
//  at address "addr".  First data byte is at "buffer", and both addr and
//  buffer are incremented sequentially (within the CCxxxx and MSP430,
//  respectively) until "count" writes have been performed.
//------------------------------------------------------------------------------
//  char TI_CC_SPIReadReg(char addr)
//
//  DESCRIPTION:
//  Reads a single configuration register at address "addr" and returns the
//  value read.
//------------------------------------------------------------------------------
//  void TI_CC_SPIReadBurstReg(char addr, char *buffer, char count)
//
//  DESCRIPTION:
//  Reads multiple configuration registers, the first register being at address
//  "addr".  Values read are deposited sequentially starting at address
//  "buffer", until "count" registers have been read.
//------------------------------------------------------------------------------
//  char TI_CC_SPIReadStatus(char addr)
//
//  DESCRIPTION:
//  Special read function for reading status registers.  Reads status register
//  at register "addr" and returns the value read.
//------------------------------------------------------------------------------
//  void TI_CC_SPIStrobe(char strobe)
//
//  DESCRIPTION:
//  Special write function for writing to command strobe registers.  Writes
//  to the strobe at address "addr".
//------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// Initialize defauklt parameters
void PI_CC_SPIParmsDefaults(spi_parms_t *spi_parms)
// ------------------------------------------------------------------------------------------------
{
    spi_parms->mode             = 0;
    spi_parms->bits             = 8;
    spi_parms->speed            = 1000000;
    spi_parms->delay            = 4;
    spi_parms->fd               = 0;
    spi_parms->ret              = 0;
    
    spi_parms->tr.tx_buf        = (unsigned long) spi_parms->tx;
    spi_parms->tr.rx_buf        = (unsigned long) spi_parms->rx;
    spi_parms->tr.len           = 0;
    spi_parms->tr.delay_usecs   = 0;
    spi_parms->tr.speed_hz      = 1000000;
    spi_parms->tr.bits_per_word = 4;
}

// ------------------------------------------------------------------------------------------------
// Delay function. # of CPU cycles delayed is similar to "cycles". Specifically,
// it's ((cycles-15) % 6) + 15.  Not exact, but gives a sense of the real-time
// delay.  Also, if MCLK ~1MHz, "cycles" is similar to # of useconds delayed.
void PI_CC_Wait(unsigned int cycles)
// ------------------------------------------------------------------------------------------------
{
    while(cycles>15)                        // 15 cycles consumed by overhead
        cycles = cycles - 6;                // 6 cycles consumed each iteration
}

// ------------------------------------------------------------------------------------------------
int PI_CC_SPISetup(spi_parms_t *spi_parms, arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    spi_parms->ret = 0;

    do
    {
        spi_parms->fd = open(arguments->spi_device, O_RDWR);
        if (spi_parms->fd < 0)
        {
            perror("SPI: can't open device");
            spi_parms->ret = -1;
            break;
        }

        // spi mode
        spi_parms->ret = ioctl(spi_parms->fd, +SPI_IOC_WR_MODE, &spi_parms->mode);
        if (spi_parms->ret == -1)
        {
            fprintf(stderr, "SPI: can't set spi mode\n");
            break;
        }

        spi_parms->ret = ioctl(spi_parms->fd, SPI_IOC_RD_MODE, &spi_parms->mode);
        if (spi_parms->ret == -1)
        {
            fprintf(stderr, "SPI: can't get spi mode\n");
            break;
        }

        // bits per word
        spi_parms->ret = ioctl(spi_parms->fd, SPI_IOC_WR_BITS_PER_WORD, &spi_parms->bits);
        if (spi_parms->ret == -1)
        {
            fprintf(stderr, "SPI: can't set bits per word\n");
            break;
        }

        spi_parms->ret = ioctl(spi_parms->fd, SPI_IOC_RD_BITS_PER_WORD, &spi_parms->bits);
        if (spi_parms->ret == -1)
        {
            fprintf(stderr, "SPI: can't get bits per word\n");
            break;
        }

        // max speed hz
        spi_parms->ret = ioctl(spi_parms->fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_parms->speed);
        if (spi_parms->ret == -1)
        {
            fprintf(stderr, "SPI: can't set max speed hz\n");
            break;
        }

        spi_parms->ret = ioctl(spi_parms->fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_parms->speed);
        if (spi_parms->ret == -1)
        {
            fprintf(stderr, "SPI: can't get max speed hz\n");
            break;
        }

        spi_parms->tr.delay_usecs = spi_parms->delay;
        spi_parms->tr.speed_hz = spi_parms->speed;
        spi_parms->tr.bits_per_word = spi_parms->bits;

        if (!spi_parms->ret)
        {
            fprintf(stderr, "-- SPI --\n");
            fprintf(stderr, "SPI mode ............: %d\n", spi_parms->mode);
            fprintf(stderr, "Bits per word .......: %d\n", spi_parms->bits);
            fprintf(stderr, "Interbyte delay .....: %d us\n", spi_parms->delay);
            fprintf(stderr, "Max speed ...........: %d Hz (%d KHz)\n", spi_parms->speed, spi_parms->speed/1000);
        }
    }
    while(0);

    return spi_parms->ret;
}

// ------------------------------------------------------------------------------------------------
int PI_CC_SPIWriteReg(spi_parms_t *spi_parms, uint8_t addr, uint8_t value)
// ------------------------------------------------------------------------------------------------
{
    spi_parms->tx[0] = addr;
    spi_parms->tx[1] = value;
    spi_parms->tr.len = 2;

    spi_parms->ret = ioctl(spi_parms->fd, SPI_IOC_MESSAGE(1), &spi_parms->tr);

    if (spi_parms->ret < 1)
    {
        fprintf(stderr, "SPI: can't send write register\n");
        return 1;
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
int PI_CC_SPIWriteBurstReg(spi_parms_t *spi_parms, uint8_t addr, const uint8_t *buffer, uint8_t count)
// ------------------------------------------------------------------------------------------------
{
    uint8_t i;

    count %= 64;
    spi_parms->tx[0] = addr | PI_CCxxx0_WRITE_BURST;   // Send address

    for (i=1; i<count+1; i++)
    {
        spi_parms->tx[i] = buffer[i-1];
    }

    spi_parms->tr.len = count+1;
    spi_parms->ret = ioctl(spi_parms->fd, SPI_IOC_MESSAGE(1), &spi_parms->tr);

    if (spi_parms->ret < 1)
    {
        fprintf(stderr, "SPI: can't send write burst register\n");
        return 1;
    }

    return spi_parms->ret; // returns length sent
}

// ------------------------------------------------------------------------------------------------
int PI_CC_SPIReadReg(spi_parms_t *spi_parms, uint8_t addr, uint8_t *byte)
// ------------------------------------------------------------------------------------------------
{
    spi_parms->tx[0] = addr | PI_CCxxx0_READ_SINGLE; // Send address
    spi_parms->tx[1] = 0; // Dummy write so we can read data
    spi_parms->tr.len = 2;

    spi_parms->ret = ioctl(spi_parms->fd, SPI_IOC_MESSAGE(1), &spi_parms->tr);

    if (spi_parms->ret < 1)
    {
        fprintf(stderr, "SPI: can't send read register\n");
        return 1;
    }

    *byte = spi_parms->rx[1];
    return 0;
}

// ------------------------------------------------------------------------------------------------
int PI_CC_SPIReadBurstReg(spi_parms_t *spi_parms, uint8_t addr, uint8_t **buffer, uint8_t count)
// ------------------------------------------------------------------------------------------------
{
    uint8_t i;

    count %= 64;
    spi_parms->tx[0] = addr | PI_CCxxx0_READ_BURST;   // Send address

    for (i=1; i<count+1; i++)
    {
        spi_parms->tx[i] = 0; // Dummy write so we can read data
    }

    spi_parms->tr.len = count+1;
    spi_parms->ret = ioctl(spi_parms->fd, SPI_IOC_MESSAGE(1), &spi_parms->tr);

    if (spi_parms->ret < 1)
    {
        fprintf(stderr, "SPI: can't send read burst register\n");
        return 1;
    }

    *buffer = &spi_parms->rx[1];
    return 0;
}

// ------------------------------------------------------------------------------------------------
// For status/strobe addresses, the BURST bit selects between status registers
// and command strobes.
int PI_CC_SPIReadStatus(spi_parms_t *spi_parms, uint8_t addr, uint8_t *status)
// ------------------------------------------------------------------------------------------------
{
    spi_parms->tx[0] = addr | PI_CCxxx0_READ_BURST;   // Send address
    spi_parms->tx[1] = 0; // Dummy write so we can read data
    spi_parms->tr.len = 2;

    spi_parms->ret = ioctl(spi_parms->fd, SPI_IOC_MESSAGE(1), &spi_parms->tr);

    if (spi_parms->ret < 1)
    {
        fprintf(stderr, "SPI: can't send read status register\n");
        return 1;
    }

    *status = spi_parms->rx[1];
    return 0;
}

// ------------------------------------------------------------------------------------------------
int PI_CC_SPIStrobe(spi_parms_t *spi_parms, uint8_t strobe)
// ------------------------------------------------------------------------------------------------
{
    spi_parms->tx[0] = strobe;   // Send strobe
    spi_parms->tr.len = 1;

    spi_parms->ret = ioctl(spi_parms->fd, SPI_IOC_MESSAGE(1), &spi_parms->tr);

    if (spi_parms->ret < 1)
    {
        fprintf(stderr, "SPI: can't send strobe %02x", strobe);
        return 1;
    }

    return 0;
}


// ------------------------------------------------------------------------------------------------
int PI_CC_PowerupResetCCxxxx(spi_parms_t *spi_parms)
// ------------------------------------------------------------------------------------------------
{
    return PI_CC_SPIStrobe(spi_parms, PI_CCxxx0_SRES);
}

