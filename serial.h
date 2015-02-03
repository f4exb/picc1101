/******************************************************************************/
/* PiCC1101  - Radio serial link using CC1101 module and Raspberry-Pi         */
/*                                                                            */
/* Serial definitions                                                         */
/*                                                                            */
/*                      (c) Edouard Griffiths, F4EXB, 2015                    */
/*                                                                            */
/******************************************************************************/

#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <inttypes.h>
#include <termios.h> 

typedef struct serial_s
{
	int SERIAL_TNC;
	struct termios tty;
	struct termios tty_old;
} serial_t;

speed_t get_serial_speed(uint32_t speed, uint32_t *speed_n);
void set_serial_parameters(arguments_t *arguments, serial_t *serial_parameters);
int write_serial(char *msg, int msglen, serial_t *serial_parameters);
int read_serial(char *buf, int buflen, serial_t *serial_parameters);

#endif