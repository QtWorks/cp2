#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <conio.h>
#include <windows.h>
//#include <winsock2.h>
#include <mmsystem.h>
#include <sys/timeb.h>
#include <time.h>

#include "CP2exec.h" 
#include "CP2PIRAQ.h"
#include "../CP2Piraq/piraqComm.h"
#include "../include/proto.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define	TIMER_RESET	0
#define	TIMER_STOP	1
#define	TIMER_START	2

#define	TIMER_RQST	1

static	int	FIRSTIMER=1;
#define	OFFSET	10
int cp2timer_config(TIMER *timer, PINFOHEADER *info);
int   cp2timer_init(TIMER *timer, int boardnumber);
void cp2timer_reset(TIMER *timer);
void cp2timer_set(TIMER *timer);
void cp2timer_start(TIMER *timer);
void cp2timer_stop(TIMER *timer);
int	cp2timer_test(TIMER *timer);


int   cp2timer_init(TIMER *timer, int boardnumber)
   {
	PCI_CARD	*pcicard;
	int		reg_base;
	int		i,j,error;

	init_pci(); 

	/* do the most basic PCI test */
	pcicard = find_pci_card(PCITIMER_VENDOR_ID,PCITIMER_DEVICE_ID,boardnumber);
	if(!pcicard)
		return(-1);

	reg_base = pcicard->phys2;
	
	// Map the card's memory
	timer->base = (char *)pci_card_membase(pcicard,256);

   /* initialize the PCI registers for proper board operation */
//   out32(reg_base + 0x38, 0x00010000);  /* enable addint interrupts */
//   out32(reg_base + 0x3C, 0xFFFFE004);  /* reset control register */
//   out32(reg_base + 0x60, 0x81818181);  /* pass through register */

   out32(reg_base + 0x60, 0xA2A2A2A2);  /* pass through register */

   printf("PCI Configuration registers as seen from timer_read\n");
   for(j=0; j<8; j++)
   {
		printf("%02X ",4*4*j);
		for(i=0; i<4; i++)
		{
			error = pci_read_config32(pcicard,4*(i+4*j));  /* read 32 bit value at requested register */
			printf("%08lX ",error);
		}
		printf("\n");
	}
   
   printf("\nI/O Base: %8X\n",reg_base);
   for(j=0; j<8; j++)
     {
     printf("%02X ",4*4*j);
     for(i=0; i<4; i++)
      printf("%08lX ",in32(reg_base + 4*(i+4*j)));
     printf("\n");
     }
   return(1);
   }
   /* fill the timer structure based on the infoheader */
