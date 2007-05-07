#include "pci_w32.h"
#include "timerlib.h"
#include <time.h>
#include <stdio.h>

int cp2timer_init(TIMER *timer, int boardnumber)
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
int cp2timer_config(TIMER *timer, PINFOHEADER *info, int pciTimerMode, float radar_prt, float xmit_pulsewidth)
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
	width.hilo = COUNTFREQ * xmit_pulsewidth + 0.5;
	for(i=0; i<MAXTESTPULSE; i++)
	{
		delay.hilo = OFFSET + i * width.hilo;
		timer->tp[i].delay.byte.lo = delay.byte.lo;
		timer->tp[i].delay.byte.hi = delay.byte.hi;
		timer->tp[i].width.byte.lo = width.byte.lo;
		timer->tp[i].width.byte.hi = width.byte.hi;
	}

	timer->seqlen = 1;   /* for now the sequence will always be 1 deep */
	prt.hilo = radar_prt  * COUNTFREQ + 0.5;		/* compute prt */

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
			//			timer->seq[i+j*len].pulseenable = (i?0x2F:0x3F);
			timer->seq[i+j*len].pulseenable = 0x3f;
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

	timer->timingmode = pciTimerMode;
	printf("timingmode = %d\n",pciTimerMode);
	if (timer->timingmode == 1)
	{
		timer->sync.byte.lo = 1;		// synclo;  
		timer->sync.byte.hi = 0;		// synchi; 
	}
	else
	{
		timer->sync.hilo = 1;		// sync;  
	}
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

