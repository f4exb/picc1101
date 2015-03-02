/******************************************************************************/
/* PiCC1101  - Radio serial link using CC1101 module and Raspberry-Pi         */
/*                                                                            */
/* KISS AX.25 blocks handling                                                 */
/*                                                                            */
/*                      (c) Edouard Griffiths, F4EXB, 2015                    */
/*                                                                            */
/******************************************************************************/

#include <string.h>

#include "kiss.h"
#include "radio.h"
#include "util.h"

static uint32_t kiss_tx_keyup_delay; // Tx keyup delay in microseconds
static float    kiss_persistence;    // Persostence parameter
static uint32_t kiss_slot_time;      // Slot time in microseconds
static uint32_t kiss_tx_tail;        // Tx tail in microseconds (obsolete)

// === Static functions declarations ==============================================================

static uint8_t *kiss_tok(uint8_t *block, uint8_t *end);
static uint8_t kiss_command(uint8_t *block);

// === Static functions ===========================================================================

// ------------------------------------------------------------------------------------------------
// Utility to unconcatenate KISS blocks. Returns pointer on next KISS delimiter past first byte (KISS_FEND = 0xC0)
// Assumes the pointer is currently on the opening KISS_FEND. Give pointer to first byte of block and past end pointer
uint8_t *kiss_tok(uint8_t *block, uint8_t *end) 
// ------------------------------------------------------------------------------------------------
{
    uint8_t *p_cur, *p_ret = NULL;


    for (p_cur = block; p_cur < end; p_cur++)
    {
        if (p_cur == block)
        {
            if (*p_cur == KISS_FEND)
            {
                continue;
            }
            else
            {
                break; // will return NULL
            }
        }

        if (*p_cur == KISS_FEND)
        {
            p_ret = p_cur;
            break;
        }
    }

    return p_ret;
}

// === Public functions ===========================================================================

// ------------------------------------------------------------------------------------------------
// Initialize the common parameters to defaults
void kiss_init(arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    kiss_tx_keyup_delay = 50000; // 50ms Tx keyup delay
    kiss_persistence = 0.25;     // 0.25 persistence parameter
    kiss_slot_time = 100000;     // 100ms slot time
    kiss_tx_tail = 0;            // obsolete
}

// ------------------------------------------------------------------------------------------------
// Remove KISS signalling
void kiss_pack(uint8_t *kiss_block, uint8_t *packed_block, size_t *size)
// ------------------------------------------------------------------------------------------------
{
    size_t  new_size = 0, i;
    uint8_t fesc = 0;

    for (i=1; i<*size-1; i++)
    {
        if (kiss_block[i] == KISS_FESC) // FESC
        {
            fesc = 1;
            continue;
        }
        if (fesc)
        {
            if (kiss_block[i] == KISS_TFEND) // TFEND
            {
                packed_block[new_size++] = KISS_FEND; // FEND
            }
            else if (kiss_block[i] == KISS_TFESC) // TFESC
            {
                packed_block[new_size++] = KISS_FESC; // FESC    
            }

             fesc = 0;
             continue;
        }

        packed_block[new_size++] = kiss_block[i];
    }

    *size = new_size;
}

// ------------------------------------------------------------------------------------------------
// Restore KISS signalling
void kiss_unpack(uint8_t *kiss_block, uint8_t *packed_block, size_t *size)
// ------------------------------------------------------------------------------------------------
{
    size_t  new_size = 0, i;

    kiss_block[0] = KISS_FEND; // FEND

    for (i=0; i<*size; i++)
    {
        if (packed_block[i] == KISS_FEND) // FEND
        {
            kiss_block[new_size++] = KISS_FESC; // FESC
            kiss_block[new_size++] = KISS_TFEND; // TFEND
        }
        else if (packed_block[i] == KISS_FESC) // FESC
        {
            kiss_block[new_size++] = KISS_FESC; // FESC
            kiss_block[new_size++] = KISS_TFESC; // TFESC
        }
        else
        {
            kiss_block[new_size++] = packed_block[i];
        }
    }

    kiss_block[new_size++] = KISS_FEND; // FEND
    *size = new_size;
}