/* Return 0 on failure, 1 on success */
int cp2timer_config(TIMER *timer, PINFOHEADER *info)
   {
   int		i,dualprt;
   HILO		delay,width,prt;
   int		j,cycles,len;

   if(FIRSTIMER)
      {
      if(!cp2timer_init(timer,0))
         {printf("could not find timer card\n"); exit(0);}
      FIRSTIMER = 0;
      }

   /* initialize all parameters in the timer structure */
   memset(timer,0,sizeof(TIMER) - sizeof(char *));

   /* now set the pulsewidths a delays for the 6 timers */
   /* set timers to stagger delay the six frequencies */
   /* using the specified pulsewidth */
   width.hilo = COUNTFREQ * info->xmit_pulsewidth + 0.5;
   for(i=0; i<MAXTESTPULSE; i++)
      {
      delay.hilo = OFFSET + i * width.hilo;
      timer->tp[i].delay.byte.lo = delay.byte.lo;
      timer->tp[i].delay.byte.hi = delay.byte.hi;
      timer->tp[i].width.byte.lo = width.byte.lo;
      timer->tp[i].width.byte.hi = width.byte.hi;
      }
   
//   timer->seqlen = info->polarization;   /* the sequence based on the sequence length */
   timer->seqlen = 1;   /* for now the sequence will always be 1 deep */
   prt.hilo = info->prt  * COUNTFREQ + 0.5;		/* compute prt */

   /* check sanity of request */
   if(timer->seqlen > 12 || timer->seqlen < 1)	{printf("TIMER_CONFIG: invalid sequence length %d\n",timer->seqlen); return(0);}
   if(prt.hilo < 1 || prt.hilo > 65535)	{printf("TIMER_CONFIG: invalid PRT1 %d\n",prt.hilo); return(0);}
   dualprt = 1;
   len = timer->seqlen * dualprt;
   cycles = 1;

//   if(info->ctrlflags & CTRL_SECONDTRIP)  /* if second trip reduction required */
//      cycles = 4 / len;

   for(j=0; j<cycles; j++)
      {
      for(i=0; i<timer->seqlen * dualprt; i++)
         {
         /* program the 5 bytes for this sequence */
         timer->seq[i+j*len].period.byte.lo = prt.byte.lo;
         timer->seq[i+j*len].period.byte.hi = prt.byte.hi;
         timer->seq[i+j*len].pulseenable = (i?0x2F:0x3F);
//         timer->seq[i+j*len].polarization = (i?0:0x80) | info->polarization_array[i % timer->seqlen];  /* add in a frame sync */
      timer->seq[i+j*len].polarization = 0;
//         timer->seq[i+j*len].phasedata = (info->ctrlflags & CTRL_SECONDTRIP) ? PhaseSeq[i+j*len] : 0x00;
      timer->seq[i+j*len].phasedata = 0x00;
         }
      }

   timer->seqlen *= dualprt * cycles;		/* double if in dual prt mode */
         
   timer->seqdelay.hilo = OFFSET;
// CP2:	set timingmode 1 for external triggering of EOL Timing Board. 
//		set a nonzero value of sync. 
// eventually these parameters should read from config.tmr
#if 0	//	as it was
   timer->timingmode = 0;
#else	//	CP2
   timer->timingmode = 1;
   timer->sync.byte.lo = 1;		// synclo;  
   timer->sync.byte.hi = 0;		// synchi;  
#endif
   timer->clockfreq = SYSTEM_CLOCK;
   timer->reffreq = 10.0E6;
   timer->phasefreq = 50.0E3;

   return(1);	/* success */
   }

   void cp2timer_reset(TIMER *timer)
   {
   if(FIRSTIMER)
      {
      if(!cp2timer_init(timer,0))
         {printf("could not find timer card\n"); exit(0);}
      FIRSTIMER = 0;
      }

   // set request ID to STOP
   timer->base[0xFE] = TIMER_RESET; /* @@@ was base[0xFE * 4] */
   // set request flag
   timer->base[0xFF] = TIMER_RQST;	/* @@@ *4 signal the timer that a request is pending */
   cp2timer_test(timer);
   printf("Passed timer_reset\n");
   }

void cp2timer_stop(TIMER *timer)
   {
   if(FIRSTIMER)
      {
      if(!cp2timer_init(timer,0))
         {printf("could not find timer card\n"); exit(0);}
      FIRSTIMER = 0;
      }

   // set request ID to STOP
   timer->base[0xFE] = TIMER_STOP; /* @@@ was base[0xFF * 4] */
   // set request flag
   timer->base[0xFF] = TIMER_RQST;	/* @@@ *4 signal the timer that a request is pending */
   cp2timer_test(timer);
   printf("Passed timer_stop\n");
   }
