#ifndef _MAIN_H_
#define _MAIN_H_

#include <inttypes.h>
#include <termios.h> 

typedef enum modulation_e {
    MOD_OOK,
    MOD_FSK2,
    MOD_FSK4,
    MOD_MSK,
    MOD_GMSK
} modulation_t;

typedef struct arguments_s {
    uint8_t      verbose;         // Verbose level
    char         *serial_device;  // TNC serial device
    char         *spi_device;     // CC1101 SPI device
    modulation_t modulation;      // Radio modulation scheme
    speed_t      serial_speed;    // TNC serial speed (Baud)
    uint32_t     serial_speed_n;  // TNC serial speed as a number
} arguments_t;

#endif
