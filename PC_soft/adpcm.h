/********************************************************************
* FileName:		ADPCM.H
* Dependencies: none
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
* Header file for ADPCM.C
*
* Change History:
* $Log$
* v1.00 - first released version of APDCM on PIC18
********************************************************************/

/* Function prototype for the ADPCM Encoder routine */
unsigned char ADPCMEncoder(unsigned short);

/* Function prototype for the ADPCM Decoder routine */
unsigned short ADPCMDecoder(unsigned char);

