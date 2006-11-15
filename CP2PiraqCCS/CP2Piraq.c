/*****************************/ 
/*        CP2_D.C            */
/*                           */
/* PIRAQ III Timeseries I,Q  */
/*    Code with DMA          */
/*****************************/

#include <csl_legacy.h>
#include <csl.h>
#include <csl_irq.h>
#include <std.h>
#include <sem.h>
#include <sys.h>
#include <mem.h>
#include <math.h>

#define		NUMVARS			3		// Number of variables (ABP)
#define		NUMCHANS		2		// Number of channels
#define		CHIRP_GATES		0 		//12 -- This will mess up malloc's if other than 0!!!

#define		FIFOCLR	 		0x1400200;
#define		FIFO1			0x1400204;
#define		FIFO2			0x1400208;
#define		FIFO3			0x140020C;
#define		FIFO4			0x1400210;

#define		FIFOLEN			16384	// Length of input hardware FIFO
#define		TSMAX			10		// Maximum Number of Timeseries gates
#define 	P3_SCOPE

#include 	"proto.h"
#include 	"CP2Piraqcfg.h"
#include 	"local_defs.h"
#include	"coeff.h"


#define 	PCI_FIFO_WR 	0x1400310
#define		PCI_FIFO_RST 	0x1400314
#define 	PCI_FIFO_RD 	0x1C00
#define 	PCI_LOCAL_BASE 	0x3000000
#define 	MAX_BUFFER_SZ 	0x400000
#define 	TIMER_CLOCK 2	5.e-09
//#define		PI				3.1415926

float IQoffset[4*NUMCHANS];	//	CP2: compute offsets for both CHA and CHB
//int Stgr;
int Cfltr;
int Maxgates;
unsigned int *Led_ptr,DMA_base, Period;
void delay(void);
void initDsp(void);
void dma_pci(int tsize, unsigned int pci_dst);
void dma_fifo(int tsize, unsigned int source);
void pci_burst_xfer();
int burstready = 0;

extern far int SCRATCH_RAM;
extern int m;

FIFO  	*Fifo;
PACKET	*CurPkt;	// DSP-internal PACKET malloc w/1-channel data: contains current header information
PACKET	*NPkt;		// sbsram N-PACKET MEM_alloc w/1-channel data 
float	*LAGbuf;	// pointer to lag
float 	*ABPbuf;	// pointer to abp averaging buffer
float	*ABPstore;	// pointer to abp double buffer
float	*SINstore;  // pointer to fake sinusoidal input data
float	test_freq; 
float 	ioffset0, qoffset0, ioffset1, qoffset1;	// DC offsets
int		samplectr, tsctr;	// sample counter, time series ctr
int	 	testcount, ledflag;
int 	*fifo1,*fifo2,*fifo3,*fifo4,*fifoclr, HWfifo_latency;
int		Tsgate,Ntsgates;
int		PCIHits, sbsram_hits;	// #hits combined for one PCI Bus transfer, #hits in SBSRAM
float 	*CFptr, sumnorm;
float    hitnorm;
float	HWfifo_latency_ratio;
unsigned long	pulse_num_low, pulse_num_high, beam_num_low, beam_num_high;
unsigned int channelMode; 	// sets channelselect() processing mode 
unsigned int freqAdjust;	// sets test-data frequency adjustment 
unsigned int TestWaveformiMax;	//	index of complete wavelength
unsigned int amplAdjust;	// sets test-data amplitude adjustment 
float		 TestWaveformAmplitude = 1.0; 

int gates, bytespergate, hits, boardnumber;

void main()
{ 
	/* Initialize global variable(s) */
	CSL_Init();  /* initialize the TI chip support library */
}

