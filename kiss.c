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

// === Static functions declarations ==============================================================

static uint8_t *kiss_tok(uint8_t *block, uint8_t *end);

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
// Run the virtual KISS TNC in a loop
void kiss_run(serial_t *serial_parms, spi_parms_t *spi_parms, arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    int i, ret, read_bytes;

    // Whole read buffer
    char read_buffer[1<<11];
    memset(read_buffer, '\0', sizeof(read_buffer));

    set_serial_parameters(serial_parms, arguments);
    radio_flush_fifos(spi_parms);
    init_radio_int(spi_parms, arguments);
    radio_receive_listen(spi_parms, arguments); // put radio in Rx mode

    while (1)
    {
    	// Process Rx block if any

        read_bytes = radio_receive_packet(spi_parms, arguments, read_buffer); 

        if (read_bytes > 0)
        {
            verbprintf(2, "Radio received %d bytes\n", read_bytes);
            radio_wait_a_bit(arguments->packet_delay); // ~ x4 2-FSK symbols
            write_serial(serial_parms, read_buffer, read_bytes);
            radio_receive_listen(spi_parms, arguments); // Do Rx again
            continue; // back to the loop, This will process a received packet if any
        }        

        // Process incoming block to transmit if any

        read_bytes = read_serial(serial_parms, read_buffer, sizeof(read_buffer));
        
        if (read_bytes > 0)
        {
            verbprintf(2, "Serial received %d bytes\n", read_bytes);
          	radio_wait_a_bit(arguments->packet_delay); // ~ x4 2-FSK symbols

            if (read_bytes > arguments->packet_length) // concatenated KISS frames
            {
            	uint8_t *kiss_fend, *kiss_frame = read_buffer;

                verbprintf(2, "Concatenated KISS block encountered\n");
                print_block(4, read_buffer, read_bytes);
                verbprintf(4, "...\n");

                while ((kiss_fend = kiss_tok(kiss_frame, (uint8_t *) read_buffer + read_bytes)))
                {
                	verbprintf(2, "Processing KISS block of %d bytes\n", kiss_fend - kiss_frame + 1);
	                print_block(4, kiss_frame, kiss_fend - kiss_frame + 1);
                	radio_send_packet(spi_parms, arguments, kiss_frame, kiss_fend - kiss_frame + 1);
                	kiss_frame = kiss_fend + 1;
		          	radio_wait_a_bit(arguments->packet_delay); // ~ x4 2-FSK symbols
                }
            }
            else // single KISS frame
            {
	            radio_send_packet(spi_parms, arguments, read_buffer, read_bytes);
	        }

            radio_receive_listen(spi_parms, arguments); // back to Rx
        }

        radio_wait_a_bit(1);
    }
}