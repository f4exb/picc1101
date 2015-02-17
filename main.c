/******************************************************************************/
/* PiCC1101  - Radio serial link using CC1101 module and Raspberry-Pi         */
/*                                                                            */
/*                                                                            */
/*                      (c) Edouard Griffiths, F4EXB, 2015                    */
/*                                                                            */
/*
/******************************************************************************/

#include <stdio.h>      // standard input / output functions
#include <stdlib.h>
#include <argp.h>
#include <string.h>
#include <signal.h>

#include "main.h"
#include "serial.h"
#include "pi_cc_spi.h"
#include "radio.h"

arguments_t   arguments;
serial_t      serial_parameters;
spi_parms_t   spi_parameters;
radio_parms_t radio_parameters;

char *test_mode_names[] = {
    "No test",
    "Simple Tx. Packet < 64 bytes",
    "Simple Tx with packet interrupt handling. Packet < 64 bytes",
    "Simple Rx. Packet < 64 bytes",
    "Simple Rx with packet interrupt handling. Packet < 64 bytes"
};

char *modulation_names[] = {
    "OOK",
    "2-FSK",
    "4-FSK",
    "MSK",
    "GFSK",
};

uint32_t rate_values[] = {
    50,
    110,
    300,
    600,
    1200,
    2400,
    4800,
    9600,
    14400,
    19200,
    28800,
    38400,
    57600,
    76800,
    115200,
    250000,
    500000
};

uint8_t nb_preamble_bytes[] = {
    2,
    3,
    4,
    6,
    8,
    12,
    16,
    24
};

/***** Argp configuration start *****/

const char *argp_program_version = "PiCC1101 0.1";
const char *argp_program_bug_address = "<f4exb06@gmail.com>";
static char doc[] = "PiRx -- Raspberry Pi serial radio link using CC1101 module.";
static char args_doc[] = "";

static struct argp_option options[] = {
    {"verbose",  'v', "VERBOSITY_LEVEL", 0, "Verbosiity level: 0 quiet else verbose level (default : quiet)"},
    {"long-help",  'H', 0, 0, "Print a long help and exit"},
    {"serial-device",  'D', "SERIAL_DEVICE", 0, "TNC Serial device, (default : /var/ax25/axp2)"},
    {"serial-speed",  'B', "SERIAL_SPEED", 0, "TNC Serial speed in Bauds (default : 9600)"},
    {"spi-device",  'd', "SPI_DEVICE", 0, "SPI device, (default : /dev/spidev0.0)"},
    {"modulation",  'M', "MODULATION_SCHEME", 0, "Radio modulation scheme, See long help (-H) option"},
    {"rate",  'R', "DATA_RATE_INDEX", 0, "Data rate index, See long help (-H) option"},
    {"modulation-index",  'm', "MODULATION_INDEX", 0, "Modulation index (default 0.5)"},
    {"fec",  'F', 0, 0, "Activate FEC (default off)"},
    {"whitening",  'W', 0, 0, "Activate whitening (default off)"},
    {"frequency",  'f', "FREQUENCY_HZ", 0, "Frequency in Hz (default: 433600000)"},
    {"packet-length",  'P', "PACKET_LENGTH", 0, "Packet length, 0 is variable (default: 250)"},
    {"test-mode",  't', "TEST_SCHEME", 0, "Test scheme, See long help (-H) option fpr details (default : 0 no test)"},
    {"test-phrase",  'y', "TEST_PHRASE", 0, "Set a test phrase to be used in test (default : \"Hello, World!\")"},
    {"repetition",  'n', "REPETITION", 0, "Repetiton factor wherever appropriate, see long Help (-H) option (default : 1 single)"},
    {"radio-status",  's', 0, 0, "Print radio status and exit"},
    {0}
};

void toto(void)
{}

static void delete_args(arguments_t *arguments);

// ------------------------------------------------------------------------------------------------
// Terminator
static void terminate(const int signal_) {
// ------------------------------------------------------------------------------------------------
    printf("PICC: Terminating with signal %d\n", signal_);
    delete_args(&arguments);
    exit(1);
}

// ------------------------------------------------------------------------------------------------
// Long help displays enumerated values
static void print_long_help()
{
    int i;

    fprintf(stderr, "Modulation scheme option -M values\n");
    fprintf(stderr, "Value:\tScheme:\n");

    for (i=0; i<NUM_MOD; i++)
    {
        fprintf(stderr, "%d\t%s\n", i, modulation_names[i]);
    }

    fprintf(stderr, "\nRate indexes option -R values\n");    
    fprintf(stderr, "Value:\tRate (Baud):\n");

    for (i=0; i<NUM_RATE; i++)
    {
        fprintf(stderr, "%2d\t%d\n", i, rate_values[i]);
    }

    fprintf(stderr, "\nRepetition factor option -n values\n");    
    fprintf(stderr, "- for test transmissions (-t option) this is the repetition of the same test packet\n");

}