void initTask(void)
{
	unsigned int 	*led1_ptr,*dma_ptr,*burst_fifo; 
	volatile unsigned int *pci_cfg_ptr;
	int 			i;
	volatile int 	ii,j;
	int 			Count;
	int 			sbsram_seg;
	PACKETHEADER	*pkt;
	unsigned int	*src, *dst;
	PACKET			*ldst;

/* Initialize Essential DSP Control Registers */
/* setup EMIF Global and Local Control Registers */
/* this should allow external memory to be used! */

	initDsp(); 

	ledflag = 1;
	
	/* Clear the LEDs */

	Led_ptr = (unsigned int *)(0x1400308);  /* LED0 */  
	led1_ptr = (unsigned int *)(0x140030C);  /* LED1 */
	*Led_ptr = ledflag;
	*led1_ptr = 0x1;

	/* set up SDRAM, SBSRAM segments */

    sbsram_seg = MEM_define((Ptr) 0x400000, 0x40000, 0);
	sbsram_hits = 0; 

	// CP2 PCI Bus transfer size:
	// Nhits * (HEADERSIZE + (config->gatesa * bytespergate))
	
	samplectr = 0;	// Initialize sample counter to 0

	// Zero IQ offset array

	IQoffset[0] = 0.0;
	IQoffset[1] = 0.0;
	IQoffset[2] = 0.0;
	IQoffset[3] = 0.0;
	
	ioffset0 = 0.0;
	qoffset0 = 0.0;
	ioffset1 = 0.0;
	qoffset1 = 0.0;
	
	fifoclr = (int *)FIFOCLR;
	fifo1   = (int *)FIFO1;
	fifo2   = (int *)FIFO2;
	fifo3   = (int *)FIFO3;
	fifo4   = (int *)FIFO4;

	Fifo = fifo_open("PRQDATA");  /* the argument is not used */

	/* Transfer packet header to current buffer */
    src = (unsigned int *)fifo_get_header_address(Fifo);
    pkt = (PACKETHEADER *)src; 
	gates = pkt->info.gates; 
	hits = pkt->info.hits; 
	bytespergate = pkt->info.bytespergate;
	boardnumber =  pkt->info.channel;

	//	compute #hits to combine per PCI transfer:
	PCIHits = (unsigned int)UDPSENDSIZE / (HEADERSIZE + (gates * bytespergate)); 
	if	(PCIHits % 2)	//	odd #hits computed
		PCIHits--;		//	make it even
	// allocate a complete 1-channel PACKET; it contains current pulse, 
	// header plus data, post channel-select
	CurPkt = (PACKET *)malloc((HEADERSIZE + (gates * bytespergate)));
	
	// method using DSP-internal PACKET for header parameter maintenance
	dst = (unsigned int *)CurPkt;	
		 
	// /4: move header as integers
	for(i = 0; i < HEADERSIZE/4; i++)	 
		*dst++ = *src++;

	ldst = (PACKET *)CurPkt;	// new method

	// NPkt PACKET pointer to one hit in sbsram N-hit alloc
	// CP2 PCI Bus transfer size: Nhits * (HEADERSIZE + (config->gatesa * bytespergate) 

	// PACKET pointer to one hit in sbsram N-hit alloc
	NPkt = (PACKET *)MEM_alloc(sbsram_seg, 
				PCIHits * (HEADERSIZE + (gates * bytespergate)),
				0); 

	// hwData DSP-internal 2-channel hwFIFO data array, interleaved I1, Q1, I2, Q2
  	a2dFifoBuffer = (int *)malloc((gates * 2 * bytespergate));

	/* zero single-pulse buffer */
	memset(a2dFifoBuffer,0,(gates * 2 * bytespergate));	

	WriteCE1(PLX_CFG_MODE);

    // Timeseries initialization
	
	Tsgate = ldst->data.info.ts_start_gate;
	Ntsgates = (ldst->data.info.ts_end_gate - Tsgate) + 1; 
	/* don't exceed maximum number of timeseries */
	if(Ntsgates > TSMAX)  
		Ntsgates = TSMAX;
   	  
	if(ldst->data.info.dataformat == 16) {
		// if simplepp & clutterfilter
		Maxgates = 400;  	
		Stgr = 1;
		Cfltr = 1;
	}
	else  {
	    /* (ldst->data.info.dataformat == 17) */
		Maxgates = 600;			// else staggered PRT
		Stgr = 2;
		Cfltr = 0;
	}
	
#ifdef	CP2_TEST_SINUSOID		// CP2 test sine wave ... along pulse in timeseries data
	SINstore = (float *)MEM_alloc(sdram0_seg,2000000 * sizeof (float),0);	//	allow up to 2000000 data
	test_freq = 0.002;	//	should be equivalent to 60002000.0Hz
	createSineTestWaveform(test_freq); 
#endif

	/* Initialize pulse and beam counters */
	pulse_num_low = ldst->data.info.pulse_num_low;
	pulse_num_high = ldst->data.info.pulse_num_high;
	beam_num_high = ldst->data.info.beam_num_high;
	beam_num_low = ldst->data.info.beam_num_low;
		
	/* Calculate the number of hits latency we're stuck with */
	HWfifo_latency = (int)(0.5 + 0.5*(float)FIFOLEN/(float)ldst->data.info.gates);
	HWfifo_latency_ratio = (float)HWfifo_latency/(float)ldst->data.info.hits;

	//	Calculate DC offset normalization
	// CP2 nix hits in normalization.
	sumnorm = 1.0/((float)ldst->data.info.gates);  

	/* Initialize DMA Channel 0 for ABP transfers */
	/* channel 0 primary control */
	dma_ptr = (unsigned int *)0x1840000;  
	*dma_ptr = 0x01000050; 
	/* channel 0 secondary control */
	dma_ptr = (unsigned int *)0x1840008;  
	*dma_ptr = 0x90; 

	/* Initialize DMA Channel 1 for A/D FIFO transfers */

	dma_ptr = (unsigned int *)0x1840040;  /* channel 1 primary control */
	*dma_ptr = 0xc0; 
	dma_ptr = (unsigned int *)0x1840048;  /* channel 1 secondary control */
	*dma_ptr = 0x90; 
	dma_ptr = (unsigned int *)0x1840030; /* DMA Global Index Register A */
	*dma_ptr = 0x10;

	/* Clear the PCI FIFO */

	burst_fifo = (unsigned int *)PCI_FIFO_RST;
	*burst_fifo = 0x0;

	/* Initialize DMA Channel 2 for PCI FIFO transfers */

	dma_ptr = (unsigned int *)0x1840004;  /* channel 2 primary control */
	*dma_ptr = 0x10; 
	dma_ptr = (unsigned int *)0x184000C;  /* channel 2 secondary control */
	*dma_ptr = 0x90; 
	dma_ptr = (unsigned int *)0x184001C; /* DMA Channel 2 Destination Address */
	*dma_ptr = (unsigned int)PCI_FIFO_WR;

	/* Re-configure Asynchronous interface for CE1 */
	/* Increase cycle length to talk to PLX chip */
    
	WriteCE1(PLX_CFG_MODE);

	/* Read PLX Mailbox 2 to get PCI Shared Memory Address */
	pci_cfg_ptr = (volatile unsigned int *)0x14000C8; /* PLX Mailbox 2 */
	DMA_base = *pci_cfg_ptr;    
	
	/* Configure PCI Bridge Chip for DMA Transfers from PCI FIFOs */

	pci_cfg_ptr = (volatile unsigned int *)0x1400100; 
	*pci_cfg_ptr   = 0xD43;           /* DMAMODE0 */
	pci_cfg_ptr = (volatile unsigned int *)0x1400108; 
	*pci_cfg_ptr = PCI_FIFO_RD;       /* DMALADR0 */
	pci_cfg_ptr = (volatile unsigned int *)0x1400110; 
	*pci_cfg_ptr = 0x8;           /* DMADPR0  */

	/* Re-program MARBR Register for test */
	pci_cfg_ptr = (volatile unsigned int *)0x1400088;
	*pci_cfg_ptr = 0x01270808;           /* MARBR  */

	/* Re-program INTCSR to enable DMA CH0 Interrupt */

	pci_cfg_ptr = (volatile unsigned int *)0x14000E8; 
	*pci_cfg_ptr = 0x50000;           /* INTCSR */

	Count = 0;
	pkt = (PACKETHEADER *)((char *)Fifo + Fifo->header_off);
	pkt->cmd.flag = 0;  // Immediately shut off the ready flag
			
	/* Clear DMA interrupt -- just in case */	

	pci_cfg_ptr = (volatile unsigned int *)0x1400128; 
	*pci_cfg_ptr = 0x8;           /* DMACSR0 */
	*pci_cfg_ptr = 0x0;           /* DMACSR0 */
	IRQ_reset(IRQ_EVT_EXTINT6);	 /* Clear any pending interrupts */
	IRQ_Enable(IRQ_EVT_EXTINT6); /* Enable PCI_STOP interrupt */
	IRQ_Enable(IRQ_EVT_EXTINT5); /* Enable Fifo Half-full interrupt */

	*fifoclr = 0;	/* Clear i,q input fifo */
	WriteCE1(HIGH_SPEED_MODE);	 /* re-enable high speed mode */
	HWI_enable();   /* Enable Hardware Interrupts */
	pkt->cmd.flag = 1;

}

