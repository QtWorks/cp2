//#include <Afx.h>

#include        <stdio.h>
#include        <string.h>
#include        <errno.h>
#include <stdio.h>
#include <conio.h>

#include "piraq.h"
#include "control.h"
#include "hpib.h"

#include "subs.h"
#include "config.h"
#include "timerlib.h"
#include "pci_w32.h"

#define STOPDELAY 30

#define REF     100e3

//void delay(int time); //!!!all calls replaced w/Sleep() w/no change to passed parameter

/*
program the timer for different modes with

	mode:           0 = continuous mode, 1 = trigger mode, 2 = sync mode
	gate0mode:      0 = regular gate 0, 1 = expanded gate 0
	gatecounts:     gate spacing in 10 MHz counts
	numbergates:    number of range gates
	timecounts:     modes 1 and 2: delay (in 10 MHz counts) after trigger
			before sampling gates
			mode 0: pulse repetition interval in 10 MHz counts

For sync mode (mode 0), delaycounts defines the sync time.  The prt
delay must be reprogrammed after the FIRST signal is detected.  

OTHER RESULTS:
	1)  the timing state machine is reset

*/

/* this routine leaves the timer in a known state */
/* the timer is assumed stopped on entry (important!) */
int timerset(CONFIG *config, PIRAQ *piraq)
   {
   int  S,D,N,i,clk,freq,glen,spare23;
   int  rcvr_pulsewidth; // local computed on 24/32 MHz timebase: config->rcvr_pulsewidth computed on 6/8 MHz

   rcvr_pulsewidth = config->rcvr_pulsewidth*4; 

// use local rcvr_pulsewidth instead of config->rcvr_pulsewidth; local (and code below) assumes 24/32 MHz timebase. 
   /* check for mode within bounds */
   if(config->timingmode < 0 || config->timingmode > 2)
	{printf("TIMERSET: invalid mode %d\n",config->timingmode); return(0);}
   
   /* check for gate0mode within bounds */
   if(config->gate0mode < 0 || config->gate0mode > 1)
	{printf("TIMERSET: invalid gate0mode %d\n",config->gate0mode); return(0);}

   /* check for receiver pulsewidth within bounds */
   /* the pulsewidth defines the FIFO clock rate in 32MHz counts (or 24MHz) */
   if(rcvr_pulsewidth == 0)
	{printf("TIMERSET: invalid receiver pulsewidth count: %d\n",rcvr_pulsewidth); return(0);}
   if(rcvr_pulsewidth >= 32 && rcvr_pulsewidth < 64 && (rcvr_pulsewidth & 1))
	{printf("TIMERSET: invalid receiver pulsewidth count: %d\n",rcvr_pulsewidth);
     printf("            If receiver pulse width greater than 32, it must be even\n",rcvr_pulsewidth);	 return(0);}
   if(rcvr_pulsewidth >= 64 && (rcvr_pulsewidth & 3))
	{printf("TIMERSET: invalid receiver pulsewidth count: %d\n",rcvr_pulsewidth);
     printf("            If pulse width greater than 64, it must be divisible by four\n",rcvr_pulsewidth);	 return(0);}
   if((config->gatesa * rcvr_pulsewidth)<12)
	{printf("TIMERSET: number of Channel A gates X receiver pulsewidth must be 12 or greater\n"); return(0);}
   if((config->gatesb * rcvr_pulsewidth)<12)
	{printf("TIMERSET: number of Channel B gates X receiver pulsewidth must be 12 or greater\n"); return(0);}

   /* check for numbergates within bounds */
   if(config->gatesa == 0)
	{printf("TIMERSET: invalid number of gates for channel A: %d\n",config->gatesa); return(0);}

   /* check for numbergates within bounds */
   if(config->gatesb == 0)
	{printf("TIMERSET: invalid number of gates for channel B: %d\n",config->gatesb); return(0);}

   /* check to see that the sampling interval can be measured in 8 MHz counts */
   if((config->gatesa * rcvr_pulsewidth) & 3)
    {printf("TIMERSET: Invalid number of gates/receiver pulsewidth combination %d %d for Channel A\n",config->gatesa,rcvr_pulsewidth);
     printf("            number of gates X pulsewidth should be a multiple of 4\n");
     return(0);}
    
   if((config->gatesb * rcvr_pulsewidth) & 3)
    {printf("TIMERSET: Invalid number of gates/receiver pulsewidth combination %d %d for Channel B\n",config->gatesb,rcvr_pulsewidth);
     printf("            number of gates X pulsewidth should be a multiple of 4\n");
     return(0);}

   /* check for sync timecounts within bounds */
   if((config->sync == 0 || config->sync > 65535) && config->timingmode != 2)
	{printf("TIMERSET: invalid sync delay %d\n",config->sync); return(0);}

   /* check for delay timecounts within bounds */
   if((config->delay == 0 || config->delay > 65535) && config->timingmode != 2)
	{printf("TIMERSET: invalid delay %d\n",config->delay); return(0);}

   /* check for tpdelay timecounts within bounds */
   if(config->tpdelay >= 65536 || config->tpdelay <= -65536)
       {printf("TIMERSET: invalid testpulse delay %d\n",config->tpdelay); return(0);}

   /* check for prt timecounts within bounds */
   if(config->prt == 0 || config->prt > 65535)
	{printf("TIMERSET: invalid prt %d\n",config->prt); return(0);}

   /* check for prt2 timecounts within bounds */
   if(config->prt2 == 0 || config->prt2 > 65535)
	{printf("TIMERSET: invalid prt %d\n",config->prt2); return(0);}

   /* make sure prt is even multiple of .5uS */
//   if(config->prt % 10 || config->prt2 % 10)
//        {printf("TIMERSET: prt and prt2 must be a multiple of 10\n"); return(0);}

   /* double check that all the gates and the delay fit into a prt */
   if(config->prt <= rcvr_pulsewidth * (config->gatesa + 1) / 4.0)
	{
	printf("TIMERSET: Gates, Delay, and EOF do not fit in PRT of %8.2f uS\n",config->prt/10.0); 
	printf("          Delay is %8.2f uS (%d 10 MHz counts)\n",config->delay/10.0,config->delay);
	printf("          %d gates at %8.2f uS (%7.1f m) + EOF = %8.2f uS\n"
	,config->gatesa
	,rcvr_pulsewidth/10.0
	,1e-6*C*.5*rcvr_pulsewidth/10.0
	,(config->gatesa + 1) * rcvr_pulsewidth / 10.0);
	return(0);
	}

   /* stop the timer */
   //STATCLR0(piraq,STAT0_TRESET |STAT0_TMODE);
   //piraq->GetControl()->UnSetBit_StatusRegister0((STAT0_TRESET | STAT0_TMODE));
	piraq->GetControl()->StopTimers();
   

   //STATCLR1(piraq,STAT1_EXTTRIGEN);
	piraq->GetControl()->UnSetBit_StatusRegister1(STAT1_EXTTRIGEN);	

   Sleep(STOPDELAY);       /* wait for all timers to time-out */

   /* program all the timers */
	//piraq->GetControl()->InitTimers();
   /* the three timers of chip 0 */
   switch(config->timingmode)
      {
      case 0:   /* software free running mode */
		  // !!!
printf("TIMERSET: config->prt = %d config->prt2 = %d rcvr_pulsewidth = %d\n",config->prt, config->prt2, rcvr_pulsewidth); 
           timer(0,5,config->prt-2                    ,piraq->timer);   /* even - odd prt */
           timer(1,5,config->prt2-2   ,piraq->timer); /* even prt (1) */

printf("tpdelay = %d\n***********************************************************\n",config->tpdelay);
     if(config->tpdelay >= 0) /* handle the zero case here */
	      {
	      if(config->tpdelay > 0)
                 timer(2,5,config->tpdelay,piraq->timer); /* delay from gate0 to tp */
	      else
                 timer(2,5,1,piraq->timer); /* delay from gate0 to tp */
	      }
	   else
	      {
              timer(2,5,-config->tpdelay,piraq->timer); /* delay from tp to gate0 */
	      }
	   break;
      case 1:   /* external trigger mode */
        timer(0,5,config->prt-2,piraq->timer);   /* even - odd prt */
	if(config->tpdelay >= 0)        /* handle the 0 case here */
	   {
           timer(1,5,config->delay  ,piraq->timer); /* delay from trig to gate0 */
	   if(config->tpdelay == 0)
              timer(2,5,config->tpdelay+1,piraq->timer); /* delay from gate0 to tp */
	   else
              timer(2,5,config->tpdelay  ,piraq->timer); /* delay from gate0 to tp */
	   }
	else   /* if tp comes before gate0 */
	   {
	   if(config->delay + config->tpdelay > 0)
	      {
              timer(1,5,config->delay + config->tpdelay,piraq->timer); /* delay from trig to tp */
              timer(2,5,-config->tpdelay,piraq->timer); /* delay from tp to gate0 */
	      }
	   else
	      {
	      if(config->delay > 1)
		 {
                 timer(1,5,1,piraq->timer); /* delay from trig to tp */
                 timer(2,5,config->delay-1,piraq->timer); /* delay from tp to gate0 */
		 }
	      else
		 {
                 timer(1,5,1,piraq->timer); /* delay from trig to tp */
                 timer(2,5,1,piraq->timer); /* delay from tp to gate0 */
		 }
	      }
	   }
	   break;
      case 2:   /* external sync free running mode */
           timer(0,5,config->prt-2,piraq->timer);   /* even - odd prt */
           timer(1,5,config->sync,piraq->timer); /* even prt (1) */
	   if(config->tpdelay >= 0) /* handle the zero case here */
	      {
	      if(config->tpdelay > 0)
                 timer(2,5,config->tpdelay,piraq->timer); /* delay from gate0 to tp */
	      else
                 timer(2,5,1,piraq->timer); /* delay from gate0 to tp */
	      }
	   else
	      {
              timer(2,5,-config->tpdelay,piraq->timer); /* delay from tp to gate0 */
	      }
	   break;
      }

   /* the three timers of chip 1 */
   timer(0,1,config->tpwidth,piraq->timer + 8);   /* test pulse width */
   timer(1,5,(config->gatesa * rcvr_pulsewidth)/4-2 ,piraq->timer + 8);   /* number of A gates */
   timer(2,5,(config->gatesb * rcvr_pulsewidth)/4-2 ,piraq->timer + 8);   /* number of B gates */

   /* gate length control */
   /* depending on the gate length, the FIR is either in /1 , /2 or /4 mode */
//!!!   if(config->pulsewidth < 32) 
   if(rcvr_pulsewidth < SYSTEM_CLOCK/2.0e6) // < 1uSec: divisor timebase counts/uSec
      {
      glen = ((rcvr_pulsewidth & 0x1F)-1) << 9; //???how to compute this? before bound was 
	  // < 32, same as mask 0x1F, but if SYSTEM_CLOCK/2.0e6 = 24, this seems like it would be 
	  // a problem ... mp 12-20-02. 
      spare23 = 0x000;
      }
//!!!   else if(config->pulsewidth < 64) 
   else if(rcvr_pulsewidth < 2*(SYSTEM_CLOCK/2.0e6)) // < 2uSec 
      {
      glen = ((rcvr_pulsewidth & 0x3E)-2) << 8;
      spare23 = 0x400;
      }
   else
      {
      glen = ((rcvr_pulsewidth & 0x7C)-4) << 7;
      spare23 = 0x800;
      }
printf("glen = %d rcvr_pulsewidth = %d\n",glen,rcvr_pulsewidth);
 
   /* set up all PIRAQ registers */ //???
   piraq->GetControl()->SetValue_StatusRegister0(
     glen | (config->testpulse << 6)   |	 /* odd and even testpulse bits */
	 (config->trigger << 4)              |  /* odd and even trigger bits */
	 ((config->timingmode == 1) ? STAT0_TMODE : 0) |  /* tmode timing mode */
//	 ((config->timingmode == 2) ? STAT0_TRIGSEL : 0) |  /* if sync mode, then choose 1PPS input */
//         ((config->gate0mode == 1) ? (STAT0_GATE0MODE /* | STAT0_SWCTRL */) : 0) |
//	 ((config->gate0mode == 1) ? (STAT0_GATE0MODE | STAT0_SWCTRL) : 0) |
	 ((config->tpdelay >= 0) ? STAT0_DELAY_SIGN : 0) | (STAT0_SW_RESET) // !TURN RESET OFF! mp 10-18-02 see singlepiraq ongoing
//         STAT0_SWSEL | STAT0_SWCTRL       /* set to channel B (top channel) */
	 );   /* start from scratch */

   //STATUS1(piraq,spare23);
   piraq->GetControl()->SetValue_StatusRegister1(spare23);

   pll(10e6,SYSTEM_CLOCK,50e3,piraq);
   Sleep(STOPDELAY);   //	??I,Q unequal amplitude on first powerup: 6-22-06 mp
//!!!note this is from orphan code\SUBS_gaussian.c, from QNX. the pll was programmed w/
//   pll(10e6,config->stalofreq,REF,piraq);

//   FIR_gaussian(config, piraq, 0.734, config->pulsewidth * 1e-6 / 32.0); translates to: 
// which is temporarily resident in this module

   FIR_gaussian(config, piraq, 1.04/*0.734*/, config->rcvr_pulsewidth * 1e-6 / (SYSTEM_CLOCK/8.0e6)); // !use config->... here
   // !investigate I,Q asymmetry in diagnostic timeseries: 2-12-04
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!   is this the right value to pgm FIR filter width!!!!!!
   return(1);
}

