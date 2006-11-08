/****************************************/
/*		  	 SHORTINT.C            		*/
/*								 		*/
/* Half full interrupt service routine 	*/
/*		   CP2 Version					*/
/****************************************/

/*
CP2 version

REMOVED pulse pair, clutter filter, etc. 

Compiler Directives:
	Define	CF_ON to force filter ON
	Define 	DEBUG_INTOUT	to generate an integer output ramp
	Define	DEBUG_INTIN		to generate an integer input ramp
	Define	DEBUG_TS	to jam known integer data into TS
	Define	REVB		to accomodate data in lower 18 bits,
						 otherwise data assumed in upper 18 bits (REVD+).	
*/

#define		REVD
//#define		ANGLE_CORRECT		// Correct az/el for hardware fifo latency

// Several debugging tools
//#define	DEBUG_INTIN			// Plug integers in place of input FIFO
//#define	DEBUG_INTMID		// Stuff integers into input of pulsepair
//#define	DEBUG_INTOUT		// Plug integers into ABP output
//#define	DEBUG_TS			// Timeseries debug

#define PIRAQ3D_SCALE	1.0/IQSCALE
	
#define		NO_PMAC_DIRECT

// RapidDOW:
//#define		NUMVARS		3		// Number of variables (ABP)
//#define		NUMCHANS	2		// Number of channels
// CP2: 
#define		NUMVARS		2		// Number of variables (I,Q)
#define		NUMCHANS	1		// Number of channels
#define		CHIRP_GATES	0 		//
#define		OFFSETI		5
#define		OFFSETQ 	5
	
#define 	REVB_DATA	((*iptr++)<<14)	//<< for sign extend
#define		REVD_DATA	(*iptr++)

#include 	<std.h>
#include 	<sem.h>
#include 	<sys.h>
#include 	<csl_legacy.h>	//	gets IRQ_Disable() 
#include 	<csl_irq.h> 
#include	<math.h>                      
#include	"proto.h"                          
#include 	"single_p3iq_dcfg.h"
#include 	"local_defs.h"

//#define	IQSCALE	 7.629394531E-6
#define	IQSCALE	 	4.65661287E-10			// 1/2^31

#define	IQMASK	0x3FFFF
                       
#define	LOTHRESH	((double)16000 * 16000)
#define	SCALE		(1.0/28.0)
#define	HITHRESH	(LOTHRESH * (SCALE * SCALE))
#define EPSILON  	1000
#define	FIFO1I		(int *)0x1400204
#define	FIFO1Q		(int *)0x140020C
#define	FIFO2I		(int *)0x1400210
#define	FIFO2Q		(int *)0x1400208

static int Firstime=1;
static int iqOffsets=0;

extern	FIFO	*Fifo;
extern  unsigned int Period;
extern int Stgr, Cfltr;   
                                              
/* Added for RapiDOW */
static int Count=0,Val=0;

extern	PACKETHEADER *pkhtemp;
extern	PACKET	*CurPkt;
extern	float	*IQbuf;
extern	float	*LAGbuf;
extern	float	*ABPbuf;
extern 	float	*ABPstore;
extern  float	*SINstore;
extern	float	test_freq;	//	frequency of test sine wave 
extern	float	ioffset0, qoffset0, ioffset1, qoffset1, sumnorm;
extern 	int		samplectr, tsctr, hitnorm, HWfifo_latency;
extern 	int		Ramp[], testcount, ledflag, Maxgates, Stgr,Tsgate, Ntsgates;
extern 	float 	*CFptr, HWfifo_latency_ratio;
extern	unsigned long	pulse_num_low, pulse_num_high, beam_num_low, beam_num_high;
extern 	float IQoffset[4*NUMCHANS];
extern	unsigned int channelMode; // sets channelselect() processing mode
extern	unsigned int freqAdjust;  // sets test-data adjustment 
extern	unsigned int TestWaveformiMax;	//	index of complete wavelength
extern unsigned int amplAdjust;     // sets test-data amplitude adjustment 
extern	float TestWaveformAmplitude;	//	index of complete wavelength

extern	int		PCIHits, sbsram_hits;	// #hits combined for one PCI Bus transfer, #hits in SBSRAM
extern	PACKET	*NPkt;		// sbsram N-PACKET MEM_alloc w/1-channel data 

extern	int gates, bytespergate, hits, boardnumber;

