/******************************************************************************/
/* PiCC1101  - Radio serial link using CC1101 module and Raspberry-Pi         */
/*                                                                            */
/* Test routines                                                              */
/*                                                                            */
/*                      (c) Edouard Griffiths, F4EXB, 2015                    */
/*                                                                            */
/******************************************************************************/

#ifndef _TEST_H_
#define _TEST_H_

#include "main.h"
#include "pi_cc_spi.h"

int radio_transmit_test_int(spi_parms_t *spi_parms, arguments_t *arguments);
int radio_receive_test_int(spi_parms_t *spi_parms, arguments_t *arguments);

int radio_transmit_test(spi_parms_t *spi_parms, arguments_t *arguments);
int radio_receive_test(spi_parms_t *spi_parms, arguments_t *arguments);

#endif