void timer(int timernum,int timermode,int count,unsigned short *iobase)
   {
   if(timernum < 0 || timernum > 2)
	{printf("TIMER: invalid timernum %d\n",timernum); return;}
   if(timermode < 0 || timermode > 5)
	{printf("TIMER: invalid timermode %d\n",timermode); return;}

Sleep(1);
   *((volatile short *)iobase +     3   ) = (0x30 + timernum * 0x40 + timermode * 2);
Sleep(1);
   *((volatile short *)iobase + timernum) = count;
Sleep(1);
   *((volatile short *)iobase + timernum) = (count >> 8);
   }

/* make sure the timer is stopped and no fifo data is pending */
void    stop_piraq(CONFIG *config,PIRAQ *piraq)
   {
   int  i;

   /* stop the timer */
   //STATCLR0(piraq,STAT0_TRESET | STAT0_TMODE);
	piraq->GetControl()->UnSetBit_StatusRegister0(STAT0_TRESET | STAT0_TMODE);	

   /* make sure that all possible timer time-outs occur */
   Sleep(STOPDELAY);    /* wait some number of milliseconds */
   
   piraq->StopDsp();
   }



// set up the piraq structure for the nth board from the config structure 
// also set a few of the piraq locations so that filldwell won't 
// go nuts 
// returns 1 if the card is found, returns 0 if not 
void    setpiraqptr(PIRAQ *piraq, int n)
   {
   unsigned int base_add,error;
   unsigned char *base;
   int bus,fnum;
   int i,j;
   
PCI_CARD *pcicard;

   if(!(pcicard=find_pci_card(PIRAQ_DEVICE_ID,PIRAQ_VENDOR_ID,n))) exit(0);

   base_add =  pci_card_ioregbase(pcicard);

//   out32(base_add + 0x00, 0xFFFFE000); /* fix the configuration memory (range word) */
//printf("did try to patch the configuration memory problem\n");

//   out32(base_add + 0x18, 0x41410341); /* fix the configuration memory (range word) */
//printf("did try to patch the configuration memory problem\n");

   out32(base_add + 0x08, 0x01210008); /* fix the configuration memory (MARBR) */
printf("did try to patch the configuration memory problem\n");

   out32(base_add + 0x20, 0x00000000); /* fix the configuration memory (DMLBAM) */
//   out32(base_add + 0x24, 0x0); /* fix the configuration memory (DMLBAI) */
printf("did try to patch the configuration memory problem\n");

   out32(base_add + 0x1C, 0xFFF00000); /* fix the configuration memory (DMRR) -- temporarily uses only 1M space */
   out32(base_add + 0x28, 0x030001C1); /* fix the configuration memory (DMPBAM) */
printf("did try to patch the configuration memory problem\n"); /* one megabyte mapped at 48MEG */

//pci_read_config32(bus,fnum,0x00,1,(char*)(&error));  /* read 32 bit value at requested register */
printf("data at PCI register 00 = %08lX\n",error);

//pci_read_config32(bus,fnum,0x04,1,(char*)(&error));  /* read 32 bit value at requested register */
printf("data at PCI register 04 = %08lX\n",error);

if(1)
   {
   printf("PCI Configuration registers\n");
   for(j=0; j<8; j++)
     {
     printf("%02X ",4*4*j);
     for(i=0; i<4; i++)
        printf("%08lX ",in32(base_add + 4*(i+4*j)));
     printf("\n");
     }
   }


   /* now look for the memory addresses of the various memory areas */
   //base = pci_card_membase(pcicard, 0x18);

//   if(error == ENOMEM) printf("ENOMEM\n");
//   if(error == ENXIO) printf("ENXIO\n");
//   if(error == EINVAL) printf("EINVAL\n");
//   reg_base = error;

//   reg_base = pci(0x18,n) & ;
//   error = mmap_device_io( 65536, reg_base);
//   if(error == ENOMEM) printf("ENOMEM\n");
//   if(error == ENXIO) printf("ENXIO\n");
//   reg_base = error;

//printf("A\n");

//   base = (unsigned char *)allocate_physical_memory(reg_base,0x1810);

//printf("base = %08X\n",(int)base);

//printf("B\n");
/*
   piraq->boardnumber = n;
   piraq->FIRCH1_I = (unsigned short *)(base + 0);
   piraq->FIRCH1_Q = (unsigned short *)(base + 0x400);
   piraq->FIRCH2_I = (unsigned short *)(base + 0x800);
   piraq->FIRCH2_Q = (unsigned short *)(base + 0xC00);
   piraq->timer    = (unsigned short *)(base + 0x1000);
   piraq->status0  = (unsigned short *)(base + 0x1400);
   piraq->status1  = (unsigned short *)(base + 0x1410);
   piraq->HPICLO   = (unsigned short *)(base + 0x1800);
   piraq->HPICHI   = (unsigned short *)(base + 0x1802);
   piraq->HPIALO   = (unsigned short *)(base + 0x1804);
   piraq->HPIAHI   = (unsigned short *)(base + 0x1806);
   piraq->HPIDLO   = (unsigned short *)(base + 0x180C);
   piraq->HPIDHI   = (unsigned short *)(base + 0x180E);
*/
//printf("setpiraqptr 7\n");

   /* put reasonable data in a few of the registers so that */
   /* other programs won't go nuts if by chance they try to use */
   /* those data before the PIRAQ itself updates them */
   //piraq->numdiscrim = 0.0;
   //piraq->dendiscrim = 1.0;
   //piraq->imask = 0;

   /* put the DSP into a known state where HPI reads/writes will work */
//   STATCLR0(STAT0_SW_RESET);   /* puts DSP into reset */
   Sleep(1);
//   STATSET0(STAT0_SW_RESET);   /* takes DSP out of reset */
   Sleep(1);
   }

