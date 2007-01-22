//////////////////////////////////////////////////
//	  	A2DFifoISR.C  
//								 		
// A2D Fifo Half full interrupt service routine
//
//////////////////////////////////////////////////

#define		NUMVARS		2		// Number of variables (I,Q)
#define		NUMCHANS	1		// Number of channels
 
#include 	<std.h>
#include 	<sem.h>
#include 	<sys.h>
#include 	<csl_legacy.h>	//	gets IRQ_Disable() 
#include 	<csl_irq.h> 
#include	<math.h>                      
#include	"piraqComm.h"                          
#include 	"CP2Piraqcfg.h"
#include 	"local_defs.h"

#define	FIFO1I		(int *)0x1400204
#define	FIFO1Q		(int *)0x140020C
#define	FIFO2I		(int *)0x1400210
#define	FIFO2Q		(int *)0x1400208

///////////////////////////////////////////////////////////
extern CircularBuffer*   Fifo;
extern PPACKET* CurPkt;
extern PPACKET* sbsRamBuffer;		// sbsram N-PACKET MEM_alloc w/1-channel data 
extern float   ioffset0; 
extern float   qoffset0; 
extern float   ioffset1; 
extern float   qoffset1; 
extern float   sumnorm;
extern int     led0flag;
extern int*    a2dFifoBuffer;			// receives the I/Q data from the A2D fifos.
extern unsigned long pulse_num_low;
extern unsigned long pulse_num_high;
extern RCVRTYPE rcvrType;
extern horiz;
extern 	float        IQoffset[4*NUMCHANS];

extern	unsigned int channelMode; // sets channelselect() processing mode

extern int nPacketsPerBlock;             // #hits combined for one PCI Bus transfer
extern int sbsram_hits;	        // #hits in SBSRAM
extern int gates;
extern int bytespergate;
extern int hits;
extern int boardnumber;
extern int burstready; 
extern unsigned int* pLed0;  /* LED0 */  
extern unsigned int* pLed1;  /* LED1 */

extern unsigned int* pPMACdpram;

///////////////////////////////////////////////////////////
int		toFloats(int Ngates, int *pIn, float *pOut);
void 	sumTimeseries(int Ngates, float * restrict pIn, float *pOut);
void    setPulseNumber(PPACKET* pPkt);
void    readPMAC(PPACKET* pPkt);

///////////////////////////////////////////////////////////

