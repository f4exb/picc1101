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
//  Description:  Header file for PI_CC_spi.c
//
//  MSP430/CC1100-2500 Interface Code Library v1.1
//
//  W. Goh
//  Texas Instruments, Inc.
//  December 2009
//  IAR Embedded Workbench v4.20
//
//  E. Griffiths
//  F4EXB, Amateur radio
//  Adapted to Raspberry-Pi
//
//------------------------------------------------------------------------------
// Change Log:
//------------------------------------------------------------------------------
// 
// TI_CC_spi.h 
//
// Version:  1.1
// Comments: Fixed function bugs
//           Added support for 5xx
//
// Version:  1.00
// Comments: Initial Release Version
//------------------------------------------------------------------------------
#ifndef _PI_CC_SPI_H_
#define _PI_CC_SPI_H_


#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "main.h"

typedef struct spi_parms_s
{
    uint8_t  mode;
    uint8_t  bits;
    uint32_t speed;
    uint16_t delay;
    int      fd;
    int      ret;
    struct   spi_ioc_transfer tr;
    uint8_t  tx[65]; // max 1 command byte + 64 bytes FIFO
    uint8_t  rx[65]; // max 1 status byte + 64 bytes FIFO
} spi_parms_t;

void PI_CC_SPIParmsDefaults(spi_parms_t *spi_parms);
void PI_CC_Wait(unsigned int);
int  PI_CC_SPISetup(spi_parms_t *spi_parms, arguments_t *arguments);
int  PI_CC_SPIWriteReg(spi_parms_t *spi_parms, uint8_t addr, uint8_t byte);
int  PI_CC_SPIWriteBurstReg(spi_parms_t *spi_parms, uint8_t addr, const uint8_t *bytes, uint8_t count);
int  PI_CC_SPIReadReg(spi_parms_t *spi_parms, uint8_t addr, uint8_t *byte);
int  PI_CC_SPIReadBurstReg(spi_parms_t *spi_parms, uint8_t addr, uint8_t **bytes, uint8_t count);
int  PI_CC_SPIReadStatus(spi_parms_t *spi_parms, uint8_t addr, uint8_t *status);
int  PI_CC_SPIStrobe(spi_parms_t *spi_parms, uint8_t strobe);
int  PI_CC_PowerupResetCCxxxx(spi_parms_t *spi_parms);

#endif