void data_xfer_loop(void)
{
	volatile unsigned int *Mailbox5Ptr; 
	int Tfer_sz;

	/* Fill PCI Shared Memory forever */
	while(1) {
 		SEM_pend(&data_ready,SYS_FOREVER);
		CurPkt->data.info.pulse_num_low = pulse_num_low;
		CurPkt->data.info.pulse_num_high = pulse_num_high;
		if (sbsram_hits == PCIHits) { // full load
			Tfer_sz = PCIHits * (HEADERSIZE + (gates * bytespergate));  /* in bytes */
			//	moved from above
			IRQ_Disable(IRQ_EVT_EXTINT5);		/* Disable Fifo interrupt during data xfer */
			dma_fifo(Tfer_sz,(unsigned int )NPkt);       /* DMA NPkt to PCI Burst FIFO */
			sbsram_hits = 0;
			burstready = 1;  
		}
		// Read PLX Mailbox 5 to get channel mode: 
		// LO, HI, or combined via dynamic range extension algorithm 
		WriteCE1(PLX_CFG_MODE);
		Mailbox5Ptr = (volatile unsigned int *)0x14000D4; /* PLX Mailbox 5 */
		channelMode = ((unsigned int)*Mailbox5Ptr) & CHMODE_MASK; /* Host sets test actions in PLX Mailbox 5 */

#ifdef	CP2_TEST_SINUSOID
		synthesizeSine(Mailbox5Ptr);
#endif
		WriteCE1(HIGH_SPEED_MODE);	 /* re-enable high speed mode */
		IRQ_Enable(IRQ_EVT_EXTINT5);  /* re-enable fifo interrupt */
	}
}