// ------------------------------------------------------------------------------------------------
// Init arguments
static void init_args(arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    arguments->verbose_level = 0;
    arguments->print_long_help = 0;
    arguments->serial_device = 0;
    arguments->serial_speed = B38400;
    arguments->serial_speed_n = 38400;
    arguments->spi_device = 0;
    arguments->print_radio_status = 0;
    arguments->modulation = MOD_FSK2;
    arguments->rate = RATE_9600;
    arguments->modulation_index = 0.5;
    arguments->freq_hz = 433600000;
    arguments->packet_length = 250;
    arguments->test_mode = TEST_NONE;
    arguments->test_phrase = strdup("Hello, World!");
    arguments->repetition = 1;
    arguments->fec = 0;
    arguments->whitening = 0;
    arguments->preamble = PREAMBLE_4;
}

// ------------------------------------------------------------------------------------------------
// Delete arguments
void delete_args(arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    if (arguments->serial_device)
    {
        free(arguments->serial_device);
    }
    if (arguments->spi_device)
    {
        free(arguments->spi_device);
    }
    if (arguments->test_phrase)
    {
        free(arguments->test_phrase);
    }
}

// ------------------------------------------------------------------------------------------------
// Print MFSK data
static void print_args(arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    fprintf(stderr, "-- options --\n");
    fprintf(stderr, "Verbosity ...........: %d\n", arguments->verbose_level);
    fprintf(stderr, "--- radio ---\n");
    fprintf(stderr, "Modulation ..........: %s\n", modulation_names[arguments->modulation]);
    fprintf(stderr, "Rate ................: %d Baud\n", rate_values[arguments->rate]);
    fprintf(stderr, "Modulation index ....: %.2f\n", arguments->modulation_index);
    fprintf(stderr, "Frequency ...........: %d Hz\n", arguments->freq_hz);
    fprintf(stderr, "Packet length .......: %d bits\n", arguments->packet_length);
    fprintf(stderr, "Preamble size .......: %d bytes\n", nb_preamble_bytes[arguments->preamble]);
    fprintf(stderr, "FEC .................: %s\n", (arguments->fec ? "on" : "off"));
    fprintf(stderr, "Whitening ...........: %s\n", (arguments->whitening ? "on" : "off"));
    fprintf(stderr, "SPI device ..........: %s\n", arguments->spi_device);

    if (arguments->test_mode != TEST_NONE)
    {
        fprintf(stderr, "Test mode ...........: %s\n", test_mode_names[arguments->test_mode]);
        fprintf(stderr, "Test phrase .........: %s\n", arguments->test_phrase);
        fprintf(stderr, "Test repetition .....: %d times\n", arguments->repetition);
    }

    fprintf(stderr, "--- serial ---\n");
    fprintf(stderr, "TNC device ..........: %s\n", arguments->serial_device);
    fprintf(stderr, "TNC speed ...........: %d Baud\n", arguments->serial_speed_n);
}

// ------------------------------------------------------------------------------------------------
// Get test scheme from index
static test_mode_t get_test_scheme(uint8_t test_mode_index)
// ------------------------------------------------------------------------------------------------
{
    if (test_mode_index < NUM_TEST)
    {
        return (test_mode_t) test_mode_index;
    }
    else
    {
        return TEST_NONE;
    }
}

// ------------------------------------------------------------------------------------------------
// Get modulation scheme from index
static modulation_t get_modulation_scheme(uint8_t modulation_index)
// ------------------------------------------------------------------------------------------------
{
    if (modulation_index < NUM_MOD)
    {
        return (modulation_t) modulation_index;
    }
    else
    {
        return MOD_FSK2;
    }
}

// ------------------------------------------------------------------------------------------------
// Get rate from index     
static rate_t get_rate(uint8_t rate_index)
// ------------------------------------------------------------------------------------------------
{
    if (rate_index < NUM_RATE)
    {
        return (rate_t) rate_index;
    }
    else
    {
        return RATE_9600;
    }
}

