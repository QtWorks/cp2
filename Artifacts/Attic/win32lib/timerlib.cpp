/*
Timerlib.cpp
Revised and working 3/25/03
Revised and working for byte-wise addressing 3/18/04
Revised for optimized timer once again  4/30/04
	@@@ denotes new changes
*/

#include <Afx.h>

#include        <stdio.h>
#include        <stdlib.h>
#include        <string>
#include        <time.h>
#include        <math.h>

#include        "../include/proto.h"


static	int	FIRSTIMER=1;

static char *parms[] = {
		      "clockfreq","reffreq","phasefreq",
		      "step","pulseenable","period","polarization","phasedata",
		      "testpulse","delay","width","seqdelay","timingmode",
		      "trigdelay","debug",""};

#define	tptest()	if(teststate)   {err++; printf("expecting \"delay\" or \"width\" keyword in line %d\n",linenum); teststate = 0;}
#define	stptest()	if(stepstate)   {err++; printf("expecting \"pulseenable\", \"period\", \"polarization\", or \"phasedata\", keyword in line %d\n",linenum); stepstate = 0;}

/* timerboard commands */
#define	TIMER_RESET	0
#define	TIMER_STOP	1
#define	TIMER_START	2

#define	TIMER_RQST	1

/* if fname is NULL string, default file will be loaded. */
/* all characters following text keyword are read-in as ascii */
void timer_read(char *fname, TIMER *timer)
   {
   int  linenum,debug,i,j,err=0;
   unsigned int temp, tval;
   char filename[180],keyword[180],value[180],line[180], *ctemp;
   FILE *fp;
   int		tindex=0,teststate=0,stepstate=0;
   SEQUENCE	save;
 
   memset(keyword,0,255);
   debug = 0;

 if(FIRSTIMER)
      {
      if(!timer_init(timer,0))
		{printf("READTIMER: Could not find timer card\n"); exit(0);}
      FIRSTIMER = 0;
      }

   /* initialize all parameters in the timer structure */
   memset(timer,0,sizeof(TIMER) - sizeof(char *));

   timer->sync.hilo = 1;		/* default trigger delay */

   if(strcmp(fname,""))        
      {
      for(i=0; fname[i] && fname[i] != '.'; i++)  filename[i] = fname[i];
      strcpy(&filename[i],".tmr");
      fp = fopen(filename,"r");
      if(!fp)
		{
		printf("Error: File not found: %s\n",filename);
		exit(-1);
	 	}
      }
   else 
      {
	  strcpy(filename,"config.tmr");
      fp = fopen(filename,"r");
      }
   
   if(!fp) {printf("READTIMER: cannot open configuration file %s\n",filename); exit(0);}

   linenum = 0;

  while((ctemp = fgets(line,170,fp)) != NULL)  /* while not end of file */
    {
    linenum++;

    getkeyval(line,keyword,value);  /* get the keyword and the value */

    if(strlen(keyword) != 0 && keyword[0] != ';')       /* if line is not a comment line or blank */  
		{
		tval = parse(keyword,parms);

		switch(tval)
			{
			case 0:        /* clockfreq */
				tptest();	stptest();
				set(value,"%f",&timer->clockfreq,keyword,linenum,filename);
				break;
			case 1:        /* reffreq */
				tptest();	stptest();
				set(value,"%f",&timer->reffreq,keyword,linenum,filename);
				break;
			case 2:        /* phasefreq */
				tptest();	stptest();
				set(value,"%f",&timer->phasefreq,keyword,linenum,filename);
				break;
			case 3:        /* step */
				tptest();	stptest();
				stepstate = 0x01;
				if(timer->seqlen > MAXSEQUENCE)	{err++; printf("exceeded maximum sequence length\n"); stepstate = 0;}
				set(value,"%i",&timer->seq[timer->seqlen].index,keyword,linenum,filename);
				break;
			case 4:        /* pulseenable */
				tptest();
				if(!stepstate)	{err++; printf("expecting \"step\" keyword in line %d\n",linenum); continue;}
				stepstate |= 0x02;
				set(value,"%i",&timer->seq[timer->seqlen].pulseenable,keyword,linenum,filename);
				if(stepstate == 0x1F)	{timer->seqlen++; stepstate = 0;}
				break;
			case 5:        /* period */
				tptest();
				if(!stepstate)	{err++; printf("expecting \"step\" keyword in line %d\n",linenum); continue;}
				stepstate |= 0x04;
				set(value,"%i",&timer->seq[timer->seqlen].period,keyword,linenum,filename);
				if(stepstate == 0x1F)	{timer->seqlen++; stepstate = 0;}
				break;
			case 6:        /* polarization */
				tptest();
				if(!stepstate)	{err++; printf("expecting \"step\" keyword in line %d\n",linenum); continue;}
				stepstate |= 0x08;
				set(value,"%i",&timer->seq[timer->seqlen].polarization,keyword,linenum,filename);
				if(stepstate == 0x1F)	{timer->seqlen++; stepstate = 0;}
				break;
			case 7:        /* phasedata */
				tptest();
				if(!stepstate)	{err++; printf("expecting \"step\" keyword in line %d\n",linenum); continue;}
				stepstate |= 0x10;
				set(value,"%i",&timer->seq[timer->seqlen].phasedata,keyword,linenum,filename);
				if(stepstate == 0x1F)	{timer->seqlen++; stepstate = 0;}
				break;
			case 8:        /* testpulse */
				tptest();	stptest();
				teststate = 0x01;
				set(value,"%i",&tindex,keyword,linenum,filename);
				if(tindex >= MAXTESTPULSE || tindex < 0)	 {err++; printf("testpulse number out of range in line %d\n",linenum); teststate = 0;}
				break;
			case 9:        /* delay */
				stptest();
				if(!teststate)	{err++; printf("expecting \"testpulse\" keyword in line %d\n",linenum); continue;}
				teststate |= 0x02;
				set(value,"%i",&timer->tp[tindex].delay,keyword,linenum,filename);
				if(teststate == 0x07)	teststate = 0;
				break;
			case 10:        /* width */
				stptest();
				if(!teststate)	{err++; printf("expecting \"testpulse\" keyword in line %d\n",linenum); continue;}
				teststate |= 0x04;
				set(value,"%i",&timer->tp[tindex].width,keyword,linenum,filename);
				if(teststate == 0x07)	teststate = 0;
				break;
			case 11:        /* seqdelay */
				tptest(); stptest();
				set(value,"%i",&temp,keyword,linenum,filename);
				if(temp > 65536)		{printf("Value for %s out of range",parms[11]); err++;}
				timer->seqdelay.hilo = temp;
				break;
			case 12:        /* timingmode */
				tptest(); stptest();
				set(value,"%i",&timer->timingmode,keyword,linenum,filename);
				if(timer->timingmode > 2 || timer->timingmode < 0)
				   {err++; printf("invalid timingmode %d\n",timer->timingmode);}
				break;
			case 13:        /* trigdelay */
				tptest(); stptest();
				set(value,"%i",&temp,keyword,linenum,filename);
				if(temp > 65536)		{printf("Value for %s out of range",parms[13]); err++;}
				timer->sync.hilo = temp;
				break;
			case 14:        /* debug */
				set(value,"%q",&debug,keyword,linenum,filename);
				break;
			default:
				printf("unrecognized keyword \"%s\" in line %d of %s\n",keyword,linenum,filename);
				err++;
				break;
			}
		}
	 }   

	fclose(fp);

   /* put the sequences in order */
   for(i=0; i<timer->seqlen; i++)	/* for each slot in the result */
      {
      for(j=i; j<timer->seqlen; j++)	/* search all slots for the next value (i) */
         {
         if(timer->seq[j].index < i)		/* see if a sequence index appears twice */
            {
			err++; 
			printf("step %d specified more than once\n",timer->seq[j].index);
			}

         if(timer->seq[j].index == i)  /* see if we find the one we're looking for */
            {
            if(j == i)	break;	/* don't have to do a swap */
            /* do the swap */
            memcpy(&save,&timer->seq[j],sizeof(SEQUENCE));
            memcpy(&timer->seq[j],&save,sizeof(SEQUENCE));
			break;
            }
         }

	  if(j == timer->seqlen)	/* an index is missing */
		{
		err++;  
		printf("step %d not specified\n",i);
		}
      }

   if(timer->timingmode == 0 && timer->sync.hilo < 10)	/* if in internally generated pulse mode */
      {
      printf("trigger delay is too small to allow self-starting in timing mode 0\n");
      printf("trigger delay set to 10\n");
      timer->sync.hilo = 10;		/* give it 10 uS so that CNT0 goes low after the PPSCLR signal is gone */
      }


   if(debug)
      {
      printf("\nDEBUG: %s\n",filename);
      printf("%-28s %i\n",parms[12],timer->timingmode);
      printf("%-28s %i\n",parms[13],timer->sync.hilo);
      return;
      }
   if(err)return;
   }  