// ------------------------------------------------------------------------------------------------
// Check if the KISS block is a command block and interpret the command 
// Returns 1 if this is a command block
// Returns 0 it this is a data block
uint8_t kiss_command(uint8_t *block)
// ------------------------------------------------------------------------------------------------
{
    uint8_t command_code = block[1] & 0x0F;
    uint8_t kiss_port = (block[1] & 0xF0)>>4;
    uint8_t command_arg = block[2];

    verbprintf(4, "KISS: command %02X %02X\n", block[1], block[2]);

    switch (command_code)
    {
        case 0: // data block
            return 0;
        case 1: // TXDELAY
            kiss_tx_keyup_delay = command_arg * 10000; // these are tenths of ms
            break;
        case 2: // Persistence parameter
            kiss_persistence = (command_arg + 1) / 256.0;
            break;
        case 3: // Slot time
            kiss_slot_time = command_arg * 10000; // these are tenths of ms
            break;
        case 4: // Tx tail
            kiss_tx_tail = command_arg * 10000; // these are tenths of ms
            break;
        case 15:
            verbprintf(1, "KISS: received aborting command\n");
            abort();
            break;
        default:
            break;
    }

    verbprintf(1, "KISS: command received for port %d: (%d,%d)\n", kiss_port, command_code, command_arg);
    return 1;
}

// ------------------------------------------------------------------------------------------------
// Run the KISS virtual TNC
void kiss_run(serial_t *serial_parms, spi_parms_t *spi_parms, arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    static const size_t bufsize = RADIO_BUFSIZE;
    uint8_t rx_buffer[bufsize], tx_buffer[bufsize];
    uint8_t rtx_toggle; // 1:Tx, 0:Rx
    int rx_count, tx_count, byte_count, ret;
    uint32_t timeout;

    set_serial_parameters(serial_parms, arguments);
    init_radio_int(spi_parms, arguments);
    memset(rx_buffer, 0, bufsize);
    memset(tx_buffer, 0, bufsize);
    radio_flush_fifos(spi_parms);
    
    verbprintf(1, "Starting...\n");

    timeout = 0;
    rtx_toggle = 0;
    rx_count = 0;
    tx_count = 0;
    radio_init_rx(spi_parms, arguments); // init for new packet to receive Rx
    radio_turn_rx(spi_parms);            // Turn Rx on

    while(1)
    {    
        byte_count = radio_receive_packet(spi_parms, arguments, &rx_buffer[rx_count]); // check if anything was received on radio link

        if (byte_count > 0)
        {
            if ((rtx_toggle) && (tx_count)) // First Rx after Tx: flush Tx buffer to the air 
            {
                if (!kiss_command(tx_buffer))
                {
                    radio_wait_free();            // Make sure no radio operation is in progress
                    radio_turn_idle(spi_parms);   // Inhibit radio operations (should be superfluous since both Tx and Rx turn to IDLE after a packet has been processed)
                    radio_flush_fifos(spi_parms); // Flush result of any Rx activity

                    verbprintf(2, "%d bytes to send\n", tx_count);

                    usleep(kiss_tx_keyup_delay);
                    radio_send_packet(spi_parms, arguments, tx_buffer, tx_count);

                    radio_init_rx(spi_parms, arguments); // init for new packet to receive Rx
                    radio_turn_rx(spi_parms);            // put back into Rx
                }

                tx_count = 0;
            }

            rtx_toggle = 1;
            rx_count += byte_count; // Accumulate Rx
            timeout = arguments->packet_length;
        }

        byte_count = read_serial(serial_parms, &tx_buffer[tx_count], bufsize - tx_count);

        if (byte_count > 0)
        {
            if ((!rtx_toggle) && (rx_count)) // First Tx after Rx: flush Rx buffer to serial 
            {
                radio_wait_free();            // Make sure no radio operation is in progress
                radio_turn_idle(spi_parms);   // Inhibit radio operations
                verbprintf(2, "Received %d bytes\n", rx_count);
                ret = write_serial(serial_parms, rx_buffer, rx_count);
                verbprintf(2, "Sent %d bytes on serial\n", ret);
                radio_init_rx(spi_parms, arguments); // Init for new packet to receive Rx
                radio_turn_rx(spi_parms);            // Put back into Rx
                rx_count = 0;
            }

            rtx_toggle = 1;
            tx_count += byte_count; // Accumulate Tx
            timeout = arguments->packet_length;
        }

        radio_wait_a_bit(1); // approx. one byte long

        if (timeout == 0)
        {
            rtx_toggle = !rtx_toggle;
        }
        else
        {
            timeout--;
        }
    }
}