/* program the phase locked loop */
void pll(double ref, double freq, double cmpfreq, PIRAQ *piraq)
   {
   int  i,data,r,n,a;

   /* initialize and configure the pll at least once */
   /* this needs to be done every time, following the convoluted QNX code, and experimental results! ... here simplified */
   plldata(0x10FA93,piraq);     /* r data with reset bit */

   /* compute the data to go into the register */
   n = 0.5 + freq / cmpfreq;
   a = n & 7;
   n /= 8;
   r = 0.5 + ref / cmpfreq;

   plldata(r << 2 | 0x100000,piraq);     /* r data with reset bit */
   plldata(n << 7 | a << 2 | 0x100001,piraq);     /* r data with reset bit */
   Sleep(1); // ape delay(1); on QNX box
   }
   
/* output a stream of serial data to the PLL */
void plldata(int data, PIRAQ *piraq)
   {
   int  i;
   /*----------------------*/
   /* PLL bits definition  */
   /*----------------------*/
   /*  2   |   1   |   0   */
   /*------+-------+-------*/
   /* DATA |  LE   | CLOCK */
   /*----------------------*/

    int Status1; 
	for(i=0; i<21; i++,data<<=1) {
		if (data & 0x100000) { // data bit set 
			piraq->GetControl()->SetBit_StatusRegister1(STAT1_PLL_DATA);   // set PLL data bit 
			piraq->GetControl()->SetBit_StatusRegister1(STAT1_PLL_CLOCK); // set PLL data bit, set clock  
		} 
		else { // data bit clear 
			piraq->GetControl()->UnSetBit_StatusRegister1(STAT1_PLL_DATA); // clear PLL data bit
			piraq->GetControl()->SetBit_StatusRegister1(STAT1_PLL_CLOCK);  // set clock
		} 
        piraq->GetControl()->UnSetBit_StatusRegister1(STAT1_PLL_CLOCK);    // clear PLL clock bit
	} 
   /* toggle LE signal */
    piraq->GetControl()->SetBit_StatusRegister1(STAT1_PLL_LE);   // set LE
    piraq->GetControl()->UnSetBit_StatusRegister1(STAT1_PLL_LE); // clear LE
}