/* open the timer board (once) and configure it with the timer structure */
void cp2timer_set(TIMER *timer)
   {
   int          value,hitcount,i,x,j,reg_base,boardnumber,error,base_add;
   char         c,buf[80];
   short        *iq;
   CONFIG       *config;
   double       num,den;
   int 			*iptr;
   int			a,r,n;
   HIMEDLO		ref,freq;
   
   if(FIRSTIMER)
      {
      if(!cp2timer_init(timer,0))
         {printf("could not find timer card\n"); exit(0);}
      FIRSTIMER = 0;
      }

//   timer_stop(timer);

   /* clear the sequence mem */
   for(i=0; i<0xC0; i++)	timer->base[i] = 0; /* @@@ *4 */

   /* set the dual port ram */

   /* The sequence starts at memory offset zero */
   /* BYTE1:  low byte of period */
   /* BYTE2:  high byte of period */
   /* BYTE3:  Pulse enables */
   /* BYTE4:  Polarization control (bits 8 - 15 on connector) */
   /* BYTE5:  Phase Data (bits 0 - 7 on connector) */
//   for(i=0; i<timer->seqlen; i++)
//   { /* @@@ took out ) * 4] on all of these */
      /* program the 5 bytes for this sequence */
//      timer->base[(i * 5 + 0) ] = timer->seq[i].period.byte.lo;
//      timer->base[(i * 5 + 1) ] = timer->seq[i].period.byte.hi;
//      timer->base[(i * 5 + 2) ] = timer->seq[i].pulseenable ^ 0x3F;
//      timer->base[(i * 5 + 3) ] = timer->seq[i].polarization; // ^ 7; This was for NEC
//      timer->base[(i * 5 + 4) ] = timer->seq[i].phasedata;
//   }


	// @@@  This has been changed for the optimized timer code
	//		there is a different sequence memory setup.
   for(i=0; i<timer->seqlen; i++)
   {
	  timer->base[(0x00 + i)] = timer->seq[i].period.byte.lo;
	  timer->base[(0x10 + i)] = timer->seq[i].period.byte.hi;
      timer->base[(0x20 + i)] = timer->seq[i].pulseenable ^ 0x3F;
      timer->base[(0x30 + i)] = timer->seq[i].polarization; 
      timer->base[(0x40 + i)] = timer->seq[i].phasedata;
   }

   /* now set the pulsewidths a delays for the 6 timers */
   for(i=0; i<MAXTESTPULSE; i++)
   { /* @@@ took out ) * 4] on all of these */
      timer->base[(0xC0 + i * 4 + 0) ] = timer->tp[i].delay.byte.lo;
      timer->base[(0xC0 + i * 4 + 1) ] = timer->tp[i].delay.byte.hi;
      timer->base[(0xC0 + i * 4 + 2) ] = timer->tp[i].width.byte.lo;
      timer->base[(0xC0 + i * 4 + 3) ] = timer->tp[i].width.byte.hi;
      }

   /* compute the data to go into the register */
   n = 0.5 + timer->clockfreq / timer->phasefreq;
   a = n & 7;
   n /= 8;
   r = 0.5 + timer->reffreq / timer->phasefreq;
   ref.himedlo = r << 2 | 0x100000;     					/* r data with reset bit */
   freq.himedlo = n << 7 | a << 2 | 0x100001;     		/* r data with reset bit */

   /* @@@ took out ) * 4] on all of these */
   /* now set up the special registers */
   timer->base[(0xD8 + 0) ] = freq.byte.lo;				//freqlo;
   timer->base[(0xD8 + 1) ] = freq.byte.med;				//freqmed;
   timer->base[(0xD8 + 2) ] = freq.byte.hi;				//freqhi;
   timer->base[(0xD8 + 3) ] = ref.byte.lo;				//reflo;
   timer->base[(0xD8 + 4) ] = ref.byte.med;				//refmed;
   timer->base[(0xD8 + 5) ] = ref.byte.hi;				//refhi;
   timer->base[(0xD8 + 6) ] = 0;					//div;
   timer->base[(0xD8 + 7) ] = 0;					//spare;
   
   timer->base[(0xD8 +  8) ] = timer->sync.byte.lo;		// synclo;
   timer->base[(0xD8 +  9) ] = timer->sync.byte.hi;		// synchi;
   timer->base[(0xD8 + 10) ] = timer->seqdelay.byte.lo;	// sequence delay lo
   timer->base[(0xD8 + 11) ] = timer->seqdelay.byte.hi;	// sequence delay hi
   timer->base[(0xD8 + 12) ] = timer->seqlen;		// sequence length
   
   timer->base[(0xD8 + 13) ] = 0;					// current sequence count
   timer->base[(0xD8 + 14) ] = timer->timingmode;	// timing mode

//   timer_reset(timer);
   }
/* poll the command response byte and handle timeouts */
int	cp2timer_test(TIMER *timer)
   {
   volatile	int	temp;
   time_t	first;
   
   if(FIRSTIMER)
      {
      if(!cp2timer_init(timer,0))
         {printf("could not find timer card\n"); exit(0);}
      FIRSTIMER = 0;
      }

   /* wait until the timer comes ready */
   first = time(NULL) + 3;   /* wait 3 seconds */
   // @@@  Taking away the *4 - An Address calculation is unnecessary
   while((timer->base[0xFF] & 1))   /* @@@ was base[0xFF * 4] */
      {
      Sleep(1);
      if(time(NULL) > first)
         {
         printf("Timeout waiting for PCITIMER to respond to request\n");
         exit(0);
         }
      }
   timer->base[0xFF] = 0x02; /* @@@ was base[0xFF * 4] */
	return(0);   
}
   
