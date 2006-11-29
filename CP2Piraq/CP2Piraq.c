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

#include 	"piraqComm.h"
#include 	"CP2Piraqcfg.h"
#include 	"local_defs.h"
#include	"coeff.h"


#define 	PCI_FIFO_WR 	0x1400310
#define		PCI_FIFO_RST 	0x1400314
#define 	PCI_FIFO_RD 	0x1C00
#define 	PCI_LOCAL_BASE 	0x3000000
#define 	MAX_BUFFER_SZ 	0x400000
#define 	TIMER_CLOCK 2	5.e-09

/// address of the dma destination register
static int dmaDestReg[3]      = { 0x1840018U, 0x1840058U, 0x184001CU };
/// address of the dma primary control register
static int dmaPriCtlReg[3]    = { 0x1840000U, 0x1840040U, 0x1840004U };
/// address of the dma secondary control register
static int dmaSecCtlReg[3]    = { 0x1840008U, 0x1840048U, 0x184000CU };
/// address of the dma source address register
static int dmaSourceReg[3]    = { 0x1840010U, 0x1840050U, 0x1840014U };
/// address of the dma transfer count register
static int dmaTransCountReg[3]= { 0x1840020U, 0x1840060U, 0x1840024U };

/// address of the dma auxillary control register
static int dmaAuxCtlreg         = 0x01840070;
/// address of the dma global adddress register a
static int dmaGlobalAddressRegA = 0x01840038U;
/// address of the dma  adddress register b
static int dmaGlobalAddressRegB = 0x0184003CU;
/// address of the dma  adddress register c
static int dmaGlobalAddressRegC = 0x01840068U;
/// address of the dma  adddress register d
static int dmaGlobalAddressRegD = 0x0184006CU;
/// address of the dma count reload register a
static int dmaCountReloadRegA   = 0x01840028U;
/// address of the dma count reload register b 
static int dmaCountReloadRegB   = 0x0184002CU;
/// address of the dma global index register a
static int dmaGlobalIndexRegA   = 0x01840030U;
/// address of the dma  index register b
static int dmaGlobalIndexRegB   = 0x01840034U;

int *a2dFifoBuffer;			// receives the I/Q data from the A2D fifos.

float IQoffset[4*NUMCHANS];	//	CP2: compute offsets for both CHA and CHB

unsigned int *Led_ptr,DMA_base, Period;

void initializeDma();
void
dmaTransfer(int channel, 
			int *src, 
			int *dst, 
			int transferCount,
			int globalReload); 
void delay(void);
void initDsp(void);
void dma_pci(int tsize, unsigned int pci_dst);
void dma_fifo(int tsize, unsigned int source);
void startPciTransfer();
int burstready = 0;

CircularBuffer  	*Fifo;
PPACKET	*CurPkt;	// DSP-internal PACKET malloc w/1-channel data: contains current header information
PPACKET	*sbsRamBuffer;		// sbsram N-PACKET MEM_alloc w/1-channel data 

int		samplectr;     // sample counter, time series ctr
int		nPacketsPerBlock;       // #hits combined for one PCI Bus transfer
int     sbsram_hits;   // #hits in SBSRAM

float 	ioffset0, qoffset0, ioffset1, qoffset1;	// DC offsets

int	    led0flag;       // holds the state of led0
unsigned int*	pLed0 = (unsigned int *)(0x1400308);  /* LED0 */  
unsigned int*	pLed1 = (unsigned int *)(0x140030C);  /* LED1 */

float   sumnorm;

unsigned long pulse_num_low;
unsigned long pulse_num_high;
unsigned long beam_num_low;
unsigned long beam_num_high;

unsigned int  channelMode; 	// sets channelselect() processing mode 
int     gates;
int     bytespergate;
int     hits;
int     boardnumber;

unsigned int freqAdjust;	// sets test-data frequency adjustment 
unsigned int TestWaveformiMax;	//	index of complete wavelength
unsigned int amplAdjust;	// sets test-data amplitude adjustment 
float		 TestWaveformAmplitude = 1.0; 
float	*SINstore;  // pointer to fake sinusoidal input data
float	test_freq; 

void main()
{ 
	/* Initialize global variable(s) */
	CSL_Init();  /* initialize the TI chip support library */
}