/* wait forever or until a Q is hit */
/* return 1 if successful, 0 if fail */
/* on exit, piraq->beamnum corresponds to the beam */
/* we will start working on next */
int  pwait(CONFIG *config,PIRAQ *piraq) /* 1 if successful 0 if timeout */
   {
   char         c;
   time_t       first;

   first = time(NULL) + 3;   /* wait 3 seconds */
   
   c = 0;

   while(piraq->oldbeamnum >= piraq->beamnum) 
      if(time(NULL) > first)
	 if(kbhit())
	    if(toupper(c = getchar()) == 'Q')
	       {
	       printf("Q was hit before FLAG changed state\n");
	       Sleep(500);
	       break;
	       }
   piraq->oldbeamnum++;

   /* re-sync if piraq falls out of sync */
   if(piraq->eofstatus)
      {
      stop_piraq(config,piraq);
      printf("wait: EOF out of sync, restarting (EOF = %08X)\n",piraq->eofstatus);
//      beeppp();
      timerset(config,piraq);
      piraq->eofstatus = 0;
return(0);
//      if(!start(config,piraq))        {exit(-1);}
      }
   
   /* toggle the watch dog */
   STATTOG1(piraq,STAT1_WATCHDOG);
//temp = *piraq->status1;      
//temp ^= STAT1_WATCHDOG;
//*piraq->status1 = temp;

   return(toupper(c) != 'Q');   /* return false if a Q was hit */
   }


/* hitcount is defined as zero at unix time 0 */
/* pulsenum is defined as zero at unix time 0 */
/* beamnum is defined as zero at unix time 0 */
/* config->sync is the difference between these assumptions and reality */
/* and is just added in at the last step */

/* return the pulsenumber for the next pulse following the current second tick */
/* this assumes that pulse zero started at exactly unix time zero */
UINT64 calc_pulsenum(int seconds,int  pri)
   {
   return(seconds * COUNTFREQ / pri + 1);
   }

