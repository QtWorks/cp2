/****************************************/
/*		  	A2DFifoISR.C            		*/
/*								 		*/
/* Half full interrupt service routine 	*/
/*		   CP2 Version					*/
/****************************************/

/*
Compiler Directives:
	Define	DEBUG_TS	to jam known integer data into TS
*/

//#define	DEBUG_TS			// Timeseries debug

#define PIRAQ3D_SCALE	1.0/IQSCALE
	
#define		NO_PMAC_DIRECT

// CP2: 
#define		NUMVARS		2		// Number of variables (I,Q)
#define		NUMCHANS	1		// Number of channels
#define		CHIRP_GATES	0 		//
#define		OFFSETI		5
#define		OFFSETQ 	5
	
#include 	<std.h>
#include 	<sem.h>
#include 	<sys.h>
#include 	<csl_legacy.h>	//	gets IRQ_Disable() 
#include 	<csl_irq.h> 
#include	<math.h>                      
#include	"proto.h"                          
#include 	"CP2Piraqcfg.h"
#include 	"local_defs.h"

#define	IQSCALE	 	4.65661287E-10			// 1/2^31
#define	IQMASK	0x3FFFF
                       
#define	FIFO1I		(int *)0x1400204
#define	FIFO1Q		(int *)0x140020C
#define	FIFO2I		(int *)0x1400210
#define	FIFO2Q		(int *)0x1400208

///////////////////////////////////////////////////////////

static int iqOffsets=0;

extern	FIFO*        Fifo;
extern	PACKETHEADER* pkhtemp;
extern	PACKET*      CurPkt;
extern	float        ioffset0; 
extern	float        qoffset0; 
extern	float        ioffset1; 
extern	float        qoffset1; 
extern	float        sumnorm;
extern 	int          samplectr;
extern 	int          hitnorm; 
extern 	int          ledflag;
extern 	int	         Stgr;
extern 	int	         Ntsgates;
extern	unsigned long	pulse_num_low, pulse_num_high, beam_num_low, beam_num_high;
extern 	float        IQoffset[4*NUMCHANS];
extern	unsigned int channelMode; // sets channelselect() processing mode
extern	unsigned int freqAdjust;  // sets test-data adjustment 
extern	unsigned int TestWaveformiMax;	//	index of complete wavelength
extern  unsigned int amplAdjust;     // sets test-data amplitude adjustment 
extern	float        TestWaveformAmplitude;	//	index of complete wavelength

extern	int		     PCIHits, sbsram_hits;	// #hits combined for one PCI Bus transfer, #hits in SBSRAM
extern	PACKET*      NPkt;		// sbsram N-PACKET MEM_alloc w/1-channel data 

extern	int gates, bytespergate, hits, boardnumber;

int		testctr, localhits;
float	localnorm;
PACKET *testptr;

float	f_test;	// test frequency for injected sine wave

int		toFloats(int Ngates, int *pIn, float *pOut);
void 	sumTimeseries(int Ngates, float * restrict pIn, float *pOut);
int k = 0; 
int m = 0;
int first = 1;  

extern int burstready; 
extern void dma_pci(int tsize, unsigned int pci_dst);
///////////////////////////////////////////////////////////

