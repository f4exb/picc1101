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
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions

#include "main.h"

int verbose_level;
int SERIAL_TNC;
struct termios tty;
struct termios tty_old;

/***** Argp configuration start *****/

const char *argp_program_version = "PiCC1101 0.1";
const char *argp_program_bug_address = "<f4exb06@gmail.com>";
static char doc[] = "PiRx -- Raspberry Pi serial radio link using CC1101 module.";
static char args_doc[] = "";

static struct argp_option options[] = {
    {"verbose",  'v', "VERBOSITY_LEVEL", 0, "Verbosiity level: 0 quiet else verbose level (default : quiet)"},
    {"serial-device",  'D', "SERIAL_DEVICE", 0, "TNC Serial device, (default : /var/ax25/axp2)"},
    {"serial-speed",  'B', "SERIAL_SPEED", 0, "TNC Serial speed in Bauds (default : 9600)"},
    {"modulation",  'M', "MODULATION_SCHEME", 0, "Radio modulation scheme, 0: OOK, 1: FSK-2, 2: FSK-4, 3: MSK, 4: GMSK (default: 1)"},
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
    arguments->verbose = 0;
    arguments->serial_device = 0;
    arguments->modulation = MOD_FSK2;
    arguments->serial_speed = B38400;
    arguments->serial_speed_n = 38400;
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
}

// ------------------------------------------------------------------------------------------------
// Print MFSK data
static void print_args(arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    fprintf(stderr, "-- options --\n");
    fprintf(stderr, "Verbosiity ..........: %d\n", verbose_level);
    fprintf(stderr, "Modulation # ........: %d\n", (int) arguments->modulation);
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
// Get serial speed
static speed_t get_serial_speed(uint32_t speed, uint32_t *speed_n)
{
    if (speed >= 230400)
    {
        *speed_n = 230400;
        return B230400;
    }
    else if (speed >= 115200)
    {
        *speed_n = 115200;
        return B115200;
    }
    else if (speed >= 57600)
    {
        *speed_n = 57600;
        return B57600;
    }
    else if (speed >= 38400)
    {
        *speed_n = 38400;
        return B38400;
    }
    else if (speed >= 19200)
    {
        *speed_n = 19200;
        return B19200;
    }
    else if (speed >= 9600)
    {
        *speed_n = 9600;
        return B9600;
    }
    else if (speed >= 4800)
    {
        *speed_n = 4800;
        return B4800;
    }
    else if (speed >= 2400)
    {
        *speed_n = 2400;
        return B2400;
    }
    else if (speed >= 1200)
    {
        *speed_n = 1200;
        return B1200;
    }
    else if (speed >= 600)
    {
        *speed_n = 600;
        return B600;
    }
    else if (speed >= 300)
    {
        *speed_n = 300;
        return B300;
    }
    else if (speed >= 134)
    {
        *speed_n = 134;
        return B134;
    }
    else if (speed >= 110)
    {
        *speed_n = 110;
        return B110;
    }
    else if (speed >= 75)
    {
        *speed_n = 75;
        return B75;
    }
    else if (speed >= 50)
    {
        *speed_n = 50;
        return B50;
    }
    else
    {
        *speed_n = 0;
        return B0;
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
            arguments->verbose = strtol(arg, &end, 10);
            if (*end)
                argp_usage(state);
            break; 
        // Modulation scheme 
        case 'M':
            i8 = strtol(arg, &end, 10); 
            if (*end)
                argp_usage(state);
            else
                arguments->modulation = get_modulation_scheme(i8);
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
// Init serial interface (TNC)
static void set_serial_parameters(arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    SERIAL_TNC = open(arguments->serial_device, O_RDWR | O_NOCTTY);

    memset (&tty, 0, sizeof tty);

    // Error Handling 
    if ( tcgetattr ( SERIAL_TNC, &tty ) != 0 ) 
    {
        printf("Error %d from tcgetattr: %s\n", errno, strerror(errno));
    }

    // Save old tty parameters 
    tty_old = tty;

    // Set Baud Rate 
    cfsetospeed (&tty, arguments->serial_speed);
    cfsetispeed (&tty, arguments->serial_speed);

    // Setting other Port Stuff 
    tty.c_cflag     &=  ~PARENB;            // Make 8n1
    tty.c_cflag     &=  ~CSTOPB;
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS8;

    tty.c_cflag     &=  ~CRTSCTS;           // no flow control
    tty.c_cc[VMIN]   =  1;                  // read doesn't block
    tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
    tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

    // Make raw 
    cfmakeraw(&tty);

    // Flush Port, then applies attributes 
    tcflush( SERIAL_TNC, TCIFLUSH );

    if ( tcsetattr ( SERIAL_TNC, TCSANOW, &tty ) != 0) 
    {
        printf("Error %d from tcsetattr: %s\n", errno, strerror(errno));
    }    
}

// ------------------------------------------------------------------------------------------------
// Write to serial interface
static int write_serial(char *msg, int msglen)
// ------------------------------------------------------------------------------------------------
{
    int n_written = 0, spot = 0;
    n_written = write(SERIAL_TNC, msg, msglen);
    return msglen - n_written;
}

// ------------------------------------------------------------------------------------------------
// Read from serial interface
static int read_serial(char *buf, int buflen)
// ------------------------------------------------------------------------------------------------
{
    int bytes_read = 0;
    bytes_read = read(SERIAL_TNC, buf, buflen);
    return bytes_read;
} 

// ------------------------------------------------------------------------------------------------
int main (int argc, char **argv)
// ------------------------------------------------------------------------------------------------
{
    arguments_t arguments;
    int i, ser_read;
    
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
    
    verbose_level = arguments.verbose;

    if (!arguments.serial_device)
    {
        arguments.serial_device = strdup("/var/ax25/axp2");
    }

    print_args(&arguments);
    set_serial_parameters(&arguments);

    while (1)
    {
        ser_read = read_serial(response, sizeof(response));
        
        if (ser_read > 0)
        {
            response[ser_read] = '\0';
            printf("%s\n", response);
        }
        else
        {
            usleep(100000);
        }
    }

    return 0;
}



