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
    {"frequency",  'f', "FREQUENCY_HZ", 0, "Frequency in Hz (default: 433600000)"},
    {"radio-status",  's', 0, 0, "Print radio status and exit"},
    {0}
};

// ------------------------------------------------------------------------------------------------
// Terminator
static void terminate(const int signal_) {
// ------------------------------------------------------------------------------------------------
    printf("Terminating with signal %d\n", signal_);
    exit(1);
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
    arguments->freq_hz = 433600000;
}

// ------------------------------------------------------------------------------------------------
// Delete arguments
static void delete_args(arguments_t *arguments)
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
}

// ------------------------------------------------------------------------------------------------
// Print MFSK data
static void print_args(arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    fprintf(stderr, "-- options --\n");
    fprintf(stderr, "Verbosity ...........: %d\n", arguments->verbose_level);
    fprintf(stderr, "--- radio ---\n");
    fprintf(stderr, "Modulation # ........: %d\n", (int) arguments->modulation);
    fprintf(stderr, "Rate # ..............: %d\n", (int) arguments->rate);
    fprintf(stderr, "Frequency ...........: %d Hz\n", arguments->freq_hz);
    fprintf(stderr, "SPI device ..........: %s\n", arguments->spi_device);
    fprintf(stderr, "--- serial ---\n");
    fprintf(stderr, "TNC device ..........: %s\n", arguments->serial_device);
    fprintf(stderr, "TNC speed ...........: %d Baud\n", arguments->serial_speed_n);
}

// ------------------------------------------------------------------------------------------------
// Get modulation scheme from index
static modulation_t get_modulation_scheme(uint8_t modulation_index)
// ------------------------------------------------------------------------------------------------
{
    if (modulation_index < sizeof(modulation_t))
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
    if (rate_index < sizeof(rate_t))
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
        // Modulation scheme 
        case 'M':
            i8 = strtol(arg, &end, 10); 
            if (*end)
                argp_usage(state);
            else
                arguments->modulation = get_modulation_scheme(i8);
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
        // Print radio status and exit
        case 's':
            arguments->print_radio_status = 1;
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

    if (!ret)
    {
        fprintf(stderr, "Cannot initialize radio link\n");
        return ret;
    }

    if (arguments.print_radio_status)
    {
        print_radio_status(&spi_parameters);
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

    return 0;
}