/* open the timer board (once) and configure it with the timer structure */
void timer_set(TIMER *timer)
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
      if(!timer_init(timer,0))
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


void timer_dmp(TIMER *timer)
   {
   int	i,j;
   
   if(FIRSTIMER)
      {
      if(!timer_init(timer,0))
         {printf("could not find timer card\n"); exit(0);}
      FIRSTIMER = 0;
      }
   
   printf("performing timer card memory dump:\n");
   for(i=0; i<16; i++)
      {
      printf("%02X:  ",i*16);
      for(j=0; j<16; j++)
         {
         printf("%02X ",timer->base[(i*16+j)] & 0xFF);  /* @@@ j) * 4] */
         }
      printf("\n");
      }
   }


/* poll the command response byte and handle timeouts */
int	timer_test(TIMER *timer)
   {
   volatile	int	temp;
   time_t	first;
   
   if(FIRSTIMER)
      {
      if(!timer_init(timer,0))
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
   
void timer_start(TIMER *timer)
   {
   int	sec;

   if(FIRSTIMER)
      {
      if(!timer_init(timer,0))
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
   timer_test(timer);
   printf("Passed timer_start\n");
   }
   
/* returns when the time is half way between to second ticks */
/* return the time of the upcomming tick */
int	half_second_wait(void)
   {
   time_t	sec;
   
   for(sec = time(0); sec != time(0);	Sleep(10));

   Sleep(495);
   
   return(sec + 2);
   }

void timer_reset(TIMER *timer)
   {
   if(FIRSTIMER)
      {
      if(!timer_init(timer,0))
         {printf("could not find timer card\n"); exit(0);}
      FIRSTIMER = 0;
      }

   // set request ID to STOP
   timer->base[0xFE] = TIMER_RESET; /* @@@ was base[0xFE * 4] */
   // set request flag
   timer->base[0xFF] = TIMER_RQST;	/* @@@ *4 signal the timer that a request is pending */
   timer_test(timer);
   printf("Passed timer_reset\n");
   }

void timer_stop(TIMER *timer)
   {
   if(FIRSTIMER)
      {
      if(!timer_init(timer,0))
         {printf("could not find timer card\n"); exit(0);}
      FIRSTIMER = 0;
      }

   // set request ID to STOP
   timer->base[0xFE] = TIMER_STOP; /* @@@ was base[0xFF * 4] */
   // set request flag
   timer->base[0xFF] = TIMER_RQST;	/* @@@ *4 signal the timer that a request is pending */
   timer_test(timer);
   printf("Passed timer_stop\n");
   }

int   timer_init(TIMER *timer, int boardnumber)
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
   

#define	XMIT1PRETRIG		0		// drives transmitter
#define	XMIT2PRETRIG		1		// drives transmitter
// called "H TEST PULSE"
#define	HSAMPLESWITCH	2
// called "V TEST PULSE"
#define	VSAMPLESWITCH	3
#define	FRAMETRIGGER		4		// called "PULSE SPARE 1"   Used for test box
#define	PRETRIGGER		5		// called "PULSE SPARE 2"	Used for test box
#define	TRIGGERSPARE1	6		// always has a delay of 0 and a width of 1 us
#define	TRIGGERSPARE2	7		// always has a delay of 0 and a width of 1 us

/* define the phase sequence for up to a length of 64 */
/*
static unsigned char PhaseSeq[] = {
	0x00,0x10,0x30,0x60,0xA0,0xF0,0x50,0xC0,
	0x40,0xD0,0x70,0x20,0xE0,0xB0,0x90,0x80,
	0x80,0x90,0xB0,0xE0,0x20,0x70,0xD0,0x40,
	0xC0,0x50,0xF0,0xA0,0x60,0x30,0x10,0x00};
*/

/*
static unsigned char PhaseSeq[] = {
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
*/
/*
static unsigned char PhaseSeq[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
*/

static unsigned char PhaseSeq[] = {
	0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

/* this is a wrapper for the primitives that make the timer do what's specified in a PIRAQX header */
void start_timer_card(TIMER *timer,INFOHEADER *header)
   {
   
	timer_config(timer,header);

//   timer_stop(timer);	// stop the timer 

   timer_set(timer);		// put the timer structure into the timer DPRAM 

   timer_reset(timer);	// tell the timer to initialize with the values from DPRAM 

   timer_start(timer);	// start timer 

//   timer_dmp(timer);		// print out contents of DPRAM 	
   }

#define	OFFSET	10

/* fill the timer structure based on the infoheader */
/* Return 0 on failure, 1 on success */
int timer_config(TIMER *timer, INFOHEADER *info)
   {
   int		i,dualprt;
   HILO		delay,width,prt[2];
   int		j,cycles,len;

   if(FIRSTIMER)
      {
      if(!timer_init(timer,0))
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
   prt[0].hilo = info->prt[0]  * COUNTFREQ + 0.5;		/* compute prt1 */
   prt[1].hilo = info->prt[1] * COUNTFREQ + 0.5;

   /* check sanity of request */
   if(timer->seqlen > 12 || timer->seqlen < 1)	{printf("TIMER_CONFIG: invalid sequence length %d\n",timer->seqlen); return(0);}
   if(prt[0].hilo < 1 || prt[0].hilo > 65535)	{printf("TIMER_CONFIG: invalid PRT1 %d\n",prt[0].hilo); return(0);}
   if(prt[1].hilo < 1 || prt[1].hilo > 65535)	{printf("TIMER_CONFIG: invalid PRT2 %d\n",prt[1].hilo); return(0);}
   dualprt = (prt[0].hilo == prt[1].hilo) ? 1 : 2;
printf("dualprt = %d\n", dualprt); 

   len = timer->seqlen * dualprt;
   cycles = 1;

//   if(info->ctrlflags & CTRL_SECONDTRIP)  /* if second trip reduction required */
//      cycles = 4 / len;

   for(j=0; j<cycles; j++)
      {
      for(i=0; i<timer->seqlen * dualprt; i++)
         {
         /* program the 5 bytes for this sequence */
         timer->seq[i+j*len].period.byte.lo = prt[i<timer->seqlen?0:1].byte.lo;
         timer->seq[i+j*len].period.byte.hi = prt[i<timer->seqlen?0:1].byte.hi;
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

#if 0 // comment out 4-21-03
/* fill the timer structure based on the infoheader */
/* Return 0 on failure, 1 on success */
int timer_config(TIMER *timer, INFOHEADER *info)
   {
   int		i,dualprt;
   HILO		delay,width,prt[2];
   int		j,cycles,len;

   if(FIRSTIMER)
      {
      if(!timer_init(timer,0))
         {printf("could not find timer card\n"); exit(0);}
      FIRSTIMER = 0;
      }

   /* initialize all parameters in the timer structure */
   memset(timer,0,sizeof(TIMER) - sizeof(char *));

   /* now set the pulsewidths a delays for the 6 timers */
   /* set all timers for 10 counts offest with 1 uS width */
   delay.hilo = 8;
   width.hilo = 8;
   for(i=0; i<MAXTESTPULSE; i++)
      {
      timer->tp[i].delay.byte.lo = delay.byte.lo;
      timer->tp[i].delay.byte.hi = delay.byte.hi;
      timer->tp[i].width.byte.lo = width.byte.lo;
      timer->tp[i].width.byte.hi = width.byte.hi;
      }

   /* set up the xmit sample delay and width for H */
//!!! Not rapidow	delay.hilo = info->sampledelay * COUNTFREQ + 0.5;
//!!! Not rapidow	width.hilo = info->samplewidth * COUNTFREQ + 0.5;
   timer->tp[HSAMPLESWITCH].delay.byte.lo = delay.byte.lo;
   timer->tp[HSAMPLESWITCH].delay.byte.hi = delay.byte.hi;
   timer->tp[HSAMPLESWITCH].width.byte.lo = width.byte.lo;
   timer->tp[HSAMPLESWITCH].width.byte.hi = width.byte.hi;
   
   /* set up the xmit sample delay and width for V */
   timer->tp[VSAMPLESWITCH].delay.byte.lo = delay.byte.lo;
   timer->tp[VSAMPLESWITCH].delay.byte.hi = delay.byte.hi;
   timer->tp[VSAMPLESWITCH].width.byte.lo = width.byte.lo;
   timer->tp[VSAMPLESWITCH].width.byte.hi = width.byte.hi;
   
   timer->seqlen = info->polarization;   /* the sequence based on the sequence length */
   prt[0].hilo = info->prt[0]  * 8000000.0 + 0.5;		/* compute prt1 */
   prt[1].hilo = info->prt[1] * 8000000.0 + 0.5;

   /* check sanity of request */
   if(timer->seqlen > 12 || timer->seqlen < 1)	{printf("TIMER_CONFIG: invalid sequence length %d\n",timer->seqlen); return(0);}
   if(prt[0].hilo < 1 || prt[0].hilo > 65535)	{printf("TIMER_CONFIG: invalid PRT1 %d\n",prt[0].hilo); return(0);}
   if(prt[1].hilo < 1 || prt[1].hilo > 65535)	{printf("TIMER_CONFIG: invalid PRT2 %d\n",prt[1].hilo); return(0);}
   dualprt = (prt[0].hilo == prt[1].hilo) ? 1 : 2;

   len = timer->seqlen * dualprt;
   cycles = 1;
   if(info->ctrlflags & CTRL_SECONDTRIP)  /* if second trip reduction required */
      cycles = 4 / len;

cycles = 1;

   for(j=0; j<cycles; j++)
      {
      for(i=0; i<timer->seqlen * dualprt; i++)
         {
         /* program the 5 bytes for this sequence */
         timer->seq[i+j*len].period.byte.lo = prt[i<timer->seqlen?0:1].byte.lo;
         timer->seq[i+j*len].period.byte.hi = prt[i<timer->seqlen?0:1].byte.hi;
         timer->seq[i+j*len].pulseenable = (i?0x2F:0x3F);
//!!!not in rapidDOW         timer->seq[i+j*len].polarization = (i?0:0x80) | info->polarization_array[i % timer->seqlen];  /* add in a frame sync */
         timer->seq[i+j*len].phasedata = (info->ctrlflags & CTRL_SECONDTRIP) ? PhaseSeq[i+j*len] : 0x00;
  timer->seq[i+j*len].phasedata = 0x00;
         }
      }

   timer->seqlen *= dualprt * cycles;		/* double if in dual prt mode */
         
   timer->seqdelay.hilo = 8;
   timer->timingmode = 0;
   
   timer->clockfreq = 64.0E6;
   timer->reffreq = 10.0E6;
   timer->phasefreq = 50.0E3;

   return(1);	/* success */
   }
#endif