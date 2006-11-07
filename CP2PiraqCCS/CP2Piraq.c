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
#include 	"single_p3iq_dcfg.h"
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
int  Tfer_sz, Count, Stgr, Cfltr, Maxgates;
unsigned int *Led_ptr,DMA_base, Period;
void delay(void);
void init_dsp(void);
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
float 	*CFptr, sumnorm, hitnorm;
float	HWfifo_latency_ratio;
unsigned long	pulse_num_low, pulse_num_high, beam_num_low, beam_num_high;
unsigned int channelMode; 	// sets channelselect() processing mode 
unsigned int freqAdjust;	// sets test-data frequency adjustment 
unsigned int TestWaveformiMax;	//	index of complete wavelength
unsigned int amplAdjust;	// sets test-data amplitude adjustment 
float		 TestWaveformAmplitude = 1.0; 

int gates, bytespergate, hits, boardnumber;
unsigned int * PMACDPRAMBaseAddress;	//	base address for accessing PMAC data 
unsigned int PMACDPRAMBaseData;		//	data at base address for accessing PMAC data 

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
	int 			sbsram_seg, sdram0_seg;
	float 			*fake_in;
	PACKETHEADER	*pkt;
	unsigned int	*src, *dst;
	PACKET			*lsrc, *ldst;
	float 			* dbg_dst;
	volatile unsigned int *Mailbox4Ptr; 
volatile unsigned int *DMPBAMPtr; 
volatile unsigned int *DMPBAMDataPtr;
volatile unsigned int DMPBAM, DMPBAMPMAC, DMPBAMPIRAQ1, DMPBAMPIRAQ2;
unsigned short DMPBAMData;	//	data at DMPBAMDataPtr
unsigned char * DMPBAMDataPtrByte;	
/* Initialize Essential DSP Control Registers */
/* setup EMIF Global and Local Control Registers */
/* this should allow external memory to be used! */

	init_dsp(); 
	ledflag = 1;
	
	/* Clear the LEDs */

	Led_ptr = (unsigned int *)(0x1400308);  /* LED0 */  
	led1_ptr = (unsigned int *)(0x140030C);  /* LED1 */
	*Led_ptr = ledflag;
	*led1_ptr = 0x1;

	/* set up SDRAM, SBSRAM segments */

    sbsram_seg = MEM_define((Ptr) 0x400000, 0x40000, 0);
    sdram0_seg = MEM_define((Ptr) 0x2000000, 0x1000000,0); // 16 MB of SDRAM

	sbsram_hits = 0; 
	// CP2 PCI Bus transfer size: Nhits * (HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON)
	
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
	PCIHits = (unsigned int)UDPSENDSIZE / (HEADERSIZE + (gates * bytespergate) + BUFFER_EPSILON); 
	if	(PCIHits % 2)	//	odd #hits computed
		PCIHits--;		//	make it even
	// allocate a complete 1-channel PACKET; it contains current pulse, header plus data, post channel-select
	CurPkt = (PACKET *)malloc((HEADERSIZE + (gates * bytespergate) + BUFFER_EPSILON)); 
	dst = (unsigned int *)CurPkt;		// method using DSP-internal PACKET for header parameter maintenance 
	for(i = 0; i < HEADERSIZE/4; i++)	// /4: move header as integers 
		*dst++ = *src++;
	lsrc = (PACKET *)fifo_get_header_address(Fifo);
	ldst = (PACKET *)CurPkt;	// new method
	// NPkt PACKET pointer to one hit in sbsram N-hit alloc
	// CP2 PCI Bus transfer size: Nhits * (HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON)
	NPkt = (PACKET *)MEM_alloc(sbsram_seg, PCIHits * (HEADERSIZE + (gates * bytespergate) + BUFFER_EPSILON), 0); // PACKET pointer to one hit in sbsram N-hit alloc
	// hwData DSP-internal 2-channel hwFIFO data array, interleaved I1, Q1, I2, Q2
  	hwData = (int *)malloc((gates * 2 * bytespergate) + BUFFER_EPSILON);
	memset(hwData,0,(gates * 2 * bytespergate) + BUFFER_EPSILON);		/* zero single-pulse buffer */

	/* Read PLX Mailbox 4 to get PMAC DPRAM base address */
	WriteCE1(PLX_CFG_MODE);
	Mailbox4Ptr = (volatile unsigned int *)0x14000D0; /* PLX Mailbox 4 */
	PMACDPRAMBaseAddress = ((unsigned int *)*Mailbox4Ptr); /* Host sets PMAC DPRAM base address in PLX Mailbox 4 */