/* Return the dwellcount for the next pulse following the current second tick */
/* This is used to syncronize remote processors to the dwell interval */
/* hitcount is defined as zero at unix time 0 */
UINT64 calc_dwellcount(int seconds, int  pri, int hits)
   {
   return(unsigned long(calc_pulsenum(seconds,pri)) / hits);
   }

/* Return the hitcount (mod hits) for the next pulse following the current second tick */
int calc_hitcount(int seconds, int  pri, int hits)
   {
   return(unsigned long(calc_pulsenum(seconds,pri)) % hits);
   }

/* return the delay (in COUNTFREQ counts) between the given second and the next pulse */
int     calc_delay(int seconds,int  pri)
   {
   return(pri - ((seconds * COUNTFREQ) % pri));
   }


/* compute the time corresponding to the first pulse in the dwell */
/* piraq flag represents the dwell count of the beam just read */
/* to compute pulsenum, you have to get the long long upper bits right */
void computetime(CONFIG *config, UINT64 beamnum,unsigned int *time,unsigned short *subsec,UINT64 *pulsenum)
   {
   UINT64  temp;
   
   /* compute the pulsenumber of the first hit in the dwell */
   *pulsenum = beamnum * config->hits;

   temp = *pulsenum * (config->prt + config->prt2) / 2;
   *time = temp / COUNTFREQ;
   *subsec = (10000 * (temp % COUNTFREQ)) / COUNTFREQ;

#ifdef fuck   
   /* compute the pulsenumber of the first hit in the dwell */
   *pulsenum = (beamnum - 1) * config->hits;

   temp = *pulsenum * config->prt;
   *time = temp / FREQ;
   *subsec = (10000 * (temp % FREQ)) / FREQ;
#endif   
   }

#define HWOB    0x0001
//#define DSPINT  0x0002
//#define HINT    0x0004
#define HRDY    0x0008
#define FETCH   0x0010

#define LO      shrt[0]
#define HI      shrt[1]

typedef union   {
                unsigned int        lohi;
                unsigned short  shrt[2];
                } SHORT2LONG;

int FIRSTHPI = 1;

/* Read values in DSP memory space through the HPI */
/* The address is the 32 bit DSP relative address */
/* This routine does not do autoincrement */
int hpird(PIRAQ *piraq, int address)
   {
//   SHORT2LONG   data;

//   data.lohi = address;

   /* init HWOB = 1 and HPIA */
//if(FIRSTHPI)
//   {
//   FIRSTHPI = 0;
//   *piraq->HPICLO = HWOB;       /* write HPIC 1st halfword (with HHWIL low) */
//   *piraq->HPICHI = HWOB;       /* write HPIC 2nd halfword (with HHWIL high) */
//   }

//   *piraq->HPIALO = data.LO;    /* write HPIC 2nd halfword (with HHWIL low) */
//   *piraq->HPIAHI = data.HI;    /* write HPIC 2nd halfword (with HHWIL high) */

//   while(!(*piraq->HPICLO & HRDY)) printf("not ready\n");

   /* init HWOB = 1 and HPIA */
//   data.LO = *piraq->HPIDLO;
//   data.HI = *piraq->HPIDHI;

   //return(data.lohi);
	return(piraq->GetHPIB()->ReadLocation(address));
   }

/* Write values in DSP memory space through the HPI */
/* The address is the 32 bit DSP relative address */
/* This routine does not do autoincrement */
void hpiwrt(PIRAQ *piraq, int address, int tdata)
   {
   //SHORT2LONG   data;

   //data.lohi = address;

   /* init HWOB = 1 and HPIA */
//if(FIRSTHPI)
//   {
//   FIRSTHPI = 0;
//   *piraq->HPICLO = HWOB;       /* write HPIC 1st halfword (with HHWIL low) */
//   *piraq->HPICHI = HWOB;       /* write HPIC 2nd halfword (with HHWIL high) */
//   }

//   *piraq->HPIALO = data.LO;    /* write HPIC 2nd halfword (with HHWIL low) */
//   *piraq->HPIAHI = data.HI;    /* write HPIC 2nd halfword (with HHWIL high) */

//   data.lohi = tdata;

//   *piraq->HPIDLO = data.LO;
//   *piraq->HPIDHI = data.HI;
	piraq->GetHPIB()->WriteLocation(address,tdata);
}
   
/********************************************************************************************

QNX FIR_gaussian and supporting procedures ported from QNX. mp 12-20-02

*********************************************************************************************/

unsigned short	FIR_cfg[][13] = {
   0x24D8,0x6108,0x04DC,0x6108,0x2000,0x0000,0x0030,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000,  /* full rate odd */
   0x24D8,0x6181,0x04DC,0x6181,0x2000,0x0000,0x0030,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000,  /* full rate even */

   0x248B,0x2E12,0x048F,0x2E12,0x2000,0x0000,0x0030,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, /* half rate odd */
   0x248B,0x2E91,0x048F,0x2E91,0x2000,0x0000,0x0030,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, /* half rate even */
// swap above:
//   0x248B,0x2E91,0x048F,0x2E91,0x2000,0x0000,0x0030,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, /* half rate odd */
//   0x248B,0x2E12,0x048F,0x2E12,0x2000,0x0000,0x0030,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, /* half rate even */

   0x2402,0x0292,0x0406,0x0292,0x2000,0x0000,0x0030,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000,  /* quarter rate odd */
   0x2402,0x0214,0x0406,0x0214,0x2000,0x0000,0x0030,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000   /* quarter rate even */

//   0x24D8,0x6108,0x04DC,0x6108,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000,  /* full rate odd */
//   0x24D8,0x6181,0x04DC,0x6181,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000,  /* full rate even */

//   0x248B,0x2E12,0x048F,0x2E12,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, /* half rate odd */
//   0x248B,0x2E91,0x048F,0x2E91,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, /* half rate even */

//   0x2402,0x0214,0x0406,0x0214,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, /* quarter rate odd */
//   0x2402,0x0292,0x0406,0x0292,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000  /* quarter rate even */
   };

