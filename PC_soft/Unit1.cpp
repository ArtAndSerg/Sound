//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
                     
#include "Unit1.h"
#include "math.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

TForm1 *Form1;
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------


/* Table of index changes */
const unsigned char IndexTable[16] = {
 0xff, 0xff, 0xff, 0xff, 2, 4, 6, 8,
 0xff, 0xff, 0xff, 0xff, 2, 4, 6, 8
};
/* Quantizer step size lookup table */
const int StepSizeTable[89] = {
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
signed short ADPCMDecoder(unsigned char code)
{
   static int step;		/* Quantizer step size */
   static int predsample;		/* Output of ADPCM predictor */
   static int diffq;		/* Dequantized predicted difference */
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
      diffq += (step >> 1);
   if( code & 1 )
      diffq += (step >> 2);

   /* Add the difference to the predicted sample */
   if( code & 8 )
      predsample -= diffq;
   else
      predsample += diffq;

   /* Check for overflow of the new predicted sample */
   if(predsample > 32767)
 predsample = 32767;
else if(predsample < -32768)
 predsample = -32768;

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
unsigned char ADPCMEncoder( signed short sample)
{
   static int step;		/* Quantizer step size */
   static int predsample = 0;		/* Output of ADPCM predictor */
   static int diffq, diff;		/* Dequantized predicted difference */
   static char index, code;		/* Index into step size table */

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
	if(predsample > 32767)
 predsample = 32767;
else if(predsample < -32768)
 predsample = -32768;

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

void __fastcall TForm1::Button1Click(TObject *Sender)
{
   unsigned char *res = new unsigned char[1000000];
   int f = FileOpen("lenin.wav", fmOpenRead);
   signed short tmp;
   double d;
   int i = 0, n =0, j = 0;
   Series1->Clear();
   Series2->Clear();
   FastLineSeries1->Clear();
   FileSeek(f, 200, 0);
   for (i = 0; i < 10000; i++) {
     FileRead(f, (void*)&tmp, 2);
     //tmp = sin((double)i / 100.) * 32000;
     if (!(i & 0x01)) {
        res[n] = ADPCMEncoder(tmp) << 4;

     }
     else {
        res[n] |= (ADPCMEncoder(tmp) );
        n++;
     }
     tmp = ((signed short)tmp)/64 + 512;
     d =  tmp;
     Series1->Add(d);
   }
   FileClose(f);

   for (i = 0; i < n; i++) {
       tmp = ADPCMDecoder((res[i] >> 4) & 0x0F);
       tmp = ((signed short)tmp)/64 + 512;
       d =  tmp;
       Series2->Add(d);
       if (i>10) FastLineSeries1->Add(Series1->YValues->Value[j] - Series2->YValues->Value[j]);
       j++;

       tmp = ADPCMDecoder(res[i] & 0x0F);
       tmp = ((signed short)tmp)/64 + 512;
       d =  tmp;
       Series2->Add(d);
       if (i>10) FastLineSeries1->Add(Series1->YValues->Value[j] - Series2->YValues->Value[j]);
       j++;

   }
   delete res;
}
//---------------------------------------------------------------------------



void __fastcall TForm1::Go1Click(TObject *Sender)
{
   static int offset = 200;
   unsigned char *res = new unsigned char[1000000];
   int f = FileOpen("lenin.wav", fmOpenRead);
   signed short tmp;
   double d;
   int i = 0, n =0, j = 0;
   Series1->Clear();
   Series2->Clear();
   FastLineSeries1->Clear();
   FileSeek(f, offset, 0);
   offset += 10000;
   for (i = 0; i < 10000; i++) {
     FileRead(f, (void*)&tmp, 2);
     //tmp = sin((double)i / 100.) * 32000;
     if (!(i & 0x01)) {
        res[n] = ADPCMEncoder(tmp) << 4;

     }
     else {
        res[n] |= (ADPCMEncoder(tmp) );
        n++;
     }
     tmp = ((signed short)tmp)/64 + 512;
     d =  tmp;
     Series1->Add(d);
   }
   FileClose(f);

   for (i = 0; i < n; i++) {
       tmp = ADPCMDecoder((res[i] >> 4) & 0x0F);
       tmp = ((signed short)tmp)/64 + 512;
       d =  tmp;
       Series2->Add(d);
       if (i>10) FastLineSeries1->Add(Series1->YValues->Value[j] - Series2->YValues->Value[j]);
       j++;

       tmp = ADPCMDecoder(res[i] & 0x0F);
       tmp = ((signed short)tmp)/64 + 512;
       d =  tmp;
       Series2->Add(d);
       if (i>10) FastLineSeries1->Add(Series1->YValues->Value[j] - Series2->YValues->Value[j]);
       j++;

   }
   delete res;
}
//---------------------------------------------------------------------------