void initTask(void)
{
	int*            fifoclr;
	unsigned int*   burst_fifo; 
	volatile unsigned int *pci_cfg_ptr;
	int 			i;
	volatile int 	ii,j;
	int 			sbsram_seg;
	PINFOHEADER	*pkt;
	unsigned int	*src, *dst;

/* Initialize Essential DSP Control Registers */
/* setup EMIF Global and Local Control Registers */
/* this should allow external memory to be used! */

	initDsp(); 

	
	/* Clear the LEDs */

	led0flag = 1;
	*pLed0   = led0flag;
	*pLed1   = 0x1;

	/* set up SBSRAM segments */

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

	Fifo = pfifo_open("PRQDATA");  /* the argument is not used */

	/* Transfer packet header to current buffer */
    src              = (unsigned int *)pfifo_get_header_address(Fifo);
    pkt              = (PINFOHEADER *)src; 
	gates            = pkt->gates; 
	hits             = pkt->hits; 
	bytespergate     = pkt->bytespergate;
	boardnumber      = pkt->channel;
	nPacketsPerBlock = pkt->packetsPerBlock;

	// allocate a complete 1-channel PACKET; it contains current pulse, 
	// header plus data, post channel-select
	CurPkt = (PPACKET *)malloc((sizeof(PINFOHEADER) + (gates * bytespergate)));
	
	// method using DSP-internal PACKET for header parameter maintenance
	dst = (unsigned int *)CurPkt;	
		 
	// /4: move header as integers
	for(i = 0; i < sizeof(PINFOHEADER)/4; i++)	 
		*dst++ = *src++;

	// sbsRamBuffer PACKET pointer to one hit in sbsram N-hit alloc
	// CP2 PCI Bus transfer size: Nhits * (HEADERSIZE + (config->gatesa * bytespergate) 

	// PACKET pointer to one hit in sbsram N-hit alloc
	sbsRamBuffer = (PPACKET *)MEM_alloc(sbsram_seg, 
				nPacketsPerBlock * (sizeof(PINFOHEADER) + (gates * bytespergate)),
				0); 

	// hwData DSP-internal 2-channel hwFIFO data array, interleaved I1, Q1, I2, Q2
  	a2dFifoBuffer = (int *)malloc((gates * 2 * bytespergate));

	/* zero single-pulse buffer */
	memset(a2dFifoBuffer,0,(gates * 2 * bytespergate));	

	WriteCE1(PLX_CFG_MODE);
   	  
#ifdef	CP2_TEST_SINUSOID		// CP2 test sine wave ... along pulse in timeseries data
	SINstore = (float *)MEM_alloc(sdram0_seg,2000000 * sizeof (float),0);	//	allow up to 2000000 data
	test_freq = 0.002;	//	should be equivalent to 60002000.0Hz
	createSineTestWaveform(test_freq); 
#endif

	/* Initialize pulse and beam counters */
	pulse_num_low  = CurPkt->info.pulse_num_low;
	pulse_num_high = CurPkt->info.pulse_num_high;
	beam_num_high  = CurPkt->info.beam_num_high;
	beam_num_low   = CurPkt->info.beam_num_low;
		
	//	Calculate DC offset normalization
	// CP2 nix hits in normalization.
	sumnorm = 1.0/((float)CurPkt->info.gates);  

	/* Clear the PCI FIFO */

	burst_fifo = (unsigned int *)PCI_FIFO_RST;
	*burst_fifo = 0x0;

	// set up the dma channels
	initializeDma();

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

	pkt = (PINFOHEADER *)((char *)Fifo + Fifo->header_off);
	pkt->flag = 0;  // Immediately shut off the ready flag
			
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
	pkt->flag = 1;

}