int		testctr, localhits;
float	localnorm;
PACKET *testptr;

float	f_test;	// test frequency for injected sine wave

int		datainput(int Ngates, int *Dmabuf, float *Inbuf);
void 	sum_timeseries(int Ngates, float * restrict IQbuf, float *IQoff);
void	timeseries(float * restrict IQbuf, float *TSptr, int TSstart, int TSsize);
void	dma_Data(int Ngates, float *ABPin, float *ABPout);
/* End added for RapiDOW */

//int j = 0; // index for test timeseries data using DEBUG_TS over entire beam
int k = 0; 
int m = 0;
int first = 1;  

extern int burstready; 
extern void dma_pci(int tsize, unsigned int pci_dst);
//extern unsigned int * PMACDPRAMBaseAddress;
//extern unsigned int PMACDPRAMBaseData;	//	data at PMAC DPRAM Base Address
//
static	int	jTestWaveform = 0;	//	index into test waveform; persist hit-to-hit
static	float * fp_dbg_src = (float *)&SINstore;	//	pointer to test waveform

void int_half_full(void) {    
	volatile int temp;
	volatile unsigned int *dma_stat,*pci_cfg_ptr;
	unsigned int *led0,*led1,*dma_ptr,cfg_store0,cfg_store1;

	int		i,j;
	PACKET	*Host_src, *src, *dst;
	unsigned int * intsrc, * SBSRAMdst;	
	int 		*iptr;
	unsigned short *pmac_ptr;
	unsigned int PMAC_base;
	float	delta_az, delta_el, old_az, old_el, az, el;
	float	*TSptrBase;
	int 	* dbg_dst, * dbg_src;
	float	* fp_dbg_dst;	//, * fp_dbg_src; // floating-point src, dst for debug
	float x;

	int 		*fifo1I,*fifo1Q,*fifo2I,*fifo2Q;
	led0 = (unsigned int *)(0x1400308);  /* LED0 */
	led1 = (unsigned int *)(0x140030C);  /* LED1 */

	fifo1I   = FIFO1I;	//(int *)0x1400204;
	fifo1Q   = FIFO1Q;	//(int *)0x140020C;
	fifo2I   = FIFO2I;	//(int *)0x1400210;
	fifo2Q   = FIFO2Q;	//(int *)0x1400208;

	/* when this loop is entered, there is a hit worth of data in the hardware FIFO */

	/* put the header structure into the host memory FIFO */
	dst = (PACKET *)fifo_get_write_address(Fifo);
	src = (PACKET *)CurPkt; // 
	Host_src = (PACKET *)fifo_get_header_address(Fifo);
	testptr = src;

	// DEBUG The following will be removed when external 
	// controls are implemented

	hitnorm = 1.0/(float)src->data.info.hits;         

	localhits = hits;

#ifndef	DEBUG_INTIN
	localnorm = IQSCALE/sqrt((double)localhits);
#else
	localnorm = 1.0;
#endif  

	TSptrBase = ABPstore + Stgr*NUMVARS*NUMCHANS*(gates + CHIRP_GATES) + 2*NUMCHANS*hits*Ntsgates; //use 2nd timeseries buffer

	// Set up data ingest either by programmed transfer or by DMA. 
	/* Read FIFO 1 I */
	dma_ptr = (unsigned int *)0x1840058; /* DMA Channel 1 Destination Address */
	*dma_ptr = (unsigned int)hwData;
	dma_ptr = (unsigned int *)0x1840050; /* DMA Channel 1 Source Address */
	*dma_ptr = (unsigned int)fifo1I;
	dma_ptr = (unsigned int *)0x1840060; /* DMA Channel 1 Transfer Counter */
	*dma_ptr = gates & 0xFFFF;     
	dma_ptr = (unsigned int *)0x1840040;  /* channel 1 primary control */
	*dma_ptr = 0xC1;  /* start DMA Channel 1 */ 
	/* Loop while waiting for transfer to complete */
	/* This can be structured so that useful multi-tasking can be done! */

	dma_stat = (volatile unsigned int *)0x1840040;  /* channel 1 primary control */
	while((*dma_stat & 0xc) == 0x4)
		asm("	NOP");

	/* Read FIFO 1 Q */

	dma_ptr = (unsigned int *)0x1840058; /* DMA Channel 1 Destination Address */
	*dma_ptr = (unsigned int)(hwData + 0x1);
	dma_ptr = (unsigned int *)0x1840050; /* DMA Channel 1 Source Address */
	*dma_ptr = (unsigned int)fifo1Q;
	dma_ptr = (unsigned int *)0x1840060; /* DMA Channel 1 Transfer Counter */
	*dma_ptr = gates & 0xFFFF;     
	dma_ptr = (unsigned int *)0x1840040;  /* channel 1 primary control */
	*dma_ptr = 0xC1;  /* start DMA Channel 1 */ 

	/* Loop while waiting for transfer to complete */
	/* This can be structured so that useful multi-tasking can be done! */

	dma_stat = (volatile unsigned int *)0x1840040;  /* channel 1 primary control */
	while((*dma_stat & 0xc) == 0x4)
		asm("	NOP");
		
	/* Read FIFO 2 I */

	dma_ptr = (unsigned int *)0x1840058; /* DMA Channel 1 Destination Address */
	*dma_ptr = (unsigned int)(hwData + 0x2);
	dma_ptr = (unsigned int *)0x1840050; /* DMA Channel 1 Source Address */
	*dma_ptr = (unsigned int)fifo2I;
	dma_ptr = (unsigned int *)0x1840060; /* DMA Channel 1 Transfer Counter */
	*dma_ptr = gates & 0xFFFF;     
	dma_ptr = (unsigned int *)0x1840040;  /* channel 1 primary control */
	*dma_ptr = 0xC1;  /* start DMA Channel 1 */ 

	/* Loop while waiting for transfer to complete */
	/* This can be structured so that useful multi-tasking can be done! */

	dma_stat = (volatile unsigned int *)0x1840040;  /* channel 1 primary control */
	while((*dma_stat & 0xc) == 0x4)
		asm("	NOP");

	/* Read FIFO 2 Q */

	dma_ptr = (unsigned int *)0x1840058; /* DMA Channel 1 Destination Address */
	*dma_ptr = (unsigned int)(hwData + 0x3);
	dma_ptr = (unsigned int *)0x1840050; /* DMA Channel 1 Source Address */
	*dma_ptr = (unsigned int)fifo2Q;
	dma_ptr = (unsigned int *)0x1840060; /* DMA Channel 1 Transfer Counter */
	*dma_ptr = gates & 0xFFFF;     
	dma_ptr = (unsigned int *)0x1840040;  /* channel 1 primary control */
	*dma_ptr = 0xC1;  /* start DMA Channel 1 */ 

	/* Loop while waiting for transfer to complete */
	/* This can be structured so that useful multi-tasking can be done! */

	dma_stat = (volatile unsigned int *)0x1840040;  /* channel 1 primary control */
	while((*dma_stat & 0xc) == 0x4)
		asm("	NOP");

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
   	datainput(gates, hwData, (float *) hwData); 

	//  For first dwell sum timeseries to determine Piraq3 DC offset
	if(!iqOffsets) {	//	I,Q offsets not calcuated
	  	sum_timeseries(gates, (float *)hwData, IQoffset);
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
	channelselect(gates, (float *)hwData, (float *)CurPkt->data.data, channelMode); 
	// move CurPkt w/combined data from DSP-internal memory to NPkt in sbsram: 
	for (i = 0; i < sizeof(PACKET)/4; i++) { // move CurPkt contents to NPkt
		*SBSRAMdst++ = *intsrc++;	
	}
	if	(burstready && (sbsram_hits == 1*boardnumber))	{	//	complete Nhit packet in SBSRAM, 
															//	and board-specific stagger elapsed
		SEM_post(&burst_ready);	//	let 'er rip! 
	}
	fp_dbg_dst = (float *)((char *)&NPkt->data.data + (sbsram_hits * (HEADERSIZE + (gates * bytespergate))));	
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

int datainput(int ngates, int *ipt, float *iqpt) {   
	int 	i, temp;
	int 	*iptr;
   	int		i0i, q0i, i1i, q1i;
	float 	*iqptr;	
	
	// Set up pointers 
	iptr = ipt;			// Copy Input pointer
	iqptr = iqpt;		// Copy IQ pointer
	
#pragma MUST_ITERATE(20, ,2) 		
   	// Input all data as int's and convert to floats
   	for(i = 0; i < ngates; i++) {
#ifndef	DEBUG_INTIN
      	// Grab data into upper 18 bits
#ifdef REVB
      	i0i = REVB_DATA;
      	q0i = REVB_DATA;
      	i1i = REVB_DATA;
      	q1i = REVB_DATA;
      
#else // #ifdef REVD
      	i0i = REVD_DATA;
      	q0i = REVD_DATA;
      	i1i = REVD_DATA;
      	q1i = REVD_DATA;
#endif // REVB/REVD
#else	// for DEBUG_INTIN
		i0i = 4*i;
		q0i = 4*i+1;
		i1i = 4*i+2;
		q1i = 4*i+3;
#endif
	   	// Convert ints to floats and store
	  	*iqptr++ = (float)(i0i) - ioffset0; 
	  	*iqptr++ = (float)(q0i) - qoffset0; 
	  	*iqptr++ = (float)(i1i) - ioffset1; 
	  	*iqptr++ = (float)(q1i) - qoffset1;

	}
	temp = 1 ;
	return(temp);
}

void sum_timeseries(int ngates, float * restrict iqpt, float *IQoff) {

	int 	i;
	float 	* restrict iqptr;	
	
	iqptr = iqpt;		// Copy IQ pointer

#pragma MUST_ITERATE(20, ,2) 		
   	// Input all data as int's and convert to floats
   	for(i=0; i < ngates; i++) {
	   	//accumulate timeseries to determine DC offset of Piraq3
			
		IQoff[0] += *iqptr++;  //i0
		IQoff[1] += *iqptr++;  //q0
	  	IQoff[2] += *iqptr++;  //i1
	  	IQoff[3] += *iqptr++;  //q1
	}
}

void timeseries(float * restrict iqpt, float *tspt, int TSstart, int TSsize) {   
	int 	i;
   	float	debug_ts_base;
   	float   *tsptr;
	float 	*iqptr;	
	
	// Set up pointers 

	tsptr = tspt;						// Copy TS pointer
	iqptr = iqpt + TSstart*NUMCHANS*2;	// Copy IQ pointer

	// Set up debug parameter
	debug_ts_base = (float)((int)tspt << 8);

  	for(i=0; i < TSsize; i++) {
#ifndef	DEBUG_TS
	  	*tsptr++ = *iqptr++;
	  	*tsptr++ = *iqptr++;
	  	*tsptr++ = *iqptr++;
	  	*tsptr++ = *iqptr++;
#else
//	  	*tsptr++ = debug_ts_base + 4*i; 		// old Phoney data
//	  	*tsptr++ = debug_ts_base + 4*i+1;
//	  	*tsptr++ = debug_ts_base + 4*i+2;
//	  	*tsptr++ = debug_ts_base + 4*i+3;
	  	*tsptr++ = PIRAQ3D_SCALE*j+0.0; j++; 	// ramp Phoney data
	  	*tsptr++ = PIRAQ3D_SCALE*j+0.0; j++; 
	  	*tsptr++ = PIRAQ3D_SCALE*j+0.0; j++;
	  	*tsptr++ = PIRAQ3D_SCALE*j+0.0; j++;
//	  	*tsptr++ = *iqptr++;					// no phoney data: put ramp in source buffer
//	  	*tsptr++ = *iqptr++;
//	  	*tsptr++ = *iqptr++;
//	  	*tsptr++ = *iqptr++;
#endif 	// DEBUG_TS
  	}
}  
   
void dma_Data(int Ngates, float *ABPin, float *ABPout) {
	float *l_ABPin, *l_ABPout;
	unsigned int *dma_ptr;
	int i;
	volatile unsigned int *dma_stat;

	dma_ptr = (unsigned int *)0x1840010; /* DMA Channel 0 Source Address */
	*dma_ptr = (unsigned int)ABPin;	
	dma_ptr = (unsigned int *)0x1840018; /* DMA Channel 0 Destination Address */
	*dma_ptr = (unsigned int)ABPout;		
	dma_ptr = (unsigned int *)0x1840020; /* DMA Channel 0 Transfer Counter */
	*dma_ptr = Ngates;         
	dma_ptr = (unsigned int *)0x1840000;  /* channel 0 primary control */
	*dma_ptr = 0x01000051;  /* start DMA Channel 0 */  

/* Loop while waiting for transfer to complete */
/* This can be structured so that useful multi-tasking can be done! */

	dma_stat = (volatile unsigned int *)0x1840000;  /* channel 0 primary control */
	while((*dma_stat & 0xc) == 0x4)
		asm("	NOP");
}