//   FIR_gaussian(config, piraq, 0.734, config->pulsewidth * 1e-6 / 32.0); translates to: 
//   FIR_gaussian(config, piraq, 0.734, config->pulsewidth * 1e-6 / (SYSTEM_CLOCK/2.0e6));

/* the tap coefficients are the lowest 14 bits of the 16 bit tap coefficient registers */
#define	SCALE		8191.0
#define	COEFF(a)	(short)(SCALE * exp(-pow((a)*timestep/sigma,2.0)/2.0))
// Coy's method: Calculate Coefficient
//	short shValue = (short)((SCALE * exp( -pow(((63-i)*dTimeStep),2.0) / (2*2*dSigma*dSigma) )) + .5);
/* base + 0 - 12  are the */
void FIR_gaussian(CONFIG *config, PIRAQ *piraq, float tbw, float pw)
   {
   int	i,p,sum=0,s,f;
   double	bw,sigma,timestep;
   
   bw = tbw / (2.0*pw);		/* compute the bandwidth */

   sigma = sqrt(log(2.0) / pow(2.0 * M_PI * bw,2.0));

//   timestep = 1.0 / piraq->pllfreq;	/* "64" Mhz */
//!!!   timestep = 1.0 / 64.0e6;
   timestep = 1.0 / SYSTEM_CLOCK;

//printf("bw = %e,  sigma = %e,  timestep = %e\n",bw,sigma,timestep);

//   for(i=128; i<256; i++)
//      piraq->FIRCH1_I[i] = 
//      piraq->FIRCH1_Q[i] = 
//      piraq->FIRCH2_I[i] = 
//      piraq->FIRCH2_Q[i] = 0;

   /* FIR chip initialization */
   /* depending on the gate length, the FIR is either in /1 , /2 or /4 mode */
//!!!   if(config->pulsewidth < 32)			/* full rate mode */
   if(config->rcvr_pulsewidth < SYSTEM_CLOCK/8.0e6) // < 1uSec: divisor 8MHz/6MHz timebase counts/uSec
   {                                                //  "full rate" mode: no output decimation, per GC2011A documentation 
printf("< 1uSec: full rate mode: no output decimation\n"); 
	   p = 0;		/* determine which set of control register values to select */
//      timestep = 1.0 / piraq->pllfreq;	/* "64" Mhz */
      for(i=0; i<32; i++)	/* full rate. Number of taps = 32 (symmetric) */
         if(i&1)	/* if odd */
            {
            piraq->FIRCH1_I[128 + 2 * i - 1] = 
            piraq->FIRCH1_I[192 + 2 * i - 1] =     /* both the A and B paths get same coefficients */
            piraq->FIRCH2_I[128 + 2 * i - 1] = 		/* and both channel 1 and channel 2 get same settings */
            piraq->FIRCH2_I[192 + 2 * i - 1] = COEFF(31-i);
#ifdef NCAR_DRX 
//			sum += COEFF(31-i);
#endif
printf("i = %d FIRCH1_Iindex[128 + 2 * i - 1] = %d FIRCH1_Iindex[192 + 2 * i - 1] = %d COEFF(31-i) = %d\n",i,(128 + 2 * i - 1),(192 + 2 * i - 1),COEFF(31-i)); 
            }
         else	/* if even */
            {
            piraq->FIRCH1_Q[128 + 2 * i + 1] = 
            piraq->FIRCH1_Q[192 + 2 * i + 1] = 
            piraq->FIRCH2_Q[128 + 2 * i + 1] = 
            piraq->FIRCH2_Q[192 + 2 * i + 1] = COEFF(31-i);
printf("i = %d FIRCH1_Qindex[128 + 2 * i + 1] = %d FIRCH1_Qindex[192 + 2 * i + 1] = %d COEFF(31-i) = %d\n",i,(128 + 2 * i + 1),(192 + 2 * i + 1),COEFF(31-i)); 
			sum += COEFF(31-i);
            }
      sum *= 2;
      }
//!!!   else if(config->pulsewidth < 64)	/* half rate mode */
   else if(config->rcvr_pulsewidth < 2.0*(SYSTEM_CLOCK/8.0e6)) // < 2uSec 
   {                                                           // < 2uSec: decimate output by 2, per GC2011A documentation
printf("< 2uSec: decimate output by 2\n"); 
      p = 2;		/* determine which set of control register values to select */
//      timestep = 2.0 / piraq->pllfreq;   /* "32" MHz */
      for(i=0; i<64; i++)	/* half rate. Number of taps = 64 (symmetric) */
         switch(i%4)
            {
#if 1 // old method
            case 0:
            piraq->FIRCH1_Q[128 + i + 0] = 
            piraq->FIRCH1_Q[192 + i + 0] = 
            piraq->FIRCH2_Q[128 + i + 0] = 
            piraq->FIRCH2_Q[192 + i + 0] = COEFF(63-i);
//printf("i = %d FIRCH1_Qindex[128 + i + 0] = %d FIRCH1_Qindex[192 + i + 0] = %d COEFF(63-i) = 0x%x\n",i,(128 + i + 0),(192 + i + 0),COEFF(63-i)); 
//printf("%d  %f\n",i,COEFF(63-i));
			sum += COEFF(63-i);
            break;

            case 1:
            piraq->FIRCH1_I[128 + i - 1] = 
            piraq->FIRCH1_I[192 + i - 1] = 
            piraq->FIRCH2_I[128 + i - 1] = 
            piraq->FIRCH2_I[192 + i - 1] = COEFF(63-i);
#ifdef NCAR_DRX 
//			sum += COEFF(63-i);
#endif
//printf("i = %d FIRCH1_Iindex[128 + i - 1] = %d FIRCH1_Iindex[192 + i - 1] = %d COEFF(63-i) = 0x%x\n",i,(128 + i - 1),(192 + i - 1),COEFF(63-i)); 
            break;

            case 2:
            piraq->FIRCH1_Q[128 + i - 1] = 
            piraq->FIRCH1_Q[192 + i - 1] = 
            piraq->FIRCH2_Q[128 + i - 1] = 
            piraq->FIRCH2_Q[192 + i - 1] = COEFF(63-i);
//printf("i = %d FIRCH1_Qindex[128 + i - 1] = %d FIRCH1_Qindex[192 + i - 1] = %d COEFF(63-i) = 0x%x\n",i,(128 + i - 1),(192 + i - 1),COEFF(63-i)); 
			sum += COEFF(63-i);
            break;

            case 3:
            piraq->FIRCH1_I[128 + i - 2] = 
            piraq->FIRCH1_I[192 + i - 2] = 
            piraq->FIRCH2_I[128 + i - 2] = 
            piraq->FIRCH2_I[192 + i - 2] = COEFF(63-i);
#ifdef NCAR_DRX 
//			sum += COEFF(63-i);
#endif
//printf("i = %d FIRCH1_Iindex[128 + i - 2] = %d FIRCH1_Iindex[192 + i - 2] = %d COEFF(63-i) = 0x%x\n",i,(128 + i - 2),(192 + i - 2),COEFF(63-i)); 
            break;
#else
            case 0:
            piraq->FIRCH1_Q[128 + i + 0] = 
            piraq->FIRCH1_Q[192 + i + 0] = 
            piraq->FIRCH2_Q[128 + i + 0] = 
            piraq->FIRCH2_Q[192 + i + 0] = COEFF(63-i);
//printf("i = %d FIRCH1_Qindex[128 + i + 0] = %d FIRCH1_Qindex[192 + i + 0] = %d COEFF(63-i) = 0x%x\n",i,(128 + i + 0),(192 + i + 0),COEFF(63-i)); 
//printf("%d  %f\n",i,COEFF(63-i));
			sum += COEFF(63-i);
            break;

            case 1:
            piraq->FIRCH1_Q[128 + i - 1] = 
            piraq->FIRCH1_Q[192 + i - 1] = 
            piraq->FIRCH2_Q[128 + i - 1] = 
            piraq->FIRCH2_Q[192 + i - 1] = COEFF(63-i);
#ifdef NCAR_DRX 
//			sum += COEFF(63-i);
#endif
//printf("i = %d FIRCH1_Iindex[128 + i - 1] = %d FIRCH1_Iindex[192 + i - 1] = %d COEFF(63-i) = 0x%x\n",i,(128 + i - 1),(192 + i - 1),COEFF(63-i)); 
            break;

            case 2:
            piraq->FIRCH1_I[128 + i - 1] = 
            piraq->FIRCH1_I[192 + i - 1] = 
            piraq->FIRCH2_I[128 + i - 1] = 
            piraq->FIRCH2_I[192 + i - 1] = COEFF(63-i);
//printf("i = %d FIRCH1_Qindex[128 + i - 1] = %d FIRCH1_Qindex[192 + i - 1] = %d COEFF(63-i) = 0x%x\n",i,(128 + i - 1),(192 + i - 1),COEFF(63-i)); 
			sum += COEFF(63-i);
            break;

            case 3:
            piraq->FIRCH1_I[128 + i - 2] = 
            piraq->FIRCH1_I[192 + i - 2] = 
            piraq->FIRCH2_I[128 + i - 2] = 
            piraq->FIRCH2_I[192 + i - 2] = COEFF(63-i);
#ifdef NCAR_DRX 
//			sum += COEFF(63-i);
#endif
//printf("i = %d FIRCH1_Iindex[128 + i - 2] = %d FIRCH1_Iindex[192 + i - 2] = %d COEFF(63-i) = 0x%x\n",i,(128 + i - 2),(192 + i - 2),COEFF(63-i)); 
            break;
#endif
            }
      sum *= 2;
      }
   else										/* "quarter rate mode" */
      {                                     // < 2uSec: decimate output by 4, per GC2011A documentation
printf("> 2uSec: decimate output by 4\n"); 
      p = 4;		/* determine which set of control register values to select */
//      timestep = 4.0 / piraq->pllfreq;  /* "16" Mhz */
      for(i=0; i<128; i++)	/* quarter rate. Number of taps = 128 (symmetric) */
          // old method, w/configs swapped above
		  if(i&1)	/* if odd */
            {
            piraq->FIRCH1_Q[128 + (i - 1) / 2] = 
            piraq->FIRCH1_Q[192 + (i - 1) / 2] = 
            piraq->FIRCH2_Q[128 + (i - 1) / 2] = 
            piraq->FIRCH2_Q[192 + (i - 1) / 2] = COEFF(127-i);
#ifdef NCAR_DRX 
//			sum += COEFF(127-i);
#endif
            }
         else	/* if even */
            {
            piraq->FIRCH1_I[128 + i / 2] = 
            piraq->FIRCH1_I[192 + i / 2] = 
            piraq->FIRCH2_I[128 + i / 2] = 
            piraq->FIRCH2_I[192 + i / 2] = COEFF(127-i);
			sum += COEFF(127-i);
            }
		 sum *= 2;
      }

   for(i=0; i<13; i++)	*(piraq->FIRCH1_Q + i)  = FIR_cfg[p][i];	/* odd */
   for(i=0; i<13; i++)	*(piraq->FIRCH1_I + i) = FIR_cfg[p+1][i];	/* even */
   for(i=0; i<13; i++)	*(piraq->FIRCH2_Q + i)  = FIR_cfg[p][i];	/* odd */
   for(i=0; i<13; i++)	*(piraq->FIRCH2_I + i) = FIR_cfg[p+1][i];	/* even */

   /* figure out the gain based on the sum of the coefficients */
   /* If 2^11 is input to the AI channel, it gets multiplied by 'sum' into a 32 bit accumulator */
   /* Then you want to multiply it by the gain so that it equals 2^15 and lines up with AO15 */
   /* The A015 line is really the 2^31 line of the accumulator, so */
   /* In other words 2^31 = sum * 2^11 * GAIN   ->   GAIN = 2^31 / (sum * 2^11)  */
   /* Or further simplifying, GAIN = 2^20 / sum  */
   /* GAIN is set with 2^s * (1 + F/16) */
   /* 2^s * (1 + F/16) = 2^20 / sum      ->     2^(s-20) * (1 + F/16) = 1 / sum  */
   /* 2^(20-s)/(1 + F/16) = sum */
   s = 20.0 - log((double)sum) / log(2.0);
   f = 16.0 * (pow(2.0,20.0-s)/(double)sum - 1.0) + 0.5;
printf("\n\nsum = %d s = %d f = %d \n\n",sum,s, f); 

   piraq->FIRCH1_I[6]  = 0x2000 + (16 * s) + f;
   piraq->FIRCH1_Q[6]  = 0x2000 + (16 * s) + f;
   piraq->FIRCH2_I[6]  = 0x2000 + (16 * s) + f;
   piraq->FIRCH2_Q[6]  = 0x2000 + (16 * s) + f;

//   for(i=0; i<8; i++)	/* quarter rate. Number of taps = 128 (symmetric) */
//      printf("%3d %04X %04X %04X %04X\n",i,piraq->FIRCH1_I[i],piraq->FIRCH1_Q[i],piraq->FIRCH2_I[i],piraq->FIRCH2_Q[i]); 

//   for(i=191; i>180; i--)	/* quarter rate. Number of taps = 128 (symmetric) */
//      printf("%3d %04X %04X %04X %04X\n",i,piraq->FIRCH1_I[i],piraq->FIRCH1_Q[i],piraq->FIRCH2_I[i],piraq->FIRCH2_Q[i]); 
   }
   