#if 0
//	test: get PIRAQ Physical address, compare w/host initialization value: works
DMPBAMPtr = (volatile unsigned int *)0x14000A8; /* A8: PLX DMPBAM register */
DMPBAM = *DMPBAMPtr; /* Host sets PMAC DPRAM base address in PLX Mailbox 4 */
//	mimic PIRAQ initialization on host: 
DMPBAMPMAC = (DMPBAM & 0x0000FFFF) | ((volatile unsigned int)PMACDPRAMBaseAddress & 0xFFFF0000);
*DMPBAMPtr = DMPBAMPMAC;	//	set pointer to PMAC Physical memory in DMPBAM
//	get PMAC DPRAM pointer contents 
//	PMACDPRAMBaseData = *PMACDPRAMBaseAddress;	//	get data from PMAC DPRAM: constant 0
//	get data from location 0 in this PMAC PCI memory space: 
DMPBAMDataPtr = 0;   
DMPBAMDataPtr += ((0x3000000+0x8000)/4);	//	add an offset
//crash:DMPBAMDataPtr += ((0x3000000+0x100000-4)/4);	//	add an offset
//read from location:
PMACDPRAMBaseData = *DMPBAMDataPtr;
//	get data from location at value of PMACDPRAMBaseAddress: 
//PMACDPRAMBaseData = *(PMACDPRAMBaseAddress+0); 
*DMPBAMPtr = DMPBAM;	//	restore pointer to PIRAQ-host shared physical memory
#endif
#if 0
//	test add delta between PIRAQs physical addresses to DMPBAM, so accesses are based from 2nd card rather
//	than 1st:
	WriteCE1(PLX_CFG_MODE);	//	""
DMPBAMPtr = (volatile unsigned int *)0x14000A8; /* A8: PLX DMPBAM register */
DMPBAMPIRAQ1 = *DMPBAMPtr; /* Host sets PMAC DPRAM base address in PLX Mailbox 4 */
DMPBAMPIRAQ2 = *DMPBAMPtr; /* Host sets PMAC DPRAM base address in PLX Mailbox 4 */
DMPBAMPIRAQ2 = 0x008f01c1;	//	explicit value 
*DMPBAMPtr = DMPBAMPIRAQ2;	//	using board2 physical address
//DMPBAM = *DMPBAMPtr;
//	pointer to integer:
//DMPBAMDataPtr = 0;   
//DMPBAMDataPtr += ((0x3000000+0)/4)+(0x1400/4);	//	add an offset; /4  for 32-bit entities
//	pointer to byte:
DMPBAMDataPtrByte = 0;   
DMPBAMDataPtrByte += 0x3000000+0xb400;	//	add an offset 
//DMPBAMDataPtrByte += 0x3000000+0x410;	//	add an offset 
DMPBAMData = (unsigned short)*DMPBAMDataPtrByte;
//PMACDPRAMBaseData = DMPBAMData;	//	send value computed to host 
//PMACDPRAMBaseData = DMPBAM;	//	send value computed to host
//	for	(ii = 0; ii < 1000000; ii++)	{ 
	while(1)	{ 
DMPBAMData |= STAT0_PCLED + 0x0; 
//off:DMPBAMData &= ~STAT0_PCLED; 
//*DMPBAMDataPtr = (volatile unsigned int)DMPBAMData;
*DMPBAMDataPtrByte = (volatile unsigned short)DMPBAMData;
	for (j = 0; j < 20; j++)	;	//	count down
//DMPBAMData &= ~STAT0_PCLED; 
//*DMPBAMDataPtr = (volatile unsigned int)DMPBAMData;
*DMPBAMDataPtrByte = (volatile unsigned short)DMPBAMData;
PMACDPRAMBaseData = DMPBAMData;	//	send value computed to host
	}
