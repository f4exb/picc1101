/******************************************************************************/
/* PiCC1101  - Radio serial link using CC1101 module and Raspberry-Pi         */
/*                                                                            */
/* Test routines                                                              */
/*                                                                            */
/*                      (c) Edouard Griffiths, F4EXB, 2015                    */
/*                                                                            */
/******************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test.h"
#include "radio.h"
#include "util.h"

// === Public functions ===========================================================================

// ------------------------------------------------------------------------------------------------
// Transmission test with interrupt handling
int radio_transmit_test_int(spi_parms_t *spi_parms, arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    init_radio_int(spi_parms, arguments);
    PI_CC_SPIStrobe(spi_parms, PI_CCxxx0_SFTX); // Flush Tx FIFO
    
    verbprintf(0, "Sending %d test packets of size %d\n", arguments->repetition, arguments->packet_length);

    while(packets_sent < arguments->repetition)
    {
        radio_wait_free(); // make sure no radio operation is in progress
        radio_send_packet(spi_parms, arguments, arguments->test_phrase, strlen(arguments->test_phrase));
        radio_wait_a_bit(arguments->packet_length / 4);
    } 
}

// ------------------------------------------------------------------------------------------------
// Reception test with interrupt handling
int radio_receive_test_int(spi_parms_t *spi_parms, arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    uint8_t nb_rx, rx_bytes[RADIO_BUFSIZE];

    init_radio_int(spi_parms, arguments);
    PI_CC_SPIStrobe(spi_parms, PI_CCxxx0_SFRX); // Flush Rx FIFO

    verbprintf(0, "Starting...\n");

    while((arguments->repetition == 0) || (packets_received < arguments->repetition))
    {
        radio_init_rx(spi_parms, arguments); // Init for new packet to receive
        radio_turn_rx(spi_parms);            // Put back into Rx

        do
        {
            radio_wait_free(); // make sure no radio operation is in progress
            nb_rx = radio_receive_packet(spi_parms, arguments, rx_bytes);
        } while(nb_rx == 0);

        rx_bytes[nb_rx] = '\0';
        verbprintf(0,"\"%s\"\n", rx_bytes);
    }
}


// ------------------------------------------------------------------------------------------------
// Simple echo test
void radio_test_echo(spi_parms_t *spi_parms, radio_parms_t *radio_parms, arguments_t *arguments, uint8_t active)
// ------------------------------------------------------------------------------------------------
{
    uint8_t  nb_bytes, rtx_bytes[RADIO_BUFSIZE];
    uint8_t  rtx_toggle, rtx_count;
    uint32_t timeout_value, timeout;
    struct timeval tdelay, tstart, tstop;

    init_radio_int(spi_parms, arguments);
    radio_flush_fifos(spi_parms);

    timeout_value = (uint32_t) (arguments->packet_length * 10 * radio_get_byte_time(radio_parms));
    timeout = 0;

    if (active)
    {
        nb_bytes = strlen(arguments->test_phrase);
        strcpy(rtx_bytes, arguments->test_phrase);
        rtx_toggle = 1;
    }
    else
    {
        rtx_toggle = 0;
    }

    while (packets_sent < arguments->repetition)
    {
        rtx_count = 0;

        do // Rx-Tx transaction in whichever order
        {
            if (arguments->tnc_keyup_delay)
            {
                usleep(arguments->tnc_keyup_delay);
            }

            if (rtx_toggle) // Tx
            {
                verbprintf(0, "Sending #%d\n", packets_sent);

                radio_wait_free(); // make sure no radio operation is in progress
                radio_send_packet(spi_parms, arguments, rtx_bytes, nb_bytes);
                radio_wait_a_bit(4);
                timeout = timeout_value; // arm Rx timeout
                rtx_count++;
                rtx_toggle = 0; // next is Rx
            }

            if (rtx_count >= 2)
            {
                break;
            }

            if (arguments->tnc_keydown_delay)
            {
                usleep(arguments->tnc_keydown_delay);
            }

            if (!rtx_toggle) // Rx
            {
                verbprintf(0, "Receiving #%d\n", packets_received);

                radio_init_rx(spi_parms, arguments); // Init for new packet to receive
                radio_turn_rx(spi_parms);            // Put back into Rx

                if (timeout > 0)
                {
                    gettimeofday(&tstart, NULL);
                }

                do
                {
                    radio_wait_free(); // make sure no radio operation is in progress
                    nb_bytes = radio_receive_packet(spi_parms, arguments, rtx_bytes);
                    radio_wait_a_bit(4);

                    if (timeout > 0)
                    {
                        gettimeofday(&tstop, NULL);
                        timeval_subtract(&tdelay, &tstop, &tstart);

                        if (ts_us(&tdelay) > timeout)
                        {
                            verbprintf(0, "Time out reached. Faking receiving data\n");
                            nb_bytes = strlen(arguments->test_phrase);
                            strcpy(rtx_bytes, arguments->test_phrase);                            
                            break;
                        }
                    }

                } while (nb_bytes == 0);

                rtx_count++;
                rtx_toggle = 1; // next is Tx                
            }

        } while(rtx_count < 2); 
    }    
}

// ------------------------------------------------------------------------------------------------
// Transmission test with polling of registers
int radio_transmit_test(spi_parms_t *spi_parms, arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    uint8_t  test_length, tx_length, byte;
    uint8_t  tx_buf[PI_CCxxx0_FIFO_SIZE];
    int      i, j, ret;
    uint32_t payload_fec = 4 + arguments->packet_length; // Number of bytes that can be protected by FEC
    uint64_t tx_delay; // Delay in microseconds for message transmission. Take 8 bytes guard interval. 

    if (arguments->modulation == MOD_FSK4)
    {
        payload_fec /= 2;
    }

    if (arguments->fec) // twice the payload delay if FEC is engaged
    {
        tx_delay = (8000000ULL * (nb_preamble_bytes[arguments->preamble] + 4 + 2*payload_fec + 8)) / rate_values[arguments->rate];
    }
    else
    {
        tx_delay = (8000000ULL * (nb_preamble_bytes[arguments->preamble] + 4 + payload_fec + 8)) / rate_values[arguments->rate];   
    }

    if (tx_delay < 100000ULL) // set a minimum wait time of 100ms
    {
        tx_delay = 100000ULL;
    }

    verbprintf(1, "Estimated Tx delay is %lld us\n", tx_delay);

    if (strlen(arguments->test_phrase) < PI_CCxxx0_FIFO_SIZE)
    {
        test_length = strlen(arguments->test_phrase);
    }
    else
    {
        verbprintf(0, "Test phrase too long. Truncated to CC1101 FIFO size\n");
        test_length = PI_CCxxx0_FIFO_SIZE;
    }

    memset(tx_buf, ' ', PI_CCxxx0_FIFO_SIZE);
    memcpy(tx_buf, arguments->test_phrase, test_length);

    if (arguments->packet_length == 0)
    {
        tx_length = test_length;
    }
    else if (arguments->packet_length < PI_CCxxx0_FIFO_SIZE)
    {
        tx_length = arguments->packet_length;
    }
    else
    {
        tx_length = PI_CCxxx0_FIFO_SIZE;
    }

    radio_set_packet_length(spi_parms, tx_length);
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_IOCFG2,   0x02); // GDO2 output pin config TX mode
    PI_CC_SPIStrobe(spi_parms, PI_CCxxx0_SFTX);

    verbprintf(0, "Sending test packet of size %d %d times\n", tx_length, arguments->repetition);

    for (i=0; i<arguments->repetition; i++)
    {
        verbprintf(0, "Packet #%d\n", i);

        for (j=0; j<tx_length; j++)
        {
            PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_TXFIFO, tx_buf[j]);
            verbprintf(2, "%02X ", spi_parms->rx[0]);
        }

        verbprintf(2, "\n");
        ret = PI_CC_SPIStrobe(spi_parms, PI_CCxxx0_STX);
        
        usleep(tx_delay);

        print_radio_status(spi_parms);
    }
}

// ------------------------------------------------------------------------------------------------
// Reception test with polling of registers
int radio_receive_test(spi_parms_t *spi_parms, arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    uint8_t iterations, rx_bytes, fsm_state, rssi_dec, crc_lqi, x_byte, pkt_on;
    uint8_t rx_buf[PI_CCxxx0_FIFO_SIZE+1];
    uint8_t rx_count;
    int i;
    uint32_t poll_us = 4*8000000 / rate_values[arguments->rate]; // 4 2-FSK symbols delay

    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_IOCFG2,   0x00); // GDO2 output pin config RX mode
    PI_CC_SPIStrobe(spi_parms, PI_CCxxx0_SFRX);
    PI_CC_SPIStrobe(spi_parms, PI_CCxxx0_SRX);

    while(1)
    {
        PI_CC_SPIReadStatus(spi_parms, PI_CCxxx0_MARCSTATE, &fsm_state);
        fsm_state &= 0x1F;

        if (fsm_state == CCxxx0_STATE_RX)
        {
            break;
        }

        usleep(1000);
    }

    print_radio_status(spi_parms);

    for (iterations=0; iterations<arguments->repetition; iterations++)
    {
        verbprintf(0, "Packet #%d\n", iterations+1);
        pkt_on = 0; // wait for packet start
        memset(rx_buf, '\0', PI_CCxxx0_FIFO_SIZE+1);

        while(1)
        {
            PI_CC_SPIReadStatus(spi_parms, PI_CCxxx0_PKTSTATUS, &x_byte); // sense GDO0 (& 0x01)

            if (x_byte & 0x01)
            {
                pkt_on = 1; // started receiving a packet
            }

            if (!(x_byte & 0x01) && pkt_on) // packet received
            {
                PI_CC_SPIReadStatus(spi_parms, PI_CCxxx0_RXBYTES, &rx_count);
                rx_count &= PI_CCxxx0_NUM_RXBYTES;
                verbprintf(1, "Received %d bytes\n", rx_count);

                for (i=0; i<rx_count; i++)
                {
                    PI_CC_SPIReadReg(spi_parms, PI_CCxxx0_RXFIFO, &(rx_buf[i]));
                }

                print_block(0, rx_buf, rx_count);

                break;
            }

            usleep(poll_us);
        }
    }

    verbprintf(0, "Done\n");
}
