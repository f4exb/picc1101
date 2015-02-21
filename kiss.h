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

void pack_kiss(uint8_t *kiss_block, uint8_t *packed_block, size_t *size);
void unpack_kiss(uint8_t *kiss_block, uint8_t *packed_block, size_t *size);

#endif