void A2DFifoISR(void) {    
	volatile int temp;
	int iqOffsets=0;
	int* sbsRamDst;	
	int* fifo1I;
	int* fifo1Q;
	int* fifo2I;
	int *fifo2Q;	

	fifo1I   = FIFO1I;	//(int *)0x1400204;
	fifo1Q   = FIFO1Q;	//(int *)0x140020C;
	fifo2I   = FIFO2I;	//(int *)0x1400210;
	fifo2Q   = FIFO2Q;	//(int *)0x1400208;

	// when this ISR is entered, the hardware fifo is at half full or greater. 
	// read a pulse worth (number of gates) from each of the four fifos.

	/* put the header structure into the host memory FIFO */

	// even though fifo_get_write_address(Fifo) doesn't do anything,
	// if we do not  make the following call, the data shows a low frequency
	// variation in the power spectrum. Very strange - this really needs to be sorted out.
	(PPACKET *)cb_get_write_address(Fifo);

	/* Read FIFO 1 I */
	dmaTransfer(1, fifo1I, a2dFifoBuffer, gates, 0); 

	/* Read FIFO 1 Q */
	dmaTransfer(1, fifo1Q, a2dFifoBuffer+1, gates, 0); 

	/* Read FIFO 2 I */
	dmaTransfer(1, fifo2I, a2dFifoBuffer+2, gates, 0); 

	/* Read FIFO 2 Q */
	dmaTransfer(1, fifo2Q, a2dFifoBuffer+3, gates, 0); 

	/* Read EOF's from each FIFO */
	temp  = *(volatile int *)fifo2I;
	temp |= *(volatile int *)fifo2Q;
	temp |= *(volatile int *)fifo1Q;
	temp |= *(volatile int *)fifo1I;

	CurPkt->info.status = 0;
	// Set the EOF flag
	if(temp & 0x3C000) {  /* if any of the lower 4 bits of the EOF are high */
		*pLed1 = 0; /* turn on the EOF fault LED */
		CurPkt->info.status |= FIFO_EOF;  // Tell PC Host got EOF!
	}
	else {
		*pLed1 = 1; /* Turn off the LED */
	}
	// set the HV flag
	CurPkt->info.horiz = horiz;
	if (rcvrType == SHV) {
		// for SBand, polarizations alternate.
		horiz = !horiz;	
	}	

	/* Convert I,Q integers to floats in-place */
	toFloats(gates, a2dFifoBuffer, (float *) a2dFifoBuffer); 

	//  For first dwell sum timeseries to determine Piraq3 DC offset
	if(!iqOffsets) {	//	I,Q offsets not calcuated
		sumTimeseries(gates, (float *)a2dFifoBuffer, IQoffset);
		ioffset0 = IQoffset[0]*sumnorm;	//	CP2: normalize to #gates only
		qoffset0 = IQoffset[1]*sumnorm;
		ioffset1 = IQoffset[2]*sumnorm;
		qoffset1 = IQoffset[3]*sumnorm;
		iqOffsets = 1;	//	now they are
	}
	// inject test data after channel-select
#ifdef	CP2_TEST_RAMP		// CP2 test linear ramp along pulse in timeseries data	
	// write an integer ramp across raw data -- see what happens in the host. 
	fp_dbg_dst = (float *)CurPkt->data.data;
	for (i = 0, j = 0; i < gates; i++) { // all data 2 channels: add offset to distinguish data source
		*fp_dbg_dst++ = (float)4194304*j+src->info.channel; j++; 	
		*fp_dbg_dst++ = (float)4194304*j+src->info.channel; j++;
	}
#endif
#ifdef	CP2_TEST_SINUSOID		// CP2 test sine wave along pulse in timeseries data
	fp_dbg_dst = (float *)CurPkt->data.data;
	for(i = 0; i < gates; i++, jTestWaveform++) {
		*fp_dbg_dst++ = *fp_dbg_src++;
		*fp_dbg_dst++ = *fp_dbg_src++;
		if (jTestWaveform >= TestWaveformiMax)	{	//	one complete waveform
			fp_dbg_src = SINstore;		//	start over
			jTestWaveform = 0;
		}
	}
#endif

	setPulseNumber(CurPkt);

	// process 2-channel hwData into 1-channel data: 
	// channel-select, gate by gate. data destination CurPkt->data.data
	ChannelSelect(gates, (float *)a2dFifoBuffer, (float *)CurPkt->data, channelMode); 

	// read PMAC and save to the pulse
	readPMAC(CurPkt);

	// move CurPkt w/combined data from DSP-internal memory to sbsRamBuffer in sbsram: 
	sbsRamDst = (int *)((char *)sbsRamBuffer 
		+ (sbsram_hits * (sizeof(PINFOHEADER) + (gates * bytespergate))));

	dmaTransfer(0, 
		(int*)CurPkt, 
		sbsRamDst,  
		(sizeof(PINFOHEADER) + (gates * bytespergate)), 
		0); 

	if	(burstready && (sbsram_hits == 2*boardnumber))	{	
		//	complete Nhit packet in burst fifo, 
		// and board-specific stagger elapsed
		// enable dma transfer from burst fifo to pci bus.
		SEM_post(&StartPciTransfer);
	}
	sbsram_hits++; // hits in sbsram

	//	nPacketsPerBlock packet accumulated
	if	(sbsram_hits == nPacketsPerBlock)	{
		// Toggle the blinkin' LED
		*pLed0 = led0flag ^= 1;
		//	DMA from SBSRAM to PCI Burst FIFO
		SEM_post(&FillBurstFifo);			
	}
}

///////////////////////////////////////////////////////////

