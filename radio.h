/******************************************************************************/
/* PiCC1101  - Radio serial link using CC1101 module and Raspberry-Pi         */
/*                                                                            */
/* Radio link definitions                                                     */
/*                                                                            */
/*                      (c) Edouard Griffiths, F4EXB, 2015                    */
/*                                                                            */
/******************************************************************************/

#ifndef _RADIO_H_
#define _RADIO_H_

#include "pi_cc_spi.h"

typedef enum sync_word_e
{
	NO_SYNC = 0,              // No preamble/sync
	SYNC_15_OVER_16,          // 15/16 sync word bits detected
	SYNC_16_OVER_16,          // 16/16 sync word bits detected
	SYNC_30_over_32,          // 30/32 sync word bits detected
	SYNC_CARRIER,             // No preamble/sync, carrier-sense above threshold
	SYNC_15_OVER_16_CARRIER,  // 15/16 + carrier-sense above threshold
	SYNC_16_OVER_16_CARRIER,  // 16/16 + carrier-sense above threshold
	SYNC_30_over_32_CARRIER   // 30/32 + carrier-sense above threshold
} sync_word_t;

typedef enum packet_config_e 
{
	PKTLEN_FIXED = 0,
	PKTLEN_VARIABLE,
	PKTLEN_INFINITE
} packet_config_t;

typedef struct radio_parms_s
{
	uint32_t        f_xtal;        // Crystal frequency (Hz)
	uint32_t        f_if;          // IF frequency (Hz)
	packet_config_t packet_config; // Packet length configuration
	uint8_t         packet_length; // Packet length if fixed
	sync_word_t     sync_ctl;      // Sync word control
	float           deviat_factor; // FSK-2 deviation is +/- data rate divised by this factor
	uint32_t        freq_word;     // Frequency 24 bit word FREQ[23..0]
    uint8_t         chanspc_m;     // Channel spacing mantissa 
    uint8_t         chanspc_e;     // Channel spacing exponent
	uint8_t         if_word;       // Intermediate frequency 5 bit word FREQ_IF[4:0] 
	uint8_t         drate_m;       // Data rate mantissa
	uint8_t         drate_e;       // Data rate exponent
	uint8_t         chanbw_m;      // Channel bandwidth mantissa
	uint8_t         chanbw_e;      // Channel bandwidth exponent
	uint8_t         deviat_m;      // Deviation mantissa
	uint8_t         deviat_e;      // Deviation exponent
} radio_parms_t;

void init_radio_parms(radio_parms_t *radio_parms);
int  init_radio(radio_parms_t *radio_parms,  spi_parms_t *spi_parms, arguments_t *arguments);
int  radio_set_packet_length(spi_parms_t *spi_parms, uint8_t pkt_len);
void print_radio_parms(radio_parms_t *radio_parms);
int  print_radio_status(spi_parms_t *spi_parms);
int  radio_set_packet_length(spi_parms_t *spi_parms, uint8_t pkt_len);
int  radio_transmit_test(spi_parms_t *spi_parms, arguments_t *arguments);

#endif