void cp2timer_start(TIMER *timer)
   {
   int	sec;

   if(FIRSTIMER)
      {
      if(!cp2timer_init(timer,0))
         {printf("could not find timer card\n"); exit(0);}
      FIRSTIMER = 0;
      }

   /* start the timer assuming the first pulse occured at unix time zero */
//   if(timer->timingmode == TIMER_SYNC)
//      {
//      sec = half_second_wait();		/* wait until half sec, return time of next second */
//	  timer->pulsenumber
//      }

   // set request ID to START
   timer->base[0xFE] = TIMER_START;	/* @@@ *4 signal the timer that a request is pending */
   // set request flag
    timer->base[0xFF] = TIMER_RQST;	/* @@@ *4 signal the timer that a request is pending */
   cp2timer_test(timer);
   printf("Passed timer_start\n");
   }
 

//#ifdef CP2_TESTING		// switch ON test code for CP2 
// test drx data throughput limits by varying data packet size w/o changing DSP computational load:  
//#define			DRX_PACKET_TESTING	// define to activate data packet resizing for CP2 throughput testing. 
//#endif
//#define TESTING_TIMESERIES // compute test diagnostic timeseries data in one of two piraq channels: 
//#define TESTING_TIMESERIES_RANGE	// test dynamic reassignment of timeseries start, end gate using 'U','D'

int keyvalid() 
{
	while(!_kbhit()) { } // no keystroke 
	getch();    // get it
	return 1;
}