/////////////////////////////////////////////////////////////////
void fillBurstFifoTask(void)
{
	volatile unsigned int *Mailbox5Ptr; 
	int Tfer_sz;

	/* Fill burst fifo from SBSRAM */
	while(1) {
 		SEM_pend(&FillBurstFifo,SYS_FOREVER);
		CurPkt->info.pulse_num_low = pulse_num_low;
		CurPkt->info.pulse_num_high = pulse_num_high;
		if (sbsram_hits == nPacketsPerBlock) { // full load
			Tfer_sz = nPacketsPerBlock * (sizeof(PINFOHEADER) + (gates * bytespergate));  /* in bytes */
		    // Disable A2D half-full fifo interrupts during data xfer */
			IRQ_Disable(IRQ_EVT_EXTINT5);		
			dma_fifo(Tfer_sz,(unsigned int )sbsRamBuffer);       /* DMA sbsRamBuffer to PCI Burst FIFO */
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

/////////////////////////////////////////////////////////////////

//	perform DMA transfer from PCI Burst FIFO to host memory
void pciTransferTask()
{
	PPACKET *dst;
	unsigned int offset;
	int Tfer_sz;
	volatile unsigned int *pci_cfg_ptr;

	while(1)	{

		// wait for the StartPciTransfer semaphore, which
		// signals that the burst fifo is ready to be transferred.
 		SEM_pend(&StartPciTransfer,SYS_FOREVER);
		Tfer_sz = nPacketsPerBlock * (sizeof(PINFOHEADER) + (gates * bytespergate));  /* in bytes */

		// Disable A2D Fifo interrupt during data xfer */
		IRQ_Disable(IRQ_EVT_EXTINT5);
		
		// determine the host side destination address		
		dst = (PPACKET *)pfifo_get_write_address(Fifo);
		offset = (unsigned int )dst - PCIBASE; 

		// Re-configure Asynchronous interface for CE1 
		// Increase cycle length to talk to PLX chip 
		WriteCE1(PLX_CFG_MODE);
	                                               
		// set pci dma destination register
		pci_cfg_ptr = (volatile unsigned int *)0x1400104; 
		*pci_cfg_ptr = DMA_base+offset;     /* DMAPADR0 */
	
		// set pci dma transfer size register
		pci_cfg_ptr = (volatile unsigned int *)0x140010C; 
		*pci_cfg_ptr = Tfer_sz;   /* DMASIZ0  */	
	
		// Tell PCI Bridge Chip to DMA data
		pci_cfg_ptr = (volatile unsigned int *)0x1400128; 
		*pci_cfg_ptr = 0x3;           /* DMACSR0 */
	
		// wait for the PciTransferFinished semaphore,
		// which is set by an interrupt
		SEM_pend(&PciTransferFinished, SYS_FOREVER);	

		pfifo_increment_head(Fifo);
		
		burstready = 0; 
		
		WriteCE1(HIGH_SPEED_MODE);	 /* re-enable high speed mode */
		IRQ_Enable(IRQ_EVT_EXTINT5);  /* re-enable A2D fifo interrupt */
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

   
/////////////////////////////////////////////////////////////////

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

	dmaTransfer(2, (int*)src, 0, (frame_cnt << 16) | (tfer_sz & 0xFFFF), tfer_sz);
}

/////////////////////////////////////////////////////////////////

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


/////////////////////////////////////////////////////////////////
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

/////////////////////////////////////////////////////////////////
/// Initialize the three dma controllers for the
/// specific jobs they will handle:
/// channel 0 - dma from internal ram to sbsram
/// channel 1 - dma from a2d fifos into internal memory
/// channel 2 - dma from sbsram to burst fifo
///
/// since none of the dma transfers are occuring
/// at the same time (all dma is polled until completion), 
/// in theory only one channel
/// could be used for all dma activities. However,
/// assigning them for specific tasks allows us to
/// only have to program some registers only once.
void
initializeDma()
{
	int i;
	int j;

	/* Initialize DMA Channel 0 for transfers from DSP memory to sbsram */
	*(unsigned int*)dmaPriCtlReg[0]    = 0x01000050;
	*(unsigned int*)dmaSecCtlReg[0]    = 0x00000090; 

	/* Initialize DMA Channel 1 for A/D FIFO transfers to DSP memory */
	*(unsigned int*)dmaPriCtlReg[1]    = 0xc0; 
	*(unsigned int*)dmaSecCtlReg[1]    = 0x90; 
	*(unsigned int*)dmaGlobalIndexRegA = 0x10;

	/* Initialize DMA Channel 2 for sbsram transfers to PCI burst fifo */
	*(unsigned int*)dmaPriCtlReg[2]    = 0x10; 
	*(unsigned int*)dmaSecCtlReg[2]    = 0x90; 
	*(unsigned int*)dmaDestReg[2]      = (unsigned int)PCI_FIFO_WR;

	// the following code just accesses all of the 
	// dma register addresses, so that the compiler
	// warnings are suppresed for unaccessed variables
	for (i = 0; i < 3; i++) {

		j = dmaDestReg[i];
		j = dmaPriCtlReg[i];
		j = dmaSecCtlReg[i];
		j = dmaSourceReg[i];
		j = dmaTransCountReg[i];
	}
	j = dmaAuxCtlreg;
	j = dmaGlobalAddressRegA;
	j = dmaGlobalAddressRegB;
	j = dmaGlobalAddressRegC;
	j = dmaGlobalAddressRegD;
	j = dmaCountReloadRegA;
	j = dmaCountReloadRegB;
	j = dmaGlobalIndexRegA;
	j = dmaGlobalIndexRegB;

	i = j;
}

/////////////////////////////////////////////////////////////////
void
dmaTransfer(int channel,
			int *src, 
			int *dst, 
			int transferCount,
			int countReloadA) 
{
    volatile int *p;
    volatile int **pp;

	if (src) {
		pp  = (volatile int **)dmaSourceReg[channel];
		*pp = src;
	}

	if (dst) {
		pp  = (volatile int **)dmaDestReg[channel];
		*pp = dst;
	}

	p  = (volatile int *)dmaTransCountReg[channel];
	*p = transferCount & 0xffff;

	// if count reload is set, program the 
	// count reload register A
	if(countReloadA) {
		p  = (volatile int *)dmaCountReloadRegA;
		*p = countReloadA & 0xffff;
	}

	p  = (volatile int *)dmaPriCtlReg[channel];
	*p = *p | 1;

	while((*p & 0xc) == 0x4)
		asm("	NOP");


}