int toFloats(int ngates, int *pIn, float *pOut) {   
	int 	i, temp;
	int 	*iptr;
	int		i0i, q0i, i1i, q1i;
	float 	*iqptr;	

	// Set up pointers 
	iptr = pIn;			// Copy Input pointer
	iqptr = pOut;		// Copy IQ pointer

#pragma MUST_ITERATE(20, ,2) 		
	// Input all data as int's and convert to floats
		for(i = 0; i < ngates; i++) {

		// Grab data into upper 18 bits
		i0i = (*iptr++);
		q0i = (*iptr++);
		i1i = (*iptr++);
		q1i = (*iptr++);

		// Convert ints to floats and store
		*iqptr++ = (float)(i0i) - ioffset0; 
		*iqptr++ = (float)(q0i) - qoffset0; 
		*iqptr++ = (float)(i1i) - ioffset1; 
		*iqptr++ = (float)(q1i) - qoffset1;

	}
	temp = 1 ;
	return(temp);
}

///////////////////////////////////////////////////////////

void sumTimeseries(int ngates, float * restrict pIn, float *pOut) {

	int 	i;
	float 	* restrict iqptr;	

	iqptr = pIn;		// Copy IQ pointer

#pragma MUST_ITERATE(20, ,2) 		
	// Input all data as int's and convert to floats
		for(i=0; i < ngates; i++) {
		//accumulate timeseries to determine DC offset of Piraq3

		pOut[0] += *iqptr++;  //i0
		pOut[1] += *iqptr++;  //q0
		pOut[2] += *iqptr++;  //i1
		pOut[3] += *iqptr++;  //q1
	}
}

///////////////////////////////////////////////////////////

void    setPulseNumber(PPACKET* pPkt)
{

	/* keep track of the 64 bit pulsenumber: works here -- commented out in int_half_full() */
	if(!(++pulse_num_low))
		pulse_num_high++;
	pPkt->info.pulse_num_low = pulse_num_low;
	pPkt->info.pulse_num_high = pulse_num_high;
}

///////////////////////////////////////////////////////////
void
readPMAC(PPACKET* pPkt)
{
	volatile unsigned int  dmrr;
	volatile unsigned int  dmpbam;

	// if the PMAC doesn't exist, as indicated by
	// a zero base address, do nothing
	if (!pPMACdpram)
		return;

	// PCI physical address of PMAC dpram.
	// wading through some old PMAC code,
	// it looks like the offsets to parameters into
	// the dpram are:
	//  0 - az
	//  2 - el
	//  4 - scan type
	//  6 - sweep
	//  8 - volume
	// 10 - size
	// 12 - transition

	// set PLX chip to configuration speed
	WriteCE1(PLX_CFG_MODE);

	// save DMRR
	dmrr = *(volatile unsigned int *)0x140009C;

	// save DMPBAM
	dmpbam = *(volatile unsigned int *)0x14000A8; 

	// set DMRR for the size of the PMAC dpram
	*(volatile unsigned int *)0x140009C = 0xffff0000; 				

	// set DMPBAM to access the PMAC dpram
	*(volatile unsigned int *)0x14000A8 = (unsigned int)pPMACdpram + (dmpbam & 0xFFFF);  

	// read the PMAC locations.
	pPkt->info.antAz    = *(unsigned short*)((unsigned char*)PCIBASE + 0 );
	pPkt->info.antEl    = *(unsigned short*)((unsigned char*)PCIBASE + 2 );
	pPkt->info.scanType = *(unsigned short*)((unsigned char*)PCIBASE + 4 );
	pPkt->info.sweepNum = *(unsigned short*)((unsigned char*)PCIBASE + 6 );
	pPkt->info.volNum   = *(unsigned short*)((unsigned char*)PCIBASE + 8 );
	pPkt->info.antSize  = *(unsigned short*)((unsigned char*)PCIBASE + 10);
	pPkt->info.antTrans = *(unsigned short*)((unsigned char*)PCIBASE + 12);

	// restore DMRR
	*(volatile unsigned int *)0x140009C = dmrr;

	// restore DMPBAM
	*(volatile unsigned int *)0x14000A8 = dmpbam;

	// restore PLX chip to high speed
	WriteCE1(HIGH_SPEED_MODE);


}