//	perform DMA transfer from PCI Burst FIFO to host memory
void pci_burst_xfer()
{
	PACKET *dst;
	unsigned int offset;
	int Tfer_sz;

	while(1)	{
 		SEM_pend(&burst_ready,SYS_FOREVER);
		Tfer_sz = PCIHits * (HEADERSIZE + (gates * bytespergate));  /* in bytes */
		//	moved from above
		IRQ_Disable(IRQ_EVT_EXTINT5);		/* Disable Fifo interrupt during data xfer */
		dst = (PACKET *)fifo_get_write_address(Fifo);
		offset = (unsigned int )dst - PCIBASE; 
		dma_pci(Tfer_sz,(unsigned int )DMA_base+offset); 		/* DMA FIFO */
		fifo_increment_head(Fifo);
		burstready = 0; 
		WriteCE1(HIGH_SPEED_MODE);	 /* re-enable high speed mode */
		IRQ_Enable(IRQ_EVT_EXTINT5);  /* re-enable fifo interrupt */
	}	//	end while(1)
}


/* Adjusts properties of CE1 space for slower and faster peripherals */	
unsigned int WriteCE1(unsigned int newValue)
{
	static unsigned int curValue = 0;
	static volatile unsigned *csr = (volatile unsigned int *)	0x1800004; /* CE1 */
	
	unsigned int previous;
	HWI_disable();
	
	previous= curValue;
	*csr = curValue = newValue;
	HWI_enable();
	
	return previous;
}

   
void dma_fifo(int tsize, unsigned int source)
{

	int frame_cnt;
	int tfer_sz;
	unsigned int src;

/* Return Asynchronous interface for CE1 to initial settings */

	WriteCE1(HIGH_SPEED_MODE);
	src = source;

/* DMA Data into PCI FIFOs */

	frame_cnt = 1;
	tfer_sz = tsize >> 2;  /* convert from bytes to words for DSP */
	while(tfer_sz > 0xFFFF) {
		tfer_sz = tfer_sz >> 1;
		frame_cnt = frame_cnt << 1;
	}

	dmaTransfer(2, 0x11, (int*)src, 0, (frame_cnt << 16) | (tfer_sz & 0xFFFF), tfer_sz);
}

void dma_pci(int tsize, unsigned int pci_dst)
{

	volatile unsigned int *pci_cfg_ptr;

	/* Re-configure Asynchronous interface for CE1 */
	/* Increase cycle length to talk to PLX chip */

	WriteCE1(PLX_CFG_MODE);
	                                               
	pci_cfg_ptr = (volatile unsigned int *)0x1400104; 

	*pci_cfg_ptr = pci_dst;     /* DMAPADR0 */
	pci_cfg_ptr = (volatile unsigned int *)0x140010C; 
	*pci_cfg_ptr = tsize;   /* DMASIZ0  */	
	
	/* Tell PCI Bridge Chip to DMA data */

	pci_cfg_ptr = (volatile unsigned int *)0x1400128; 
	*pci_cfg_ptr = 0x3;           /* DMACSR0 */
	SEM_pend(&fifo_ready,SYS_FOREVER);	

}

void 
createSineTestWaveform(float freq)	//	create test sine waveform of freq; store to SINstore
{
	float	x;
	float *	test_dst;
	int		i; 
	 
	test_dst = SINstore;
	x = freq*2.0*PI;
	TestWaveformiMax	= (unsigned int)(1.0/freq);
	//	compute sinusoid suitable along pulse, for as many gates as possible: 6/uS, 1000Hz
	for(i = 0; i <= TestWaveformiMax; i++) {	//	compute one complete test waveform (up to 2000 gates)
		*test_dst++ = 16777216.0*TestWaveformAmplitude*cosf(x*i);
		*test_dst++ = -16777216.0*TestWaveformAmplitude*sinf(x*i);
	}

}


