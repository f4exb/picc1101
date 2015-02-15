#ifndef _MAIN_H_
#define _MAIN_H_

#include <inttypes.h>
#include <termios.h> 

typedef enum test_mode_e {
    TEST_NONE = 0,
    TEST_TX_SIMPLE,
    TEST_RX_SIMPLE,
    NUM_TEST
} test_mode_t;

extern char *test_mode_names[];

typedef enum modulation_e {
    MOD_OOK,
    MOD_FSK2,
    MOD_FSK4,
    MOD_MSK,
    MOD_GFSK,
    NUM_MOD
} modulation_t;

extern char *modulation_names[];

typedef enum rate_e {
    RATE_110,
    RATE_300,
    RATE_600,
    RATE_1200,
    RATE_2400,
    RATE_4800,
    RATE_9600,
    RATE_14400,
    RATE_19200,
    RATE_28800,
    RATE_38400,
    RATE_57600,
    RATE_76800,
    RATE_115200,
    RATE_250K,
    RATE_500K,
    NUM_RATE
} rate_t;

extern uint32_t rate_values[];

typedef struct arguments_s {
    uint8_t      verbose_level;      // Verbose level
    uint8_t      print_long_help;    // Print a long help and exit
    // --- serial link virtual TNC ---
    char         *serial_device;     // TNC serial device
    speed_t      serial_speed;       // TNC serial speed (Baud)
    uint32_t     serial_speed_n;     // TNC serial speed as a number
    // --- spi link radio ---
    char         *spi_device;        // CC1101 SPI device
    uint8_t      print_radio_status; // Print radio status and exit
    modulation_t modulation;         // Radio modulation scheme
    rate_t       rate;               // Data rate (Baud)
    float        modulation_index;   // Modulation index
    uint32_t     freq_hz;            // Frequency in Hz
    uint8_t      packet_length;      // Fixed packet length
    test_mode_t  test_mode;          // Enter testing mode with specified test scheme 
    char         *test_phrase;       // Test phrase to transmit
    uint8_t      test_rx;            // Reception test. Exits after receiving number of repetition packets
    uint8_t      repetition;         // Repetition factor
    uint8_t      fec;                // Activate FEC
    uint8_t      whitening;          // Activate whitening
} arguments_t;

#endif
