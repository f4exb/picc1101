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
//----------------------------------------------------------------------------
//  Description:  This file contains definitions specific to the CC1100/2500.
//  The configuration registers, strobe commands, and status registers are 
//  defined, as well as some common masks for these registers.
//
//  MSP430/CC1100-2500 Interface Code Library v1.0
//
//  K. Quiring
//  Texas Instruments, Inc.
//  July 2006
//  IAR Embedded Workbench v3.41
//
//  Renamed as PI_xxx... For Raspberry-Pi SPI interface
//  E.Griffiths
//  February 2015
//  F4EXB, Amateur radio.
//
//----------------------------------------------------------------------------

#ifndef _PI_CC_CC1100_CC2500_H_
#define _PI_CC_CC1100_CC2500_H_

// Configuration Registers
#define PI_CCxxx0_IOCFG2       0x00        // GDO2 output pin configuration
#define PI_CCxxx0_IOCFG1       0x01        // GDO1 output pin configuration
#define PI_CCxxx0_IOCFG0       0x02        // GDO0 output pin configuration
#define PI_CCxxx0_FIFOTHR      0x03        // RX FIFO and TX FIFO thresholds
#define PI_CCxxx0_SYNC1        0x04        // Sync word, high byte
#define PI_CCxxx0_SYNC0        0x05        // Sync word, low byte
#define PI_CCxxx0_PKTLEN       0x06        // Packet length
#define PI_CCxxx0_PKTCTRL1     0x07        // Packet automation control
#define PI_CCxxx0_PKTCTRL0     0x08        // Packet automation control
#define PI_CCxxx0_ADDR         0x09        // Device address
#define PI_CCxxx0_CHANNR       0x0A        // Channel number
#define PI_CCxxx0_FSCTRL1      0x0B        // Frequency synthesizer control
#define PI_CCxxx0_FSCTRL0      0x0C        // Frequency synthesizer control
#define PI_CCxxx0_FREQ2        0x0D        // Frequency control word, high byte
#define PI_CCxxx0_FREQ1        0x0E        // Frequency control word, middle byte
#define PI_CCxxx0_FREQ0        0x0F        // Frequency control word, low byte
#define PI_CCxxx0_MDMCFG4      0x10        // Modem configuration
#define PI_CCxxx0_MDMCFG3      0x11        // Modem configuration
#define PI_CCxxx0_MDMCFG2      0x12        // Modem configuration
#define PI_CCxxx0_MDMCFG1      0x13        // Modem configuration
#define PI_CCxxx0_MDMCFG0      0x14        // Modem configuration
#define PI_CCxxx0_DEVIATN      0x15        // Modem deviation setting
#define PI_CCxxx0_MCSM2        0x16        // Main Radio Cntrl State Machine config
#define PI_CCxxx0_MCSM1        0x17        // Main Radio Cntrl State Machine config
#define PI_CCxxx0_MCSM0        0x18        // Main Radio Cntrl State Machine config
#define PI_CCxxx0_FOCCFG       0x19        // Frequency Offset Compensation config
#define PI_CCxxx0_BSCFG        0x1A        // Bit Synchronization configuration
#define PI_CCxxx0_AGCCTRL2     0x1B        // AGC control
#define PI_CCxxx0_AGCCTRL1     0x1C        // AGC control
#define PI_CCxxx0_AGCCTRL0     0x1D        // AGC control
#define PI_CCxxx0_WOREVT1      0x1E        // High byte Event 0 timeout
#define PI_CCxxx0_WOREVT0      0x1F        // Low byte Event 0 timeout
#define PI_CCxxx0_WORCTRL      0x20        // Wake On Radio control
#define PI_CCxxx0_FREND1       0x21        // Front end RX configuration
#define PI_CCxxx0_FREND0       0x22        // Front end TX configuration
#define PI_CCxxx0_FSCAL3       0x23        // Frequency synthesizer calibration
#define PI_CCxxx0_FSCAL2       0x24        // Frequency synthesizer calibration
#define PI_CCxxx0_FSCAL1       0x25        // Frequency synthesizer calibration
#define PI_CCxxx0_FSCAL0       0x26        // Frequency synthesizer calibration
#define PI_CCxxx0_RCCTRL1      0x27        // RC oscillator configuration
#define PI_CCxxx0_RCCTRL0      0x28        // RC oscillator configuration
#define PI_CCxxx0_FSTEST       0x29        // Frequency synthesizer cal control
#define PI_CCxxx0_PTEST        0x2A        // Production test
#define PI_CCxxx0_AGCTEST      0x2B        // AGC test
#define PI_CCxxx0_TEST2        0x2C        // Various test settings
#define PI_CCxxx0_TEST1        0x2D        // Various test settings
#define PI_CCxxx0_TEST0        0x2E        // Various test settings