*DMPBAMPtr = DMPBAMPIRAQ1;	//	restore pointer to PIRAQ-host shared physical memory
	WriteCE1(HIGH_SPEED_MODE);	 /* re-enable high speed mode */
#endif
#if 0
//	test turn on PIRAQ1 LED: 
DMPBAMDataPtr = 0;   
DMPBAMDataPtr += ((0x3000000+0)/4)+(0x1400);	//	add an offset; /2 for 16-bit entities
//DMPBAMDataPtr += (0x1400);	//	add an offset; /2 for 16-bit entities
PMACDPRAMBaseData = *DMPBAMDataPtr;
//PMACDPRAMBaseData |= STAT0_PCLED; 
//off:
PMACDPRAMBaseData &= ~STAT0_PCLED; 
*DMPBAMDataPtr = PMACDPRAMBaseData;
exit(0);
#endif  
// Timeseries initialization
	
	Tsgate = ldst->data.info.ts_start_gate;
	Ntsgates = (ldst->data.info.ts_end_gate - Tsgate) + 1; 
	if(Ntsgates > TSMAX)  /* don't exceed maximum number of timeseries */
		Ntsgates = TSMAX;
   	  
	if(ldst->data.info.dataformat == 16) {
		Maxgates = 400;  		// if simplepp & clutterfilter
		Stgr = 1;
		Cfltr = 1;
	}
	else /* (ldst->data.info.dataformat == 17) */ {
		Maxgates = 600;			// else staggered PRT
		Stgr = 2;
		Cfltr = 0;
	}
	// test CurPkt functioning end-to-end: 
//	ldst->data.info.clutter_start[0] = ldst->data.info.hits; 
//	ldst->data.info.clutter_end[0] = ldst->data.info.channel; 
	
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
	sumnorm = 1.0/((float)ldst->data.info.gates); // CP2 nix hits in normalization. 

	/* Initialize DMA Channel 0 for ABP transfers */
	dma_ptr = (unsigned int *)0x1840000;  /* channel 0 primary control */
	*dma_ptr = 0x01000050; 
	dma_ptr = (unsigned int *)0x1840008;  /* channel 0 secondary control */
	*dma_ptr = 0x90; 

	/* Initialize DMA Channel 1 for FIFO transfers */

#ifdef DMA_EN1
	dma_ptr = (unsigned int *)0x1840040;  /* channel 1 primary control */
	*dma_ptr = 0xc0; 
	dma_ptr = (unsigned int *)0x1840048;  /* channel 1 secondary control */
	*dma_ptr = 0x90; 
	dma_ptr = (unsigned int *)0x1840030; /* DMA Global Index Register A */
	*dma_ptr = 0x10;