/////////////////////////////////////////////////////////////////////////////
int 
main(int argc, char* argv[], char* envp[])
{
	CP2PIRAQ* piraq1 = 0;
	CP2PIRAQ* piraq2 = 0;
	CP2PIRAQ* piraq3 = 0;

	CONFIG *config1, *config2, *config3;
	char fname1[100]; char fname2[100]; char fname3[100]; // configuration filenames
	char* destIP = "192.168.3.255";

	// set/compute #hits combined by piraq: equal in both piraq executable (CP2_DCCS3_1.out) and CP2exec.exe 
	unsigned int Nhits; 
	int outport;
	char c;
	int piraqs = 0;   // board count -- default to single board operation 
	FILE * dspEXEC; 

	__int64 pulsenum;
	__int64 beamnum; 

	int	PIRAQadjustAmplitude = 0; 
	int	PIRAQadjustFrequency = 1; 
	TIMER ext_timer; // structure defining external timer parameters 

	config1	= new CONFIG; 
	config2	= new CONFIG; 
	config3	= new CONFIG; 

	if (argc < 4) {
		printf("CP2exec <DSP filename> <rcv data format: 'f' floats, 's' unsigned shorts> <piraq mask> [config file]\n"); 
		exit(1);
	}

	if ((dspEXEC = fopen(argv[1],"r")) == NULL) // DSP executable file not found 
	{ 
		char* dspFileName = argv[1];
		printf("Usage: %s <DSP filename> DSP executable file not found\n", argv[0]); exit(0); 
	} 
	fclose(dspEXEC); // existence test passed; use command-line filename

	printf("file %s will be loaded into piraq\n", argv[1]); 

	piraqs = atoi(argv[3]); 

	if (argc > 4) { // entered a filename
		strcpy(fname1, argv[4]);
		strcpy(fname2, argv[4]);
		strcpy(fname3, argv[4]);
	}
	else {
		strcpy(fname1, "config1");	 
		strcpy(fname2, "config2");	 
		strcpy(fname3, "config3");	 
	}

	printf(" config1 filename %s will be used\n", fname1);
	printf(" config2 filename %s will be used\n", fname2);
	printf(" config3 filename %s will be used\n", fname3);
	printf("\n\nTURN TRANSMITTER OFF for piraq dc offset measurement.\nPress any key to continue.\n"); 
	while(!kbhit())	;
	c = toupper(getch()); // get the character

	outport = 3100; 

	cp2timer_stop(&ext_timer); // stop timer card 

	// read in fname.dsp, or use configX.dsp if NULL passed. set up all parameters
	readconfig(fname1, config1);    
	readconfig(fname2, config2);    
	readconfig(fname3, config3);   

	///@todo
	/// NOTE- Nhits is computed here from the size of the udp packet, such that
	/// it will be smaller than 64K. This must also hold true for the PCI 
	/// bus transfers. Since Nhits is used in both plces, this logic will fail
	/// if the udo packet overhead is ever smaller than the PCI packet overhead.
	///
	/// The logic needs to be rewritten such that Nhits does not allow the PCI
	/// block size, nor the udp block size, to exceed 64K.
	int blocksize = sizeof(UDPHEADER)+
			sizeof(COMMAND) + 
			sizeof(INFOHEADER) + 
			config1->gatesa * 2 * sizeof(float);

	Nhits = 65536 / blocksize; 
	if	(Nhits % 2)	//	computed odd #hits
		Nhits--;	//	make it even
	///////////////////////////////////////////////////////////////////////////
	//
	//    Create piraqs. They all have to be created, so that all boards
	//    are found in succesion, even if we will not be collecting data 
	//    from all of them.

	piraq1 = new CP2PIRAQ(destIP, outport,   fname1, argv[1], Nhits);
	piraq2 = new CP2PIRAQ(destIP, outport+1, fname2, argv[1], Nhits);
	piraq3 = new CP2PIRAQ(destIP, outport+2, fname3, argv[1], Nhits);

	///////////////////////////////////////////////////////////////////////////
	//
	//     Calculate starting beam and pulse numbers. I think 
	//     that these are passed on to the piraqs

	float prt;
	prt = piraq3->prt();
	unsigned int pri; 
	pri = (unsigned int)((((float)COUNTFREQ)/(float)(1/prt)) + 0.5); 
	time_t now = time(&now);
	pulsenum = ((((__int64)(now+2)) * (__int64)COUNTFREQ) / pri) + 1; 
	beamnum = pulsenum / config1->hits;
	printf("pulsenum = %I64d\n", pulsenum); 
	printf("beamnum  =  %I64d\n", beamnum); 

	///////////////////////////////////////////////////////////////////////////
	//
	//      start the piraqs, waiting for each to indicate they are ready

	if (piraqs & 0x01) { // turn on slot 1
		if (piraq1->start(pulsenum, beamnum)) 
			exit(-1);
	} 
	if (piraqs & 0x02) { // turn on slot 2
		if (piraq2->start(pulsenum, beamnum)) 
			exit(-1);
	} 
	if (piraqs & 0x04) { // turn on slot 3
		if (piraq3->start(pulsenum, beamnum)) 
			exit(-1);
	} 

	///////////////////////////////////////////////////////////////////////////
	//
	// start timer board immediately

	PINFOHEADER info;
	info = piraq3->info();
//	start_timer_card(&ext_timer, &info);  
	cp2timer_config(&ext_timer,&info);
    cp2timer_set(&ext_timer);		// put the timer structure into the timer DPRAM 
    cp2timer_reset(&ext_timer);	// tell the timer to initialize with the values from DPRAM 
    cp2timer_start(&ext_timer);	// start timer 

	///////////////////////////////////////////////////////////////////////////
	//
	//      poll the piraqs in succesion

	while(1) {   // until 'q' 
		if (piraqs & 0x01) { // turn on slot 1
			if (piraq1->poll())
				continue;
		}
		if (piraqs & 0x02) { // turn on slot 1
			if (piraq2->poll())
				continue;
		}
		if (piraqs & 0x04) { // turn on slot 1
			if (piraq3->poll())
				continue;
		}

		if (kbhit()) {
			c = toupper(getch());

#ifdef TESTING_TIMESERIES_RANGE
			if(c == 'U') {
				pn_pkt->data.info.ts_start_gate += 1;
				pn_pkt->data.info.ts_end_gate += 1;
			}
			if(c == 'D') {
				pn_pkt->data.info.ts_start_gate -= 1;
				pn_pkt->data.info.ts_end_gate -= 1;
			}
#endif
			// !!!DO LATER: leave these here but do not use for now. 
			//				if(c == '+')		TimerStartCorrection += (__int64)ExactmSecperBeam; 
			//				if(c == '-')		TimerStartCorrection -= (__int64)ExactmSecperBeam; 
#ifndef TESTING_TIMESERIES
			//				if(c == '+')		TimerStartCorrection += (__int64)mSecperBeam; 
			//				if(c == '-')		TimerStartCorrection -= (__int64)mSecperBeam;
			if(c == 'A')		
			{ 
				PIRAQadjustAmplitude = 1; 
				PIRAQadjustFrequency = 0; 
				printf("'U','D','+','-' adjust test waveform amplitude\n"); 
			}
			if(c == 'W')		
			{ 
				PIRAQadjustAmplitude = 0; 
				PIRAQadjustFrequency = 1; 
				printf("'U','D','+','-' adjust test waveform frequency\n");
			}
			//	adjust test data freq. up/down fine/coarse
			if(PIRAQadjustFrequency)	{	//	use these keys to adjust the test sine frequency
				if(c == '+')		piraq1->SetCP2PIRAQTestAction(INCREMENT_TEST_SINUSIOD_FINE);	
				if(c == '-')		piraq1->SetCP2PIRAQTestAction(DECREMENT_TEST_SINUSIOD_FINE);
				if(c == 'U')		piraq1->SetCP2PIRAQTestAction(INCREMENT_TEST_SINUSIOD_COARSE);	 
				if(c == 'D')		piraq1->SetCP2PIRAQTestAction(DECREMENT_TEST_SINUSIOD_COARSE);	
			}
			//	adjust test data amplitude: up/down fine/coarse
			if(PIRAQadjustAmplitude)	{	//	use these keys to adjust the test sine amplitude
				if(c == '+')		piraq1->SetCP2PIRAQTestAction(INCREMENT_TEST_AMPLITUDE_FINE);	
				if(c == '-')		piraq1->SetCP2PIRAQTestAction(DECREMENT_TEST_AMPLITUDE_FINE);
				if(c == 'U')		piraq1->SetCP2PIRAQTestAction(INCREMENT_TEST_AMPLITUDE_COARSE);	 
				if(c == 'D')		piraq1->SetCP2PIRAQTestAction(DECREMENT_TEST_AMPLITUDE_COARSE);	
			}
#else
			// adjust test timeseries power: !note requires NOT TESTING_TIMESERIES_RANGE. 
			if(c == 'U')		test_ts_power *= sqrt(10.0); 
			if(c == 'D')		test_ts_power /= sqrt(10.0); // factor of arg in power
			// adjust test timeseries frequency: 
			if(c == '+')		test_ts_adjust += 2; 
			if(c == '-')		test_ts_adjust -= 2; 
#endif

			//	temporarily use keystrokes '0'-'8' to switch PIRAQ channel mode on 3 boards
			if((c >= '0') && (c <= '8'))	
			{	// '0'-'2' piraq1, etc.
				switch (c)
				{
				case 0://	send CHA
					piraq1->SetCP2PIRAQTestAction(SEND_CHA);	
					break;
				case 1://	send CHB
					piraq1->SetCP2PIRAQTestAction(SEND_CHB);	 
					break;
				case 2://	send combined data
					piraq1->SetCP2PIRAQTestAction(SEND_COMBINED);	
					break;
				case 3://	send CHA
					piraq2->SetCP2PIRAQTestAction(SEND_CHA);							
					break;
				case 4://	send CHB
					piraq2->SetCP2PIRAQTestAction(SEND_CHB);	 						
					break;
				case 5://	send combined data	
					piraq2->SetCP2PIRAQTestAction(SEND_COMBINED);						
					break;
				case 6://	send CHA
					piraq3->SetCP2PIRAQTestAction(SEND_CHA);							
					break;
				case 7://	send CHB 
					piraq3->SetCP2PIRAQTestAction(SEND_CHB);							
					break;
				case 8://	send combined data
					piraq3->SetCP2PIRAQTestAction(SEND_COMBINED);
					break;
				}
			}

			if(c == 'Q' || c == 27)   {
				printf("\nUser terminated:\n");	
				break;
			}
		} // end if (kbhit())  
	} // end	while(1)

	// exit NOW:
	printf("\npress a key to stop piraqs and exit\n"); while(!keyvalid()) ; 
	printf("piraqs stopped.\nexit.\n\n"); 

	// remove for lab testing: keep transmitter pulses active w/o go.exe running. 12-9-04
	cp2timer_stop(&ext_timer); 

	if (piraq1)
		delete piraq1; 
	if (piraq2)
		delete piraq2; 
	if (piraq3)
		delete piraq3;

	//		printf("TimerStartCorrection = %dmSec\n", TimerStartCorrection); 
	exit(0); 

}