void
synthesizeSine(unsigned int *Mailbox5Ptr) 
{
		freqAdjust = ((unsigned int)*Mailbox5Ptr) & FREQADJ_MASK; /* Host sets channelMode in PLX Mailbox 5 */
		if (freqAdjust)	{	//	host requested frequency adjust
			switch(freqAdjust)	{
				case	INCREMENT_TEST_SINUSIOD_COARSE:
//					test_freq += 0.001;		//	was 0.001
					test_freq += 0.0002;	
					break; 
				case	INCREMENT_TEST_SINUSIOD_FINE:
					test_freq += 0.000001;
					break; 
				case	DECREMENT_TEST_SINUSIOD_COARSE:
//					if	(test_freq > 0.001)	test_freq -= 0.001;
					if	(test_freq > 0.0002)	test_freq -= 0.0002;
					break; 
				case	DECREMENT_TEST_SINUSIOD_FINE:
					if	(test_freq > 0.000002)	test_freq -= 0.000001;
					break; 
				default:
					break;
			}
			createSineTestWaveform(test_freq); // generate new test waveform for output
			freqAdjust = 0;	//	do once
			channelMode = ((unsigned int)*Mailbox5Ptr); //	use cM here to get PLX Mailbox 5 contents 
			*Mailbox5Ptr = channelMode & ~FREQADJ_MASK;	//	turn off adjust request
			channelMode = ((unsigned int)*Mailbox5Ptr) & CHMODE_MASK;	//	restore channelMode to former glory 
		}
		amplAdjust = ((unsigned int)*Mailbox5Ptr) & AMPLADJ_MASK; /* Host sets channelMode in PLX Mailbox 5 */
		if (amplAdjust)	{	//	host requested amplitude adjust
			switch(amplAdjust)	{
				case	INCREMENT_TEST_AMPLITUDE_COARSE:
					TestWaveformAmplitude *= 3.0; 
					break; 
				case	INCREMENT_TEST_AMPLITUDE_FINE:
					TestWaveformAmplitude *= 1.1; 
					break; 
				case	DECREMENT_TEST_AMPLITUDE_COARSE:
					TestWaveformAmplitude /= 3.0; 
					break; 
				case	DECREMENT_TEST_AMPLITUDE_FINE:
					TestWaveformAmplitude /= 1.1; 
					break; 
				default:
					break;
			}
			if (TestWaveformAmplitude >= 100.0)	//	at maximum amplitude
				TestWaveformAmplitude = 1.0;	//	cap it there
			createSineTestWaveform(test_freq); // generate new test waveform for output
			amplAdjust = 0;
			channelMode = ((unsigned int)*Mailbox5Ptr); //	use cM here to get PLX Mailbox 5 contents 
			*Mailbox5Ptr = channelMode & ~AMPLADJ_MASK;	//	turn off adjust request
			channelMode = ((unsigned int)*Mailbox5Ptr) & CHMODE_MASK;	//	restore channelMode to former glory 
		}
}


void
dmaTransfer(int channel, 
			unsigned int controlWord, 
			int *src, 
			int *dst, 
			int transferCount,
			int globalReload) 
{

	static int dmaChannelPriCtlReg[3] = { 0x1840000U, 0x1840040U, 0x1840004U};
//	static unsigned int dmaChannelSecCtlReg[3] = { 0x1840008U, 0x1840048U, 0x184000CU};
	static int dmaChannelSourceReg[3] = { 0x1840010U, 0x1840050U, 0x1840014U};
	static int dmaChannelDestReg[3]   = { 0x1840018U, 0x1840058U, 0x184001CU};
	static int dmaChannelCountReg[3]  = { 0x1840020U, 0x1840060U, 0x1840024U};
	static int dmaChannelReloadReg[3] = {         0U,         0U, 0x1840028U};


    volatile int *p;
    volatile int **pp;

	if (src) {
		pp  = (volatile int **)dmaChannelSourceReg[channel];
		*pp = src;
	}

	if (dst) {
		pp  = (volatile int **)dmaChannelDestReg[channel];
		*pp = dst;
	}

	p  = (volatile int *)dmaChannelCountReg[channel];
	*p = transferCount & 0xffff;

	if(globalReload) {
		p  = (volatile int *)dmaChannelReloadReg[channel];
		*p = globalReload & 0xffff;
	}

	p  = (volatile int *)dmaChannelPriCtlReg[channel];
	*p = controlWord;

	while((*p & 0xc) == 0x4)
		asm("	NOP");


}