#endif

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
//	dma_ptr = (unsigned int *)0x1840014; /* DMA Channel 2 Source Address */
//	*dma_ptr = (unsigned int)Scratch;

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
//	*pci_cfg_ptr = 0x01240000;           /* MARBR  */

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
	PACKET *dst;
	float * dbg_dst;
	unsigned int * src, * SBSRAMdst; 
	unsigned int offset;
	unsigned int i; 
	volatile unsigned int *Mailbox5Ptr; 

	/* Fill PCI Shared Memory forever */
	while(1) {
 		SEM_pend(&data_ready,SYS_FOREVER);
		CurPkt->data.info.pulse_num_low = pulse_num_low;
		CurPkt->data.info.pulse_num_high = pulse_num_high;
		if (sbsram_hits == PCIHits) { // full load
			Tfer_sz = PCIHits * (HEADERSIZE + (gates * bytespergate) + BUFFER_EPSILON);  /* in bytes */
			//	moved from above
			IRQ_Disable(IRQ_EVT_EXTINT5);		/* Disable Fifo interrupt during data xfer */
			dst = (PACKET *)fifo_get_write_address(Fifo);
			dma_fifo(Tfer_sz,(unsigned int )NPkt);       /* DMA NPkt to PCI Burst FIFO */
			offset = (unsigned int )dst - PCIBASE; 
#if 0
			dma_pci(Tfer_sz,(unsigned int )DMA_base+offset); 		/* DMA FIFO */
			sbsram_hits = 0;
			/* increment packet index in fifo structure */
			fifo_increment_head(Fifo);
#endif
			sbsram_hits = 0;
			burstready = 1;  
		}
		/* Read PLX Mailbox 5 to get channel mode: LO, HI, or combined via dynamic range extension algorithm */
		WriteCE1(PLX_CFG_MODE);
		Mailbox5Ptr = (volatile unsigned int *)0x14000D4; /* PLX Mailbox 5 */
		channelMode = ((unsigned int)*Mailbox5Ptr) & CHMODE_MASK; /* Host sets test actions in PLX Mailbox 5 */

#ifdef	CP2_TEST_SINUSOID
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

	while(1)	{
 		SEM_pend(&burst_ready,SYS_FOREVER);
		Tfer_sz = PCIHits * (HEADERSIZE + (gates * bytespergate) + BUFFER_EPSILON);  /* in bytes */
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

/* generates a delay for led blinking */
void delay(void)
   {
   volatile int i,j;
    
   j = 1;
   for (i=1; i<500000; i++)
      j += i;
   }
   
void dma_fifo(int tsize, unsigned int source)
{
int frame_cnt, tfer_sz;
unsigned int *dma_ptr,*burst_fifo,*mem_ptr,src;
volatile unsigned int *dma_stat;

//	SEM_pend(&fifo_ready,SYS_FOREVER);	

/* Return Asynchronous interface for CE1 to initial settings */

	WriteCE1(HIGH_SPEED_MODE);
	src = source;

#ifndef DMA_EN2

   /* read data from the SBSRAM into the PCI FIFOs */
   /* this is for test use only */
	
	tfer_sz = tsize >> 2;  /* convert from bytes to words */
	mem_ptr = (unsigned int *)src;
	burst_fifo = (unsigned int *)PCI_FIFO_WR;
	for(i = 0; i < tfer_sz; i++) {
       	*burst_fifo = *mem_ptr++;
	}        

#endif
#ifdef DMA_EN2
/* DMA Data into PCI FIFOs */

	frame_cnt = 1;
	tfer_sz = tsize >> 2;  /* convert from bytes to words for DSP */
	while(tfer_sz > 0xFFFF) {
		tfer_sz = tfer_sz >> 1;
		frame_cnt = frame_cnt << 1;
	}
	dma_ptr = (unsigned int *)0x1840014; /* DMA Channel 2 Source Address */
	*dma_ptr = (unsigned int)src;	
	dma_ptr = (unsigned int *)0x1840024; /* DMA Channel 2 Transfer Counter */
	*dma_ptr = (frame_cnt << 16) | (tfer_sz & 0xFFFF);       
	dma_ptr = (unsigned int *)0x1840028; /* DMA Global Count Reload Register A */
	*dma_ptr = tfer_sz & 0xFFFF;
	dma_ptr = (unsigned int *)0x1840004;  /* channel 2 primary control */
	*dma_ptr = 0x11;  /* start DMA Channel 2 */  

/* Loop while waiting for transfer to complete */
/* This can be structured so that useful multi-tasking can be done! */

	dma_stat = (volatile unsigned int *)0x1840004;  /* channel 2 primary control */
	while((*dma_stat & 0xc) == 0x4)
		asm("	NOP");
#endif
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

void createSineTestWaveform(float freq)	//	create test sine waveform of freq; store to SINstore
{
	float	x;
	float *	test_dst;
	int		i; 
	 
	test_dst = SINstore;
//	x = freq*PI;
	x = freq*2.0*PI;
	TestWaveformiMax	= (unsigned int)(1.0/freq);
	//	compute sinusoid suitable along pulse, for as many gates as possible: 6/uS, 1000Hz
//	for(i = 0; i < 2000; i++) {	//	compute test waveform up to 2000 gates
//		*test_dst++ = 16777216.0*cosf(x*i);
//		*test_dst++ = -16777216.0*sinf(x*i);
//	}
	for(i = 0; i <= TestWaveformiMax; i++) {	//	compute one complete test waveform (up to 2000 gates)
		*test_dst++ = 16777216.0*TestWaveformAmplitude*cosf(x*i);
		*test_dst++ = -16777216.0*TestWaveformAmplitude*sinf(x*i);
	}

}
