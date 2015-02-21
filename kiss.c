/******************************************************************************/
/* PiCC1101  - Radio serial link using CC1101 module and Raspberry-Pi         */
/*                                                                            */
/* KISS AX.25 blocks handling                                                 */
/*                                                                            */
/*                      (c) Edouard Griffiths, F4EXB, 2015                    */
/*                                                                            */
/******************************************************************************/

#include "kiss.h"

// === Public functions ===========================================================================

// ------------------------------------------------------------------------------------------------
// Remove KISS signalling
void pack_kiss(uint8_t *kiss_block, uint8_t *packed_block, size_t *size)
// ------------------------------------------------------------------------------------------------
{
	size_t  new_size = 0, i;
	uint8_t fesc = 0;

	for (i=1; i<*size-1; i++)
	{
		if (kiss_block[i] == 0xDB) // FESC
		{
			fesc = 1;
			continue;
		}
		if (fesc)
		{
			if (kiss_block[i] == 0xDC) // TFEND
			{
				packed_block[new_size++] = 0x0C; // FEND
			}
			else if (kiss_block[i] == 0xDD) // TFESC
			{
				packed_block[new_size++] = 0xDB; // FESC	
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
void unpack_kiss(uint8_t *kiss_block, uint8_t *packed_block, size_t *size)
// ------------------------------------------------------------------------------------------------
{
	size_t  new_size = 0, i;

	kiss_block[0] = 0x0C; // FEND

	for (i=0; i<*size; i++)
	{
		if (packed_block[i] == 0x0C) // FEND
		{
			kiss_block[new_size++] = 0xDB; // FESC
			kiss_block[new_size++] = 0xDC; // TFEND
		}
		else if (packed_block[i] == 0xDB) // FESC
		{
			kiss_block[new_size++] = 0xDB; // FESC
			kiss_block[new_size++] = 0xDD; // TFESC
		}
		else
		{
			kiss_block[new_size++] = packed_block[i];
		}
	}

	kiss_block[new_size++] = 0x0C; // FEND
	*size = new_size;
}