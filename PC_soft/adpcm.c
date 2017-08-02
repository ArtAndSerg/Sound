/********************************************************************
* FileName:		ADPCM.C
* Dependencies: adpcm.h
* Processor:	PIC18
* Hardware:		PICDEM HPC Explorer Board with Speech Playback PICtail
* Compiler:		MPLAB C18 version v3.11
* Linker:		MPLINK v4.11
* Company:		Microchip Technology, Inc.
*
* Software License Agreement:
*
* The software supplied herewith by Microchip Technology Incorporated
* (the "Company") is intended and supplied to you, the Company's
* customer, for use solely and exclusively with products manufactured
* by the Company. 
*
* The software is owned by the Company and/or its supplier, and is 
* protected under applicable copyright laws. All rights are reserved. 
* Any use in violation of the foregoing restrictions may subject the 
* user to criminal sanctions under applicable laws, as well as to 
* civil liability for the breach of the terms and conditions of this 
* license.
*
* THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES, 
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED 
* TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
* PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT, 
* IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR 
* CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
*
*********************************************************************
* File Description:
*   This file contains the ADPCM encode and decode routines.  These
*   routines were obtained from the Interactive Multimedia Association's
*   Reference ADPCM algorithm.  This algorithm was first implemented by
*   Intel/DVI.
*
* Change History:
* $Log$
* v1.00 - first released version of APDCM on PIC18
********************************************************************/

#include "adpcm.h"


/* Table of index changes */
const signed char IndexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

/* Quantizer step size lookup table */
const unsigned short StepSizeTable[89] = {
   7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
   19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
   50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
   130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
   337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
   876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
   2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
   5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
   15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

/*****************************************************************************
*   ADPCMDecoder - ADPCM decoder routine                                     *
******************************************************************************
*   Input variables:                                                         *
*      unsigned char code - 8-bit number containing the 4-bit ADPCM code     *
*      struct ADPCMstate *state - ADPCM structure                            *
*   Return variables:                                                        *
*      unsigned short - 16-bit unsigned speech sample                        *
*****************************************************************************/
unsigned short ADPCMDecoder(unsigned char code)
{
   static long step;		/* Quantizer step size */
   static long predsample;		/* Output of ADPCM predictor */
   static long diffq;		/* Dequantized predicted difference */
   static char index;		/* Index into step size table */

   /* Restore previous values of predicted sample and quantizer step size index */
   //predsample = (long)(state->prevsample);
   //index = state->previndex;

   /* Find quantizer step size from lookup table using index */
   step = StepSizeTable[index];

   /* Inverse quantize the ADPCM code into a difference using the quantizer step size */
   diffq = step >> 3;
   if( code & 4 )
      diffq += step;
   if( code & 2 )
      diffq += step >> 1;
   if( code & 1 )
      diffq += step >> 2;

   /* Add the difference to the predicted sample */
   if( code & 8 )
      predsample -= diffq;
   else
      predsample += diffq;

   /* Check for overflow of the new predicted sample */
   if( predsample > 65535 )
      predsample = 65535;
   else if( predsample < 0 )
      predsample = 0;

   /* Find new quantizer step size by adding the old index and a
      table lookup using the ADPCM code */
   index += IndexTable[code];

   /* Check for overflow of the new quantizer step size index */
   if( index < 0 )
      index = 0;
   if( index > 88 )
      index = 88;


   /* Save predicted sample and quantizer step size index for next iteration */
  // state->prevsample = (unsigned short)(predsample);
  // state->previndex = index;

   /* Return the new speech sample */
   return( (unsigned short)(predsample) );

}


/************************************************************************
*	ADPCMEncoder - ADPCM encoder routine                                 
*************************************************************************
*	Input Variables:                                                     
*		signed long sample - 16-bit signed speech sample             
*	Return Variable:                                                     
*		char - 8-bit number containing the 4-bit ADPCM code          
************************************************************************/
unsigned char ADPCMEncoder( unsigned short sample)
{
   static long step;		/* Quantizer step size */
   static long predsample = 0;		/* Output of ADPCM predictor */
   static long diffq, diff;		/* Dequantized predicted difference */
   static char index= 0, code;		/* Index into step size table */

	// Restore previous values of predicted sample and quantizer step
	// size index
	//predsample = (short long)(state->prevsample);
	//index = state->previndex;
	step = StepSizeTable[index];

	// Compute the difference between the acutal sample (sample) and the
	// the predicted sample (predsample)
	diff = sample - predsample;
	if(diff >= 0)
		code = 0;
	else
	{
		code = 8;
		diff = -diff;
	}

	// Quantize the difference into the 4-bit ADPCM code using the
	// the quantizer step size
	// Inverse quantize the ADPCM code into a predicted difference
	// using the quantizer step size
	diffq = step >> 3;
	if( diff >= step )
	{
		code |= 4;
		diff -= step;
		diffq += step;
	}
	step >>= 1;
	if( diff >= step )
	{
		code |= 2;
		diff -= step;
		diffq += step;
	}
	step >>= 1;
	if( diff >= step )
	{
		code |= 1;
		diffq += step;
	}

	// Fixed predictor computes new predicted sample by adding the
	// old predicted sample to predicted difference
	if( code & 8 )
		predsample -= diffq;
	else
		predsample += diffq;

	// Check for overflow of the new predicted sample
	if( predsample > 65535 )
		predsample = 65535;
	else if( predsample < 0 )
		predsample = 0;

	// Find new quantizer stepsize index by adding the old index
	// to a table lookup using the ADPCM code
	index += IndexTable[code];

	// Check for overflow of the new quantizer step size index
	if( index < 0 )
		index = 0;
	if( index > 88 )
		index = 88;

	// Save the predicted sample and quantizer step size index for
	// next iteration
	//state->prevsample = (unsigned short)predsample;
	//state->previndex = index;

	// Return the new ADPCM code
	return ( code & 0x0f );
}


