/******************************************************************************/
/* PiCC1101  - Radio serial link using CC1101 module and Raspberry-Pi         */
/*                                                                            */
/* Radio link interface                                                       */
/*                                                                            */
/*                      (c) Edouard Griffiths, F4EXB, 2015                    */
/*                                                                            */
/******************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wiringPi.h>

#include "main.h"
#include "radio.h"
#include "pi_cc_spi.h"
#include "pi_cc_cc1100-cc2500.h"

char *state_names[] = {
    "SLEEP",            // 00
    "IDLE",             // 01
    "XOFF",             // 02
    "VCOON_MC",         // 03
    "REGON_MC",         // 04
    "MANCAL",           // 05
    "VCOON",            // 06
    "REGON",            // 07
    "STARTCAL",         // 08
    "BWBOOST",          // 09
    "FS_LOCK",          // 10
    "IFADCON",          // 11
    "ENDCAL",           // 12
    "RX",               // 13
    "RX_END",           // 14
    "RX_RST",           // 15
    "TXRX_SWITCH",      // 16
    "RXFIFO_OVERFLOW",  // 17
    "FSTXON",           // 18
    "TX",               // 19
    "TX_END",           // 20
    "RXTX_SWITCH",      // 21
    "TXFIFO_UNDERFLOW", // 22
    "undefined",        // 23
    "undefined",        // 24
    "undefined",        // 25
    "undefined",        // 26
    "undefined",        // 27
    "undefined",        // 28
    "undefined",        // 29
    "undefined",        // 30
    "undefined"         // 31
};

// 4x4 channel bandwidth limits
float chanbw_limits[] = {
    812000.0,
    650000.0,
    541000.0,
    464000.0,
    406000.0,
    325000.0,
    270000.0,
    232000.0,
    203000.0,
    162000.0,
    135000.0,
    116000.0,
    102000.0,
    81000.0,
    68000.0,
    58000.0
};

static volatile radio_int_data_t *radio_int_data = 0;

// === Interupt handlers ==========================================================================

void toto(void)
{

}

void int_packet_simple(void)
{
    uint8_t x_byte, int_packet, rx_bytes, rssi_dec, crc_lqi;
    int i;

    PI_CC_SPIReadStatus(radio_int_data->spi_parms, PI_CCxxx0_PKTSTATUS, &x_byte); // sense interrupt lines
    int_packet = x_byte & 0x01; // GDO0 (& 0x01) packet interrupt

    if (radio_int_data->mode == RADIOMODE_RX)
    {
        if (int_packet)
        {
            fprintf(stderr, "GDO0 rising edge\n");
            radio_int_data->packet_receive = 1; 
        }
        else
        {
            fprintf(stderr, "GDO0 falling edge\n");
            if (radio_int_data->packet_receive) // packet has been received
            {
                if (radio_int_data->packet_count == radio_int_data->packet_limit)
                {
                    radio_int_data->terminate;
                    return;
                }

                fprintf(stderr, "Packet #%d\n", radio_int_data->packet_count);

                PI_CC_SPIReadStatus(radio_int_data->spi_parms, PI_CCxxx0_RXBYTES, &rx_bytes);
                rx_bytes &= PI_CCxxx0_NUM_RXBYTES;
                fprintf(stderr, "Received %d bytes\n", rx_bytes);

                for (i=0; i<rx_bytes; i++)
                {
                    if (i<(rx_bytes-2)) // packet bytes
                    {
                        PI_CC_SPIReadReg(radio_int_data->spi_parms, PI_CCxxx0_RXFIFO, &x_byte);
                        radio_int_data->rx_buf[i] = x_byte;
                    }
                    else if (i == (rx_bytes-2)) // RSSI
                    {
                        PI_CC_SPIReadReg(radio_int_data->spi_parms, PI_CCxxx0_RXFIFO, &rssi_dec); 
                    }
                    else // LQI + CRC
                    {
                        PI_CC_SPIReadReg(radio_int_data->spi_parms, PI_CCxxx0_RXFIFO, &crc_lqi);
                    }

                    fprintf(stderr, "%X:%02X ", radio_int_data->spi_parms->rx[0] & 0x0F, radio_int_data->spi_parms->rx[1]);   
                }

                fprintf(stderr, "\nRSSI: %.1f dBm. LQI=%d. CRC=%d\n", 
                    rssi_dbm(rssi_dec),
                    0x7F - (crc_lqi & 0x7F),
                    (crc_lqi & PI_CCxxx0_CRC_OK)>>7);

                radio_int_data->packet_count++;
                radio_int_data->packet_receive = 0;        
            }
        }
    }
}

// === Static functions ===========================================================================

// ------------------------------------------------------------------------------------------------
// Calculate frequency word FREQ[23..0]
static uint32_t get_freq_word(uint32_t freq_xtal, uint32_t freq_hz)
// ------------------------------------------------------------------------------------------------
{
	uint64_t res; // calculate on 64 bits to save precision
	res = ((uint64_t) freq_hz * (uint64_t) (1<<16)) / ((uint64_t) freq_xtal);
	return (uint32_t) res;
}

// ------------------------------------------------------------------------------------------------
// Calculate frequency word FREQ[23..0]
static uint32_t get_if_word(uint32_t freq_xtal, uint32_t if_hz)
// ------------------------------------------------------------------------------------------------
{
	return (if_hz * (1<<10)) / freq_xtal;
}

// ------------------------------------------------------------------------------------------------
// Calculate modulation format word MOD_FORMAT[2..0]
static uint8_t get_mod_word(modulation_t modulation_code)
// ------------------------------------------------------------------------------------------------
{
	switch (modulation_code)
	{
		case MOD_OOK:
			return 3;
			break;
    	case MOD_FSK2:
    		return 0;
    		break;
    	case MOD_FSK4:
    		return 4;
    		break;
    	case MOD_MSK:
    		return 7;
    		break;
    	case MOD_GFSK:
    		return 1;
    		break;
    	default:
    		return 0;
	}
}

// ------------------------------------------------------------------------------------------------
// Calculate CHANBW words according to CC1101 bandwidth steps
static void get_chanbw_words(float bw, radio_parms_t *radio_parms)
// ------------------------------------------------------------------------------------------------
{
    uint8_t e_index, m_index;

    for (e_index=0; e_index<4; e_index++)
    {
        for (m_index=0; m_index<4; m_index++)
        {
            if (bw > chanbw_limits[4*e_index + m_index])
            {
                radio_parms->chanbw_e = e_index;
                radio_parms->chanbw_m = m_index;
                return;
            }
        }
    }

    radio_parms->chanbw_e = 3;
    radio_parms->chanbw_m = 3;
}

// ------------------------------------------------------------------------------------------------
// Calculate data rate, channel bandwidth and deviation words. Assumes 26 MHz crystal.
//   o DRATE = (Fxosc / 2^28) * (256 + DRATE_M) * 2^DRATE_E
//   o CHANBW = Fxosc / (8(4+CHANBW_M) * 2^CHANBW_E)
//   o DEVIATION = (Fxosc / 2^17) * (8 + DEVIATION_M) * 2^DEVIATION_E
static void get_rate_words(rate_t rate_code, modulation_t modulation_code, float modulation_index, radio_parms_t *radio_parms)
// ------------------------------------------------------------------------------------------------
{
    double drate, deviat, f_xtal;

    drate = (double) rate_values[rate_code];

    if ((modulation_code == MOD_FSK4) && (drate > 300000.0))
    {
        fprintf(stderr, "RADIO: forcibly set data rate to 300 kBaud for 4-FSK\n");
        drate = 300000.0;
    }

    deviat = drate * modulation_index;
    f_xtal = (double) radio_parms->f_xtal;

    get_chanbw_words(2.0*(deviat + drate), radio_parms); // Apply Carson's rule for bandwidth

    radio_parms->drate_e = (uint8_t) (floor(log2( drate*(1<<20) / f_xtal )));
    radio_parms->drate_m = (uint8_t) (((drate*(1<<28)) / (f_xtal * (1<<radio_parms->drate_e))) - 256);
    radio_parms->drate_e &= 0x0F; // it is 4 bits long

    radio_parms->deviat_e = (uint8_t) (floor(log2( deviat*(1<<14) / f_xtal )));
    radio_parms->deviat_m = (uint8_t) (((deviat*(1<<17)) / (f_xtal * (1<<radio_parms->deviat_e))) - 8);
    radio_parms->deviat_e &= 0x07; // it is 3 bits long
    radio_parms->deviat_m &= 0x07; // it is 3 bits long

    radio_parms->chanspc_e &= 0x03; // it is 2 bits long
}

// ------------------------------------------------------------------------------------------------
// Set interrupt routines and interrupt data block
static void init_radio_int_data(radio_int_data_t *radio_int_data)
// ------------------------------------------------------------------------------------------------
{
    radio_int_data->terminate = 0;
    radio_int_data->packet_count = 0;
    radio_int_data->packet_receive = 0;
    radio_int_data->packet_send = 0;
}

// === Public functions ===========================================================================

// ------------------------------------------------------------------------------------------------
// Calculate RSSI in dBm from decimal RSSI read out of RSSI status register
float rssi_dbm(uint8_t rssi_dec)
// ------------------------------------------------------------------------------------------------
{
    if (rssi_dec < 128)
    {
        return (rssi_dec / 2.0) - 74.0;
    }
    else
    {
        return ((rssi_dec - 256) / 2.0) - 74.0;
    }
}

// ------------------------------------------------------------------------------------------------
// Initialize constant radio parameters
void init_radio_parms(radio_parms_t *radio_parms)
// ------------------------------------------------------------------------------------------------
{
	radio_parms->f_xtal        = 26000000;         // 26 MHz Xtal
	radio_parms->f_if          = 310000;           // 304.6875 kHz (lowest point below 310 kHz)
	radio_parms->sync_ctl      = SYNC_30_over_32;  // 30/32 sync word bits detected
    radio_parms->chanspc_m     = 0;                // Do not use channel spacing for the moment defaulting to 0
    radio_parms->chanspc_e     = 0;                // Do not use channel spacing for the moment defaulting to 0
    radio_parms->packet_config = PKTLEN_FIXED;     // Use fixed packet length
}

// ------------------------------------------------------------------------------------------------
// Initialize the radio link interface
int init_radio(radio_parms_t *radio_parms, spi_parms_t *spi_parms, arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    int ret = 0;
    uint8_t  reg_word;


    wiringPiSetup(); // initialize Wiring Pi library and GDOx interrupt routines
    wiringPiISR(WPI_GDO0, INT_EDGE_BOTH, &toto); // set interrupt handler for paket interrupts

    // open SPI link
    PI_CC_SPIParmsDefaults(spi_parms);
    ret = PI_CC_SPISetup(spi_parms, arguments);

    if (ret != 0)
    {
        fprintf(stderr, "RADIO: cannot open SPI link, RC=%d\n", ret);
        return ret;
    }

    ret = PI_CC_PowerupResetCCxxxx(spi_parms); // reset chip

    if (ret != 0)
    {
        fprintf(stderr, "RADIO: cannot reset CC1101 chip, RC=%d\n", ret);
        return ret;
    }

    radio_parms->packet_length = arguments->packet_length;  // Packet length
    get_rate_words(arguments->rate, arguments->modulation, arguments->modulation_index, radio_parms);

    // Write register settings

    // IOCFG2 = 0x00: Set in Rx mode (0x02 for Tx mode)
    // o 0x00: Asserts when RX FIFO is filled at or above the RX FIFO threshold. 
    //         De-asserts when RX FIFO is drained below the same threshold.
    // o 0x02: Asserts when the TX FIFO is filled at or above the TX FIFO threshold.
    //         De-asserts when the TX FIFO is below the same threshold.
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_IOCFG2,   0x00); // GDO2 output pin config.

	// IOCFG0 = 0x06: Asserts when sync word has been sent / received, and de-asserts at the
	// end of the packet. In RX, the pin will de-assert when the optional address
	// check fails or the RX FIFO overflows. In TX the pin will de-assert if the TX
	// FIFO underflows:    
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_IOCFG0,   0x06); // GDO0 output pin config.

    // FIFO_THR = 14: 
    // o 5 bytes in TX FIFO (59 available spaces)
    // o 60 bytes in the RX FIFO
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_FIFOTHR,  0x0E); // FIFO threshold.

    // PKTLEN: packet length up to 255 bytes. 
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_PKTLEN, radio_parms->packet_length); // Packet length.

    // PKTCTRL0: Packet automation control #0
    // . bit  7:   unused
    // . bit  6:   0  -> whitening off
    // . bits 5:4: 00 -> normal mode use FIFOs for Rx and Tx
    // . bit  3:   unused
    // . bit  2:   1  -> CRC enabled
    // . bits 1:0: xx -> Packet length mode. Taken from radio config.
    reg_word = (arguments->whitening<<6) + 0x04 + (int) radio_parms->packet_config;
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_PKTCTRL0, reg_word); // Packet automation control.

    // PKTCTRL1: Packet automation control #1
    // . bits 7:5: 000 -> Preamble quality estimator threshold
    // . bit  4:   unused
    // . bit  3:   0   -> Automatic flush of Rx FIFO disabled (too many side constraints see doc)
    // . bit  2:   1   -> Append two status bytes to the payload (RSSI and LQI + CRC OK)
    // . bits 1:0: 00  -> No address check of received packets
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_PKTCTRL1, 0x04); // Packet automation control.

    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_ADDR,     0x00); // Device address for packet filtration (unused, see just above).
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_CHANNR,   0x00); // Channel number (unused, use direct frequency programming).

    // FSCTRL0: Frequency offset added to the base frequency before being used by the
    // frequency synthesizer. (2s-complement). Multiplied by Fxtal/2^14
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_FSCTRL0,  0x00); // Freq synthesizer control.

    // FSCTRL1: The desired IF frequency to employ in RX. Subtracted from FS base frequency
    // in RX and controls the digital complex mixer in the demodulator. Multiplied by Fxtal/2^10
    // Here 0.3046875 MHz (lowest point below 310 kHz)
	radio_parms->if_word = get_if_word(radio_parms->f_xtal, radio_parms->f_if);    
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_FSCTRL1, (radio_parms->if_word & 0x1F)); // Freq synthesizer control.

    // FREQ2..0: Base frequency for the frequency sythesizer
    // Fo = (Fxosc / 2^16) * FREQ[23..0]
    // FREQ2 is FREQ[23..16]
    // FREQ1 is FREQ[15..8]
    // FREQ0 is FREQ[7..0]
    // Fxtal = 26 MHz and FREQ = 0x10A762 => Fo = 432.99981689453125 MHz
    radio_parms->freq_word = get_freq_word(radio_parms->f_xtal, arguments->freq_hz);
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_FREQ2,    ((radio_parms->freq_word>>16) & 0xFF)); // Freq control word, high byte
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_FREQ1,    ((radio_parms->freq_word>>8)  & 0xFF)); // Freq control word, mid byte.
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_FREQ0,    (radio_parms->freq_word & 0xFF));       // Freq control word, low byte.

    // MODCFG4 Modem configuration - bandwidth and data rate exponent
    // High nibble: Sets the decimation ratio for the delta-sigma ADC input stream hence the channel bandwidth
    // . bits 7:6: 0  -> CHANBW_E: exponent parameter (see next)
    // . bits 5:4: 2  -> CHANBW_M: mantissa parameter as per:
    //      BW = Fxosc / 8(4+CHANBW_M).2^CHANBW_E => Here: BW = 26/48 MHz = 541.67 kHz
    //      Factory defaults: M=0, E=1 => BW = 26/128 ~ 203 kHz
    // Low nibble:
    // . bits 3:0: 13 -> DRATE_E: data rate base 2 exponent => here 13 (multiply by 8192)
    reg_word = (radio_parms->chanbw_e<<6) + (radio_parms->chanbw_m<<4) + radio_parms->drate_e;
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_MDMCFG4,  reg_word); // Modem configuration.

    // MODCFG3 Modem configuration: DRATE_M data rate mantissa as per formula:
    //    Rate = (256 + DRATE_M).2^DRATE_E.Fxosc / 2^28 
    // Here DRATE_M = 59, DRATE_E = 13 => Rate = 250 kBaud
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_MDMCFG3,  radio_parms->drate_m); // Modem configuration.

    // MODCFG2 Modem configuration: DC block, modulation, Manchester, sync word
    // o bit 7:    0   -> Enable DC blocking (1: disable)
    // o bits 6:4: xxx -> (provided)
    // o bit 3:    0   -> Manchester disabled (1: enable)
    // o bits 2:0: 011 -> Sync word qualifier is 30/32 (static init in radio interface)
    reg_word = (get_mod_word(arguments->modulation)<<4) + radio_parms->sync_ctl;
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_MDMCFG2,  reg_word); // Modem configuration.

    // MODCFG1 Modem configuration: FEC, Preamble, exponent for channel spacing
    // o bit 7:    0   -> FEC disabled (1: enable)
    // o bits 6:4: 2   -> number of preamble bytes (0:2, 1:3, 2:4, 3:6, 4:8, 5:12, 6:16, 7:24)
    // o bits 3:2: unused
    // o bits 1:0: CHANSPC_E: exponent of channel spacing (here: 2)
    reg_word = 0x20 + radio_parms->chanspc_e + arguments->fec<<7 + ((int) arguments->preamble)<<4;
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_MDMCFG1,  reg_word); // Modem configuration.

    // MODCFG0 Modem configuration: CHANSPC_M: mantissa of channel spacing following this formula:
    //    Df = (Fxosc / 2^18) * (256 + CHANSPC_M) * 2^CHANSPC_E
    //    Here: (26 /  ) * 2016 = 0.199951171875 MHz (200 kHz)
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_MDMCFG0,  radio_parms->chanspc_m); // Modem configuration.

    // DEVIATN: Modem deviation
    // o bit 7:    0   -> not used
    // o bits 6:4: 0   -> DEVIATION_E: deviation exponent
    // o bit 3:    0   -> not used
    // o bits 2:0: 0   -> DEVIATION_M: deviation mantissa
    //
    //   Modulation  Formula
    //
    //   2-FSK    |  
    //   4-FSK    :  Df = (Fxosc / 2^17) * (8 + DEVIATION_M) * 2^DEVIATION_E : Here: 1.5869140625 kHz
    //   GFSK     |
    //
    //   MSK      :  Tx: not well documented, Rx: no effect
    //
    //   OOK      : No effect
    //    
    reg_word = (radio_parms->deviat_e<<4) + (radio_parms->deviat_m);
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_DEVIATN,  reg_word); // Modem dev (when FSK mod en)

    // MCSM2: Main Radio State Machine. See documentation.
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_MCSM2 ,   0x00); //MainRadio Cntrl State Machine

    // MCSM1: Main Radio State Machine. 
    // o bits 7:6: not used
    // o bits 5:4: CCA_MODE: Clear Channel Indicator 
    //   0 (00): Always clear
    //   1 (01): Clear if RSSI below threshold
    //   2 (10): Always claar unless receiving a packet
    //   3 (11): Claar if RSSI below threshold unless receiving a packet
    // o bits 3:2: RXOFF_MODE: Select to what state it should go when a packet has been received
    //   0 (00): IDLE
    //   1 (01): FSTXON
    //   2 (10): TX
    //   3 (11): RX (stay)
    // o bits 1:0: TXOFF_MODE: Select what should happen when a packet has been sent
    //   0 (00): IDLE
    //   1 (01): FSTXON
    //   2 (10): TX (stay)
    //   3 (11): RX
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_MCSM1 ,   0x3F); //MainRadio Cntrl State Machine

    // MCSM0: Main Radio State Machine.
    // o bits 7:6: not used
    // o bits 5:4: FS_AUTOCAL: When to perform automatic calibration
    //   0 (00): Never i.e. manually via strobe command
    //   1 (01): When going from IDLE to RX or TX (or FSTXON)
    //   2 (10): When going from RX or TX back to IDLE automatically
    //   3 (11): Every 4th time when going from RX or TX to IDLE automatically
    // o bits 3:2: PO_TIMEOUT: 
    //   Value : Exp: Timeout after XOSC start
    //   0 (00):   1: Approx. 2.3 – 2.4 μs
    //   1 (01):  16: Approx. 37 – 39 μs
    //   2 (10):  64: Approx. 149 – 155 μs
    //   3 (11): 256: Approx. 597 – 620 μs
    // o bit 1: PIN_CTRL_EN:   Enables the pin radio control option
    // o bit 0: XOSC_FORCE_ON: Force the XOSC to stay on in the SLEEP state.
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_MCSM0 ,   0x18); //MainRadio Cntrl State Machine

    // FOCCFG: Frequency Offset Compensation Configuration.
    // o bits 7:6: not used
    // o bit 5:    If set, the demodulator freezes the frequency offset compensation and clock
    //             recovery feedback loops until the CS signal goes high.
    // o bits 4:3: The frequency compensation loop gain to be used before a sync word is detected.
    //   0 (00): K
    //   1 (01): 2K
    //   2 (10): 3K
    //   3 (11): 4K
    // o bit 2: FOC_POST_K: The frequency compensation loop gain to be used after a sync word is detected.
    //   0: Same as FOC_PRE_K
    //   1: K/2
    // o bits 1:0: FOC_LIMIT: The saturation point for the frequency offset compensation algorithm:
    //   0 (00): ±0 (no frequency offset compensation)
    //   1 (01): ±BW CHAN /8
    //   2 (10): ±BW CHAN /4
    //   3 (11): ±BW CHAN /2
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_FOCCFG,   0x1D); // Freq Offset Compens. Config

    // BSCFG:Bit Synchronization Configuration
    // o bits 7:6: BS_PRE_KI: Clock recovery loop integral gain before sync word
    //   0 (00): Ki
    //   1 (01): 2Ki
    //   2 (10): 3Ki
    //   3 (11): 4Ki
    // o bits 5:4: BS_PRE_KP: Clock recovery loop proportional gain before sync word
    //   0 (00): Kp
    //   1 (01): 2Kp
    //   2 (10): 3Kp
    //   3 (11): 4Kp
    // o bit 3: BS_POST_KI: Clock recovery loop integral gain after sync word
    //   0: Same as BS_PRE_KI
    //   1: Ki/2
    // o bit 2: BS_POST_KP: Clock recovery loop proportional gain after sync word
    //   0: Same as BS_PRE_KP
    //   1: Kp
    // o bits 1:0: BS_LIMIT: Data rate offset saturation (max data rate difference)
    //   0 (00): ±0 (No data rate offset compensation performed)
    //   1 (01): ±3.125 % data rate offset
    //   2 (10): ±6.25 % data rate offset
    //   3 (11): ±12.5 % data rate offset
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_BSCFG,    0x1C); //  Bit synchronization config.

    // AGCCTRL2: AGC Control
    // o bits 7:6: MAX_DVGA_GAIN. Allowable DVGA settings
    //   0 (00): All gain settings can be used
    //   1 (01): The highest gain setting can not be used
    //   2 (10): The 2 highest gain settings can not be used
    //   3 (11): The 3 highest gain settings can not be used
    // o bits 5:3: MAX_LNA_GAIN. Maximum allowable LNA + LNA 2 gain relative to the maximum possible gain.
    //   0 (000): Maximum possible LNA + LNA 2 gain
    //   1 (001): Approx. 2.6 dB below maximum possible gain
    //   2 (010): Approx. 6.1 dB below maximum possible gain
    //   3 (011): Approx. 7.4 dB below maximum possible gain
    //   4 (100): Approx. 9.2 dB below maximum possible gain
    //   5 (101): Approx. 11.5 dB below maximum possible gain
    //   6 (110): Approx. 14.6 dB below maximum possible gain
    //   7 (111): Approx. 17.1 dB below maximum possible gain
    // o bits 2:0: MAGN_TARGET: target value for the averaged amplitude from the digital channel filter (1 LSB = 0 dB).
    //   0 (000): 24 dB
    //   1 (001): 27 dB
    //   2 (010): 30 dB
    //   3 (011): 33 dB
    //   4 (100): 36 dB
    //   5 (101): 38 dB
    //   6 (110): 40 dB
    //   7 (111): 42 dB
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_AGCCTRL2, 0xC7); // AGC control.

    // AGCCTRL1: AGC Control
    // o bit 7: not used
    // o bit 6: AGC_LNA_PRIORITY: Selects between two different strategies for LNA and LNA 2 gain
    //   0: the LNA 2 gain is decreased to minimum before decreasing LNA gain
    //   1: the LNA gain is decreased first.
    // o bits 5:4: CARRIER_SENSE_REL_THR: Sets the relative change threshold for asserting carrier sense
    //   0 (00): Relative carrier sense threshold disabled
    //   1 (01): 6 dB increase in RSSI value
    //   2 (10): 10 dB increase in RSSI value
    //   3 (11): 14 dB increase in RSSI value
    // o bits 3:0: CARRIER_SENSE_ABS_THR: Sets the absolute RSSI threshold for asserting carrier sense. 
    //   The 2-complement signed threshold is programmed in steps of 1 dB and is relative to the MAGN_TARGET setting.
    //   0 is at MAGN_TARGET setting.
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_AGCCTRL1, 0x00); // AGC control.

    // AGCCTRL0: AGC Control
    // o bits 7:6: HYST_LEVEL: Sets the level of hysteresis on the magnitude deviation
    //   0 (00): No hysteresis, small symmetric dead zone, high gain
    //   1 (01): Low hysteresis, small asymmetric dead zone, medium gain
    //   2 (10): Medium hysteresis, medium asymmetric dead zone, medium gain
    //   3 (11): Large hysteresis, large asymmetric dead zone, low gain
    // o bits 5:4: WAIT_TIME: Sets the number of channel filter samples from a gain adjustment has
    //   been made until the AGC algorithm starts accumulating new samples.
    //   0 (00):  8
    //   1 (01): 16
    //   2 (10): 24
    //   3 (11): 32
    // o bits 3:2: AGC_FREEZE: Control when the AGC gain should be frozen.
    //   0 (00): Normal operation. Always adjust gain when required.
    //   1 (01): The gain setting is frozen when a sync word has been found.
    //   2 (10): Manually freeze the analogue gain setting and continue to adjust the digital gain. 
    //   3 (11): Manually freezes both the analogue and the digital gain setting. Used for manually overriding the gain.
    // o bits 0:1: FILTER_LENGTH: 
    //   2-FSK, 4-FSK, MSK: Sets the averaging length for the amplitude from the channel filter.    |  
    //   ASK ,OOK: Sets the OOK/ASK decision boundary for OOK/ASK reception.
    //   Value : #samples: OOK/ASK decixion boundary
    //   0 (00):        8: 4 dB
    //   1 (01):       16: 8 dB
    //   2 (10):       32: 12 dB
    //   3 (11):       64: 16 dB  
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_AGCCTRL0, 0xB2); // AGC control.

    // FREND1: Front End RX Configuration
    // o bits 7:6: LNA_CURRENT: Adjusts front-end LNA PTAT current output
    // o bits 5:4: LNA2MIX_CURRENT: Adjusts front-end PTAT outputs
    // o bits 3:2: LODIV_BUF_CURRENT_RX: Adjusts current in RX LO buffer (LO input to mixer)
    // o bits 1:0: MIX_CURRENT: Adjusts current in mixer
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_FREND1,   0xB6); // Front end RX configuration.

    // FREND0: Front End TX Configuration
    // o bits 7:6: not used
    // o bits 5:4: LODIV_BUF_CURRENT_TX: Adjusts current TX LO buffer (input to PA). The value to use
    //   in this field is given by the SmartRF Studio software
    // o bit 3: not used
    // o bits 1:0: PA_POWER: Selects PA power setting. This value is an index to the PATABLE, 
    //   which can be programmed with up to 8 different PA settings. In OOK/ASK mode, this selects the PATABLE
    //   index to use when transmitting a ‘1’. PATABLE index zero is used in OOK/ASK when transmitting a ‘0’. 
    //   The PATABLE settings from index ‘0’ to the PA_POWER value are used for ASK TX shaping, 
    //   and for power ramp-up/ramp-down at the start/end of transmission in all TX modulation formats.
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_FREND0,   0x10); // Front end RX configuration.

    // FSCAL3: Frequency Synthesizer Calibration
    // o bits 7:6: The value to write in this field before calibration is given by the SmartRF
    //   Studio software.
    // o bits 5:4: CHP_CURR_CAL_EN: Disable charge pump calibration stage when 0.
    // o bits 3:0: FSCAL3: Frequency synthesizer calibration result register.
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_FSCAL3,   0xEA); // Frequency synthesizer cal.

    // FSCAL2: Frequency Synthesizer Calibration
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_FSCAL2,   0x0A); // Frequency synthesizer cal.
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_FSCAL1,   0x00); // Frequency synthesizer cal.
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_FSCAL0,   0x11); // Frequency synthesizer cal.
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_FSTEST,   0x59); // Frequency synthesizer cal.

    // TEST2: Various test settings. The value to write in this field is given by the SmartRF Studio software.
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_TEST2,    0x88); // Various test settings.

    // TEST1: Various test settings. The value to write in this field is given by the SmartRF Studio software.
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_TEST1,    0x31); // Various test settings.

    // TEST0: Various test settings. The value to write in this field is given by the SmartRF Studio software.
    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_TEST0,    0x09); // Various test settings.

    if (arguments->verbose_level > 0)
    {
        print_radio_parms(radio_parms);
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
// Print status registers to stderr
int  print_radio_status(spi_parms_t *spi_parms)
// ------------------------------------------------------------------------------------------------
{
    uint8_t regs[14];
    uint8_t reg_index = PI_CCxxx0_PARTNUM;
    int ret = 0;

    memset(regs, 0, 14);

    while ((ret == 0) && (reg_index <= PI_CCxxx0_RXBYTES))
    {
        ret = PI_CC_SPIReadStatus(spi_parms, reg_index, &regs[reg_index - PI_CCxxx0_PARTNUM]);
        reg_index++;
    }

    if (ret != 0)
    {
        fprintf(stderr, "RADIO: read status register %02X failed\n", reg_index-1);
        return ret;
    }

    fprintf(stderr, "Part number ...........: %d\n", regs[0]);
    fprintf(stderr, "Version ...............: %d\n", regs[1]);
    fprintf(stderr, "Freq offset estimate ..: %d\n", regs[2]);
    fprintf(stderr, "CRC OK ................: %d\n", ((regs[3] & 0x80)>>7));
    fprintf(stderr, "LQI ...................: %d\n", regs[3] & 0x7F);
    fprintf(stderr, "RSSI ..................: %.1f dBm\n", rssi_dbm(regs[4]));
    fprintf(stderr, "Radio FSM state .......: %s\n", state_names[regs[5] & 0x1F]);
    fprintf(stderr, "WOR time ..............: %d\n", ((regs[6] << 8) + regs[7]));
    fprintf(stderr, "Carrier Sense .........: %d\n", ((regs[8] & 0x40)>>6));
    fprintf(stderr, "Preamble Qual Reached .: %d\n", ((regs[8] & 0x20)>>5));
    fprintf(stderr, "Clear channel .........: %d\n", ((regs[8] & 0x10)>>4));
    fprintf(stderr, "Start of frame delim ..: %d\n", ((regs[8] & 0x08)>>3));
    fprintf(stderr, "GDO2 ..................: %d\n", ((regs[8] & 0x04)>>2));
    fprintf(stderr, "GDO0 ..................: %d\n", ((regs[8] & 0x01)));
    fprintf(stderr, "VCO VC DAC ............: %d\n", regs[9]);
    fprintf(stderr, "FIFO Tx underflow .....: %d\n", ((regs[10] & 0x80)>>7));
    fprintf(stderr, "FIFO Tx bytes .........: %d\n", regs[10] & 0x7F);
    fprintf(stderr, "FIFO Rx overflow ......: %d\n", ((regs[11] & 0x80)>>7));
    fprintf(stderr, "FIFO Rx bytes .........: %d\n", regs[11] & 0x7F);
    fprintf(stderr, "RC CRTL0 ..............: %d\n", (regs[12] & 0x7F));
    fprintf(stderr, "RC CRTL1 ..............: %d\n", (regs[13] & 0x7F));

    return ret;
}


// ------------------------------------------------------------------------------------------------
// Print actual radio link parameters once initialized
//   o Operating frequency ..: Fo   = (Fxosc / 2^16) * FREQ[23..0]
//   o Channel spacing ......: Df   = (Fxosc / 2^18) * (256 + CHANSPC_M) * 2^CHANSPC_E
//   o Channel bandwidth ....: BW   = Fxosc / (8 * (4+CHANBW_M) * 2^CHANBW_E)
//   o Data rate (Baud) .....: Rate = (Fxosc / 2^28) * (256 + DRATE_M) * 2^DRATE_E
//   o Deviation ............: Df   = (Fxosc / 2^17) * (8 + DEVIATION_M) * 2^DEVIATION_E

void print_radio_parms(radio_parms_t *radio_parms)
// ------------------------------------------------------------------------------------------------
{
    fprintf(stderr, "\n--- Actual radio channel parameters ---\n");
    fprintf(stderr, "Operating frequency ....: %.3f MHz (W=%d)\n", 
        ((radio_parms->f_xtal/1e6) / (1<<16))*radio_parms->freq_word, radio_parms->freq_word);
    fprintf(stderr, "Intermediate frequency .: %.3f kHz (W=%d)\n", 
        ((radio_parms->f_xtal/1e3) / (1<<10))*radio_parms->if_word, radio_parms->if_word);
    fprintf(stderr, "Channel spacing ........: %.3f kHz (M=%d, E=%d)\n", 
        ((radio_parms->f_xtal/1e3) / (1<<18))*(256+radio_parms->chanspc_m)*(1<<radio_parms->chanspc_e), radio_parms->chanspc_m, radio_parms->chanspc_e);
    fprintf(stderr, "Channel bandwidth.......: %.3f kHz (M=%d, E=%d)\n",
        (radio_parms->f_xtal/1e3) / (8*(4+radio_parms->chanbw_m)*(1<<radio_parms->chanbw_e)), radio_parms->chanbw_m, radio_parms->chanbw_e);
    fprintf(stderr, "Data rate ..............: %.1f Baud (M=%d, E=%d)\n",
        ((float) (radio_parms->f_xtal) / (1<<28)) * (256 + radio_parms->drate_m) * (1<<radio_parms->drate_e), radio_parms->drate_m, radio_parms->drate_e);
    fprintf(stderr, "Deviation ..............: %.3f kHz (M=%d, E=%d)\n",
        ((radio_parms->f_xtal/1e3) / (1<<17)) * (8 + radio_parms->deviat_m) * (1<<radio_parms->deviat_e), radio_parms->deviat_m, radio_parms->deviat_e);
    fprintf(stderr, "Packet length ..........: %d bytes\n",
        radio_parms->packet_length);
}

// ------------------------------------------------------------------------------------------------
// Set fixed packet length
int radio_set_packet_length(spi_parms_t *spi_parms, uint8_t pkt_len)
// ------------------------------------------------------------------------------------------------
{
    return PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_PKTLEN, pkt_len); // Packet length.
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

    if (arguments->verbose_level > 0)
    {
        fprintf(stderr, "Estimated Tx delay is %lld us\n", tx_delay);
    }

    if (strlen(arguments->test_phrase) < PI_CCxxx0_FIFO_SIZE)
    {
        test_length = strlen(arguments->test_phrase);
    }
    else
    {
        fprintf(stderr, "Test phrase too long. Truncated to CC1101 FIFO size\n");
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

    fprintf(stderr, "Sending test packet of size %d %d times\n", tx_length, arguments->repetition);

    for (i=0; i<arguments->repetition; i++)
    {
        fprintf(stderr, "Test #%d\n", i+1);

        for (j=0; j<tx_length; j++)
        {
            PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_TXFIFO, tx_buf[j]);
            fprintf(stderr, "%02X ", spi_parms->rx[0]);
        }

        fprintf(stderr, "\n");
        ret = PI_CC_SPIStrobe(spi_parms, PI_CCxxx0_STX);
        
        usleep(tx_delay);

        print_radio_status(spi_parms);
    }
}

// ------------------------------------------------------------------------------------------------
// Reception test with polling of registers
int radio_receive_test_int(spi_parms_t *spi_parms, arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    radio_int_data_t *data_block = malloc(sizeof(radio_int_data_t));

    init_radio_int_data(data_block);

    data_block->spi_parms = spi_parms;
    data_block->packet_limit = arguments->repetition;
    uint32_t wait_us = 4*8000000 / rate_values[arguments->rate]; // 4 2-FSK symbols delay

    PI_CC_SPIWriteReg(spi_parms, PI_CCxxx0_IOCFG2,   0x00); // GDO2 output pin config RX mode
    PI_CC_SPIStrobe(spi_parms, PI_CCxxx0_SFRX); // Flush Rx FIFO
    PI_CC_SPIStrobe(spi_parms, PI_CCxxx0_SRX); // Enter Rx mode

    radio_int_data = data_block;

    fprintf(stderr, "Starting...\n");

    while(!(data_block->terminate))
    {
        usleep(wait_us); 
    }

    free(data_block);
}

// ------------------------------------------------------------------------------------------------
// Reception test with polling of registers
int radio_receive_test(spi_parms_t *spi_parms, arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    uint8_t iterations, rx_bytes, fsm_state, rssi_dec, crc_lqi, x_byte, pkt_on;
    uint8_t rx_buf[PI_CCxxx0_FIFO_SIZE+1];
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
        fprintf(stderr, "Packet #%d\n", iterations+1);
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
                PI_CC_SPIReadStatus(spi_parms, PI_CCxxx0_RXBYTES, &rx_bytes);
                rx_bytes &= PI_CCxxx0_NUM_RXBYTES;
                fprintf(stderr, "Received %d bytes\n", rx_bytes);

                for (i=0; i<rx_bytes; i++)
                {
                    if (i<arguments->packet_length) // packet bytes
                    {
                        PI_CC_SPIReadReg(spi_parms, PI_CCxxx0_RXFIFO, &rx_buf[i]);    
                    }
                    else if (i == arguments->packet_length) // RSSI
                    {
                        PI_CC_SPIReadReg(spi_parms, PI_CCxxx0_RXFIFO, &rssi_dec); 
                    }
                    else // LQI + CRC
                    {
                        PI_CC_SPIReadReg(spi_parms, PI_CCxxx0_RXFIFO, &crc_lqi);
                    }

                    fprintf(stderr, "%X:%02X ", spi_parms->rx[0] & 0x0F, spi_parms->rx[1]);   
                }

                fprintf(stderr, "\nRSSI: %.1f dBm. LQI=%d. CRC=%d\n", 
                    rssi_dbm(rssi_dec),
                    0x7F - (crc_lqi & 0x7F),
                    (crc_lqi & PI_CCxxx0_CRC_OK)>>7);

                break;
            }

            usleep(poll_us);
        }

        fprintf(stderr, "\"%s\"\n", rx_buf);
    }

    fprintf(stderr, "Done\n");
}