#if 0
double	sinc(double arg)
   {
   if(arg == 0.0)	return(1.0);
   return(sin(arg) / arg);
   }

void FIR_halfband(CONFIG *config, PIRAQ *piraq, int decimation)
   {
   int	i,p;
   double	wc,timestep;
   
   wc = 2.0 * M_PI * piraq->pllfreq / (decimation * 4.0);

   /* FIR chip initialization */
   /* depending on the gate length, the FIR is either in /1 , /2 or /4 mode */
   if(config->rcvr_pulsewidth < 32)			/* full rate mode */
      {
      p = 0;		/* determine which set of control register values to select */
      timestep = 1.0 / piraq->pllfreq;	/* "64" Mhz */
      for(i=0; i<32; i++)	/* full rate. Number of taps = 64 (symmetric) */
         if(i&1)	/* if odd */
            {
            piraq->FIRCH1_Q[128 + 2 * i - 1] = 
            piraq->FIRCH1_Q[192 + 2 * i - 1] = SCALE * sinc((31-i) * timestep * wc);
            }
         else	/* if even */
            {
            piraq->FIRCH1_I[128 + 2 * i + 1] = 
            piraq->FIRCH1_I[192 + 2 * i + 1] = SCALE * sinc((31-i) * timestep * wc);
            }
      }
   else if(config->rcvr_pulsewidth < 64)	/* half rate mode */
      {
      p = 2;		/* determine which set of control register values to select */
      timestep = 2.0 / piraq->pllfreq;   /* "32" MHz */
      for(i=0; i<64; i++)	/* half rate. Number of taps = 64 (symmetric) */
         switch(i%4)
            {
            case 0:
            piraq->FIRCH1_I[128 + i + 0] = 
            piraq->FIRCH1_I[192 + i + 0] = SCALE * sinc((63-i) * timestep * wc);
            break;

            case 1:
            piraq->FIRCH1_Q[128 + i - 1] = 
            piraq->FIRCH1_Q[192 + i - 1] = SCALE * sinc((63-i) * timestep * wc);
            break;

            case 2:
            piraq->FIRCH1_I[128 + i - 1] = 
            piraq->FIRCH1_I[192 + i - 1] = SCALE * sinc((63-i) * timestep * wc);
            break;

            case 3:
            piraq->FIRCH1_Q[128 + i - 2] = 
            piraq->FIRCH1_Q[192 + i - 2] = SCALE * sinc((63-i) * timestep * wc);
            break;
            }
      }
   else										/* quarter rate mode */
      {
      p = 4;		/* determine which set of control register values to select */
      timestep = 4.0 / piraq->pllfreq;  /* "16" Mhz */
      for(i=0; i<128; i++)	/* quarter rate. Number of taps = 128 (symmetric) */
         if(i&1)	/* if odd */
            {
            piraq->FIRCH1_Q[128 + (i - 1) / 2] = 
            piraq->FIRCH1_Q[192 + (i - 1) / 2] = SCALE * sinc((127-i) * timestep * wc);
            }
         else	/* if even */
            {
            piraq->FIRCH1_I[128 + i / 2] = 
            piraq->FIRCH1_I[192 + i / 2] = SCALE * sinc((127-i) * timestep * wc);
            }
      }

   for(i=0; i<13; i++)	*(piraq->FIRCH1_I + i)  = FIR_cfg[p][i];	/* odd */
   for(i=0; i<13; i++)	*(piraq->FIRCH1_Q + i) = FIR_cfg[p+1][i];	/* even */
   for(i=0; i<13; i++)	*(piraq->FIRCH2_I + i)  = FIR_cfg[p][i];	/* odd */
   for(i=0; i<13; i++)	*(piraq->FIRCH2_Q + i) = FIR_cfg[p+1][i];	/* even */
   }   
#endif

   