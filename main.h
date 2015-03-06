#ifndef _MAIN_H_
#define _MAIN_H_

#include <inttypes.h>
#include <termios.h> 

#define ALLOW_VAR_BLOCKS 0
#define ALLOW_REAL_TIME  1

typedef enum test_mode_e {
    TEST_NONE = 0,
    TEST_TX_SIMPLE,
    TEST_TX_INTERRUPT,
    TEST_RX_SIMPLE,
    TEST_RX_INTERRUPT,
    TEST_TX_ECHO,
    TEST_RX_ECHO,
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
    RATE_50,
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

typedef enum preamble_e {
    PREAMBLE_2,
    PREAMBLE_3,
    PREAMBLE_4,
    PREAMBLE_6,
    PREAMBLE_8,
    PREAMBLE_12,
    PREAMBLE_16,
    PREAMBLE_24,
    NUM_PREAMBLE
} preamble_t;

extern uint8_t nb_preamble_bytes[];

typedef struct arguments_s {
    uint8_t      verbose_level;        // Verbose level
    uint8_t      print_long_help;      // Print a long help and exit
    // --- serial link virtual TNC --  -
    char         *serial_device;       // TNC serial device
    speed_t      serial_speed;         // TNC serial speed (Baud)
    uint32_t     serial_speed_n;       // TNC serial speed as a number
    // --- spi link radio ---
    char         *spi_device;          // CC1101 SPI device
    uint8_t      print_radio_status;   // Print radio status and exit
    modulation_t modulation;           // Radio modulation scheme
    rate_t       rate;                 // Data rate (Baud)
    float        rate_skew;            // Data rate skew multiplier from nominal
    float        modulation_index;     // Modulation index
    uint32_t     freq_hz;              // Frequency in Hz
    uint8_t      packet_length;        // Fixed packet length
    uint8_t      variable_length;      // Set variable length packet mode. Fixed packet argument becomes maximum packet size
    test_mode_t  test_mode;            // Enter testing mode with specified test scheme 
    char         *test_phrase;         // Test phrase to transmit
    uint8_t      test_rx;              // Reception test. Exits after receiving number of repetition packets
    uint8_t      repetition;           // Repetition factor
    uint8_t      fec;                  // Activate FEC
    uint8_t      whitening;            // Activate whitening
    preamble_t   preamble;             // Preamblescheme (number of preamble bytes)
    uint32_t     packet_delay;         // Delay before sending packet on serial or radio in 4 2-FSK symbols approximately
    uint32_t     tnc_serial_window;    // Time window in microseconds for concatenating serial frames (0: no concatenation)
    uint32_t     tnc_radio_window;     // Time window in microseconds for concatenating radio frames (0: no concatenation)
    uint32_t     tnc_keyup_delay;      // TNC keyup delay in microseconds
    uint32_t     tnc_keydown_delay;    // TNC keydown delay in microseconds
    uint32_t     tnc_switchover_delay; // TNC Rx/Tx switchover delay in microseconds
    uint8_t      real_time;            // Engage so called "real time" scheduling
} arguments_t;

#endif