void A2DFifoISR(void) {    
	volatile int temp;
	volatile unsigned int *pci_cfg_ptr;
	unsigned int *led0;
	unsigned int *led1;

	int		i,j;
	PACKET* src;
	PACKET* dst;
	unsigned int * intsrc, * SBSRAMdst;	
	int 		*fifo1I,*fifo1Q,*fifo2I,*fifo2Q;
	led0 = (unsigned int *)(0x1400308);  /* LED0 */
	led1 = (unsigned int *)(0x140030C);  /* LED1 */

	fifo1I   = FIFO1I;	//(int *)0x1400204;
	fifo1Q   = FIFO1Q;	//(int *)0x140020C;
	fifo2I   = FIFO2I;	//(int *)0x1400210;
	fifo2Q   = FIFO2Q;	//(int *)0x1400208;

	/* when this loop is entered, there is a hit worth of data in the hardware FIFO */

	/* put the header structure into the host memory FIFO */

	// Even though dst is never referenced, if we do not
	// make the following call, the data shows a low frequency
	// variation. Very strange - this really needs to be sorted out.
	dst = (PACKET *)fifo_get_write_address(Fifo);
	src = (PACKET *)CurPkt; // 
	testptr = src;

	// DEBUG The following will be removed when external 
	// controls are implemented

	hitnorm = 1.0/(float)src->data.info.hits;         

	localhits = hits;

	localnorm = IQSCALE/sqrt((double)localhits);

	/* Read FIFO 1 I */
	dmaTransfer(1, 0xC1, fifo1I, a2dFifoBuffer, gates, 0); 

	/* Read FIFO 1 Q */
	dmaTransfer(1, 0xC1, fifo1Q, a2dFifoBuffer+1, gates, 0); 
		
	/* Read FIFO 2 I */
	dmaTransfer(1, 0xC1, fifo2I, a2dFifoBuffer+2, gates, 0); 

	/* Read FIFO 2 Q */
	dmaTransfer(1, 0xC1, fifo2Q, a2dFifoBuffer+3, gates, 0); 

    /* Read EOF's from each FIFO */
   	temp  = *(volatile int *)fifo2I;
   	temp |= *(volatile int *)fifo2Q;
   	temp |= *(volatile int *)fifo1Q;
   	temp |= *(volatile int *)fifo1I;

   	if(temp & 0x3C000) {  /* if any of the lower 4 bits of the EOF are high */
		*led1 = 0; /* turn on the EOF fault LED */
		src->data.info.packetflag = -1;  // Tell PC Host got EOF!
	}
	else {
		*led1 = 1; /* Turn off the LED */
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
		*fp_dbg_dst++ = (float)4194304*j+src->data.info.channel; j++; 	
		*fp_dbg_dst++ = (float)4194304*j+src->data.info.channel; j++;
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

	intsrc = (unsigned int *)CurPkt;
	SBSRAMdst = (unsigned int *)((char *)NPkt + (sbsram_hits * (HEADERSIZE + (gates * bytespergate))));	// add nth hit offset 

	/* keep track of the 64 bit pulsenumber: works here -- commented out in int_half_full() */
	if(!(++pulse_num_low))
		pulse_num_high++;
	CurPkt->data.info.pulse_num_low = pulse_num_low;
	CurPkt->data.info.pulse_num_high = pulse_num_high;

	// process 2-channel hwData into 1-channel data: channel-select, gate by gate. data destination CurPkt->data.data
	ChannelSelect(gates, (float *)a2dFifoBuffer, (float *)CurPkt->data.data, channelMode); 
	// move CurPkt w/combined data from DSP-internal memory to NPkt in sbsram: 
	for (i = 0; i < sizeof(PACKET)/4; i++) { // move CurPkt contents to NPkt
		*SBSRAMdst++ = *intsrc++;	
	}
	if	(burstready && (sbsram_hits == 1*boardnumber))	{	//	complete Nhit packet in SBSRAM, 
															//	and board-specific stagger elapsed
		SEM_post(&burst_ready);	//	let 'er rip! 
	}
	sbsram_hits++; // hits in sbsram

   	if(++samplectr >= hits) { // beam done
		/* Update the beam number */
		if(!(++beam_num_low))
			beam_num_high++;
		src->data.info.beam_num_low = beam_num_low;
		src->data.info.beam_num_high = beam_num_high;
		   
		*led0 = ledflag ^= 1;	// Toggle the blinkin' LED
	
		samplectr = 0;		// Zero the sample counter
	}
	if	(sbsram_hits == PCIHits)	{	//	Nhit packet accumulated
		SEM_post(&data_ready);			//	DMA from SBSRAM to PCI Burst FIFO
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