// ------------------------------------------------------------------------------------------------
// Option parser 
static error_t parse_opt (int key, char *arg, struct argp_state *state)
// ------------------------------------------------------------------------------------------------
{
    arguments_t *arguments = state->input;
    char        *end;  // Used to indicate if ASCII to int was successful
    uint8_t     i8;
    uint32_t    i32;

    switch (key){
        // Verbosity 
        case 'v':
            arguments->verbose_level = strtol(arg, &end, 10);
            if (*end)
                argp_usage(state);
            break; 
        // Print long help and exit
        case 'H':
            arguments->print_long_help = 1;
            break;
        // Activate FEC
        case 'F':
            arguments->fec = 1;
            break;
        // Activate whitening
        case 'W':
            arguments->whitening = 1;
            break;
        // Reception test
        case 'r':
            arguments->test_rx = 1;
            break;
        // Modulation scheme 
        case 'M':
            i8 = strtol(arg, &end, 10); 
            if (*end)
                argp_usage(state);
            else
                arguments->modulation = get_modulation_scheme(i8);
            break;
        // Test scheme 
        case 't':
            i8 = strtol(arg, &end, 10); 
            if (*end)
                argp_usage(state);
            else
                arguments->test_mode = get_test_scheme(i8);
            break;
        // Radio data rate 
        case 'R':
            i8 = strtol(arg, &end, 10); 
            if (*end)
                argp_usage(state);
            else
                arguments->rate = get_rate(i8);
            break;
        // Radio link frequency
        case 'f':
            arguments->freq_hz = strtol(arg, &end, 10);
            if (*end)
                argp_usage(state);
            break; 
        // Packet length
        case 'P':
            arguments->packet_length = strtol(arg, &end, 10) % 256;
            if (*end)
                argp_usage(state);
            break; 
        // Repetition factor
        case 'n':
            arguments->repetition = strtol(arg, &end, 10);
            if (*end)
                argp_usage(state);
            break; 
        // Serial device
        case 'D':
            arguments->serial_device = strdup(arg);
            break;
        // Serial speed  
        case 'B':
            i32 = strtol(arg, &end, 10); 
            if (*end)
                argp_usage(state);
            else
                arguments->serial_speed = get_serial_speed(i32, &(arguments->serial_speed_n));
            break;
        // SPI device
        case 'd':
            arguments->spi_device = strdup(arg);
            break;
        // Transmission test phrase
        case 'y':
            arguments->test_phrase = strdup(arg);
            break;
        // Print radio status and exit
        case 's':
            arguments->print_radio_status = 1;
            break;
        // Modulation index
        case 'm':
            arguments->modulation_index = atof(arg);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

// ------------------------------------------------------------------------------------------------
static struct argp argp = {options, parse_opt, args_doc, doc};
// ------------------------------------------------------------------------------------------------

/***** ARGP configuration stop *****/

// ------------------------------------------------------------------------------------------------
int main (int argc, char **argv)
// ------------------------------------------------------------------------------------------------
{
    int i, ret, ser_read;

    // Whole response
    char response[1<<12];
    memset(response, '\0', sizeof(response));

    // unsolicited termination handling
    struct sigaction sa;
    // Catch all signals possible on process exit!
    for (i = 1; i < 64; i++) 
    {
        // skip SIGUSR2 for Wiring Pi
        if (i == 17)
            continue; 

        // These are uncatchable or harmless 
        if (i != SIGKILL
            && i != SIGSTOP
            && i != SIGVTALRM
            && i != SIGWINCH
            && i != SIGPROF) 
        {
            memset(&sa, 0, sizeof(sa));
            sa.sa_handler = terminate;
            sigaction(i, &sa, NULL);
        }
    }

    // Set argument defaults
    init_args(&arguments); 

    // Parse arguments 
    argp_parse (&argp, argc, argv, 0, 0, &arguments);

    if (arguments.print_long_help)
    {
        print_long_help();
        return 0;
    }
    
    if (!arguments.serial_device)
    {
        arguments.serial_device = strdup("/var/ax25/axp2");
    }
    if (!arguments.spi_device)
    {
        arguments.spi_device = strdup("/dev/spidev0.0");
    }

    print_args(&arguments);

    init_radio_parms(&radio_parameters);
    ret = init_radio(&radio_parameters, &spi_parameters, &arguments);

    if (ret != 0)
    {
        fprintf(stderr, "PICC: Cannot initialize radio link, RC=%d\n", ret);
        return ret;
    }

    if (arguments.print_radio_status)
    {
        fprintf(stderr, "\n--- Radio state ---\n");
        print_radio_status(&spi_parameters);
        return 0;
    }
    else if (arguments.test_mode == TEST_TX_SIMPLE)
    {
        radio_transmit_test(&spi_parameters, &arguments);
        return 9;
    }
    else if (arguments.test_mode == TEST_TX_INT_SIMPLE)
    {
        radio_transmit_test_int(&spi_parameters, &arguments);
        return 9;
    }
    else if (arguments.test_mode == TEST_RX_SIMPLE)
    {
        radio_receive_test(&spi_parameters, &arguments);
        return 0;
    }
    else if (arguments.test_mode == TEST_RX_INT_SIMPLE)
    {
        radio_receive_test_int(&spi_parameters, &arguments);
        return 0;
    }

    set_serial_parameters(&serial_parameters, &arguments);

    while (1)
    {
        ser_read = read_serial(&serial_parameters, response, sizeof(response));
        
        if (ser_read > 0)
        {
            response[ser_read] = '\0';
            printf("%s", response);
        }
        else
        {
            usleep(100000);
        }
    }

    delete_args(&arguments);
    return 0;
}



