/******************************************************************************/
/* PiCC1101  - Radio serial link using CC1101 module and Raspberry-Pi         */
/*                                                                            */
/* KISS AX.25 blocks handling                                                 */
/*                                                                            */
/*                      (c) Edouard Griffiths, F4EXB, 2015                    */
/*                                                                            */
/******************************************************************************/
#ifndef _KISS_H_
#define _KISS_H_

#include <stdint.h>
#include <stdlib.h>

#include "main.h"
#include "pi_cc_spi.h"
#include "serial.h"

#define KISS_FEND  0xC0
#define KISS_TFEND 0xDC
#define KISS_FESC  0xDB
#define KISS_TFESC 0xDD

void kiss_pack(uint8_t *kiss_block, uint8_t *packed_block, size_t *size);
void kiss_unpack(uint8_t *kiss_block, uint8_t *packed_block, size_t *size);
void kiss_run(serial_t *serial_parms, spi_parms_t *spi_parms, arguments_t *arguments);
void kiss_init(arguments_t *arguments);

#endif