// Strobe commands
#define PI_CCxxx0_SRES         0x30        // Reset chip.
#define PI_CCxxx0_SFSTXON      0x31        // Enable/calibrate freq synthesizer
#define PI_CCxxx0_SXOFF        0x32        // Turn off crystal oscillator.
#define PI_CCxxx0_SCAL         0x33        // Calibrate freq synthesizer & disable
#define PI_CCxxx0_SRX          0x34        // Enable RX.
#define PI_CCxxx0_STX          0x35        // Enable TX.
#define PI_CCxxx0_SIDLE        0x36        // Exit RX / TX
#define PI_CCxxx0_SAFC         0x37        // AFC adjustment of freq synthesizer
#define PI_CCxxx0_SWOR         0x38        // Start automatic RX polling sequence
#define PI_CCxxx0_SPWD         0x39        // Enter pwr down mode when CSn goes hi
#define PI_CCxxx0_SFRX         0x3A        // Flush the RX FIFO buffer.
#define PI_CCxxx0_SFTX         0x3B        // Flush the TX FIFO buffer.
#define PI_CCxxx0_SWORRST      0x3C        // Reset real time clock.
#define PI_CCxxx0_SNOP         0x3D        // No operation.

// Status registers
#define PI_CCxxx0_PARTNUM      0x30        // Part number
#define PI_CCxxx0_VERSION      0x31        // Current version number
#define PI_CCxxx0_FREQEST      0x32        // Frequency offset estimate
#define PI_CCxxx0_LQI          0x33        // Demodulator estimate for link quality
#define PI_CCxxx0_RSSI         0x34        // Received signal strength indication
#define PI_CCxxx0_MARCSTATE    0x35        // Control state machine state
#define PI_CCxxx0_WORTIME1     0x36        // High byte of WOR timer
#define PI_CCxxx0_WORTIME0     0x37        // Low byte of WOR timer
#define PI_CCxxx0_PKTSTATUS    0x38        // Current GDOx status and packet status
#define PI_CCxxx0_VCO_VC_DAC   0x39        // Current setting from PLL cal module
#define PI_CCxxx0_TXBYTES      0x3A        // Underflow and # of bytes in TXFIFO
#define PI_CCxxx0_RXBYTES      0x3B        // Overflow and # of bytes in RXFIFO
#define PI_CCxxx0_NUM_RXBYTES  0x7F        // Mask "# of bytes" field in _RXBYTES

// Other memory locations
#define PI_CCxxx0_PATABLE      0x3E
#define PI_CCxxx0_TXFIFO       0x3F
#define PI_CCxxx0_RXFIFO       0x3F

// Masks for appended status bytes
#define PI_CCxxx0_LQI_RX       0x01        // Position of LQI byte
#define PI_CCxxx0_CRC_OK       0x80        // Mask "CRC_OK" bit within LQI byte

// Definitions to support burst/single access:
#define PI_CCxxx0_WRITE_BURST  0x40
#define PI_CCxxx0_READ_SINGLE  0x80
#define PI_CCxxx0_READ_BURST   0xC0

// Various constants
#define PI_CCxxx0_FIFO_SIZE         64     // Rx or Tx FIFO size
#define PI_CCxxx0_PACKET_COUNT_SIZE 255    // Packet bytes maximum count

// FSM states
typedef enum ccxxx0_state_e {
    CCxxx0_STATE_SLEEP = 0,
    CCxxx0_STATE_IDLE,
    CCxxx0_STATE_XOFF,
    CCxxx0_STATE_VCOON_MC,
    CCxxx0_STATE_REGON_MC,
    CCxxx0_STATE_MANCAL,
    CCxxx0_STATE_VCOON,
    CCxxx0_STATE_REGON,
    CCxxx0_STATE_STARTCAL,
    CCxxx0_STATE_BWBOOST,
    CCxxx0_STATE_FS_LOCK,
    CCxxx0_STATE_IFADCON,
    CCxxx0_STATE_ENDCAL,
    CCxxx0_STATE_RX,
    CCxxx0_STATE_RX_END,
    CCxxx0_STATE_RX_RST,
    CCxxx0_STATE_TXRX_SWITCH,
    CCxxx0_STATE_RXFIFO_OVERFLOW,
    CCxxx0_STATE_FSTXON,
    CCxxx0_STATE_TX,
    CCxxx0_STATE_TX_END,
    CCxxx0_STATE_RXTX_SWITCH,
    CCxxx0_STATE_TXFIFO_UNDERFLOW
} ccxxx0_state_t;

#endif