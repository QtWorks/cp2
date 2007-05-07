#include "PciTimer.h"
#include "pci_w32.h"


static	int	FIRSTIMER=1;
#define	OFFSET	10

/////////////////////////////////////////////////////
PciTimerConfig::PciTimerConfig(int timingMode, 
							   float radarPrt, 
							   float xmitPulseWidth):
_timingMode(timingMode),
_radarPrt(radarPrt),
_xmitPulseWidth(xmitPulseWidth)
{
	// initialize
	reset();
}

/////////////////////////////////////////////////////
PciTimerConfig::~PciTimerConfig()
{
}

/////////////////////////////////////////////////////
void
PciTimerConfig::reset() {

	for (int i = 0; i < 6; i++) {
		setBpulse(i, 0, 0);
	}

	_sequences.clear();
}

/////////////////////////////////////////////////////
void 
PciTimerConfig::setBpulse(int index, unsigned short width, unsigned short delay) 
{
	_bpulse[index].width.hilo = width;
	_bpulse[index].delay.hilo = delay;
}

/////////////////////////////////////////////////////
void
PciTimerConfig::addSequence(int length, 
							unsigned char pulseMask,
							int polarization,
							int phase) {
}

/////////////////////////////////////////////////////
int 
PciTimer::timer_init(int boardnumber)
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
	base = (char *)pci_card_membase(pcicard,256);

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
/////////////////////////////////////////////////////
/* fill the timer structure based on the infoheader */
/* Return 0 on failure, 1 on success */
int 
PciTimer::timer_config(int pciTimerMode, float radar_prt, float xmit_pulsewidth)
{

	if(FIRSTIMER)
	{
		if(!timer_init(0))
		{printf("could not find timer card\n"); exit(0);}
		FIRSTIMER = 0;
	}

	/* now set the pulsewidths a delays for the 6 timers */
	/* set timers to stagger delay the six frequencies */
	/* using the specified pulsewidth */
	HILO width.hilo = COUNTFREQ * _config.xmitPulseWidth + 0.5;
	for(i=0; i<MAXTESTPULSE; i++)
	{
		HILO delay.hilo = OFFSET + i * width.hilo;
		_config._bpulse[i].delay.byte.lo = delay.byte.lo;
		_config._bpulse[i].delay.byte.hi = delay.byte.hi;
		_config._bpulse[i].width.byte.lo = width.byte.lo;
		_config._bpulse[i].width.byte.hi = width.byte.hi;
	}

	HILO prt.hilo = radar_prt  * COUNTFREQ + 0.5;		/* compute prt */

	if(prt.hilo < 1 || prt.hilo > 65535) {
		printf("TIMER_CONFIG: invalid PRT1 %d\n",prt.hilo); 
		return(0);
	}

	_timingmode = pciTimerMode;
	printf("timingmode = %d\n",pciTimerMode);
	if (_timingmode == 1)
	{
		_sync.byte.lo = 1;		// synclo;  
		_sync.byte.hi = 0;		// synchi; 
	}
	else
	{
		_sync.hilo = 1;		// sync;  
	}
	_clockfreq = SYSTEM_CLOCK;
	_reffreq = 10.0E6;
	_phasefreq = 50.0E3;

	return(1);	/* success */
}

/////////////////////////////////////////////////////
void 
PciTimer::timer_reset()
{
	if(FIRSTIMER)
	{
		if(!cp2timer_init(timer,0))
		{printf("could not find timer card\n"); exit(0);}
		FIRSTIMER = 0;
	}

	// set request ID to STOP
	base[0xFE] = TIMER_RESET; /* @@@ was base[0xFE * 4] */
	// set request flag
	base[0xFF] = TIMER_RQST;	/* @@@ *4 signal the timer that a request is pending */
	timer_test(timer);
	printf("Passed timer_reset\n");
}

/////////////////////////////////////////////////////
void 
PciTimer::timer_stop()
{
	if(FIRSTIMER)
	{
		if(!cp2timer_init(timer,0))
		{printf("could not find timer card\n"); exit(0);}
		FIRSTIMER = 0;
	}

	// set request ID to STOP
	_base[0xFE] = TIMER_STOP; /* @@@ was base[0xFF * 4] */
	// set request flag
	_base[0xFF] = TIMER_RQST;	/* @@@ *4 signal the timer that a request is pending */
	cp2timer_test(timer);
	printf("Passed timer_stop\n");
}

/////////////////////////////////////////////////////
/* open the timer board (once) and configure it with the timer structure */
void 
PciTimer::timer_set()
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
	for(i=0; i<0xC0; i++)	_base[i] = 0; /* @@@ *4 */

	/* set the dual port ram */

	/* The sequence starts at memory offset zero */
	/* BYTE1:  low byte of period */
	/* BYTE2:  high byte of period */
	/* BYTE3:  Pulse enables */
	/* BYTE4:  Polarization control (bits 8 - 15 on connector) */
	/* BYTE5:  Phase Data (bits 0 - 7 on connector) */
	//   for(i=0; i<_seqlen; i++)
	//   { /* @@@ took out ) * 4] on all of these */
	/* program the 5 bytes for this sequence */
	//      _base[(i * 5 + 0) ] = _seq[i].period.byte.lo;
	//      _base[(i * 5 + 1) ] = _seq[i].period.byte.hi;
	//      _base[(i * 5 + 2) ] = _seq[i].pulseenable ^ 0x3F;
	//      _base[(i * 5 + 3) ] = _seq[i].polarization; // ^ 7; This was for NEC
	//      _base[(i * 5 + 4) ] = _seq[i].phasedata;
	//   }


	// @@@  This has been changed for the optimized timer code
	//		there is a different sequence memory setup.
	for(i=0; i<_seqlen; i++)
	{
		_base[(0x00 + i)] = _seq[i].period.byte.lo;
		_base[(0x10 + i)] = _seq[i].period.byte.hi;
		_base[(0x20 + i)] = _seq[i].pulseenable ^ 0x3F;
		_base[(0x30 + i)] = _seq[i].polarization; 
		_base[(0x40 + i)] = _seq[i].phasedata;
	}

	/* now set the pulsewidths a delays for the 6 timers */
	for(i=0; i<MAXTESTPULSE; i++)
	{ /* @@@ took out ) * 4] on all of these */
		_base[(0xC0 + i * 4 + 0) ] = _tp[i].delay.byte.lo;
		_base[(0xC0 + i * 4 + 1) ] = _tp[i].delay.byte.hi;
		_base[(0xC0 + i * 4 + 2) ] = _tp[i].width.byte.lo;
		_base[(0xC0 + i * 4 + 3) ] = _tp[i].width.byte.hi;
	}

	/* compute the data to go into the register */
	n = 0.5 + _clockfreq / _phasefreq;
	a = n & 7;
	n /= 8;
	r = 0.5 + _reffreq / _phasefreq;
	ref.himedlo = r << 2 | 0x100000;     					/* r data with reset bit */
	freq.himedlo = n << 7 | a << 2 | 0x100001;     		/* r data with reset bit */

	/* @@@ took out ) * 4] on all of these */
	/* now set up the special registers */
	_base[(0xD8 + 0) ] = freq.byte.lo;				//freqlo;
	_base[(0xD8 + 1) ] = freq.byte.med;				//freqmed;
	_base[(0xD8 + 2) ] = freq.byte.hi;				//freqhi;
	_base[(0xD8 + 3) ] = ref.byte.lo;				//reflo;
	_base[(0xD8 + 4) ] = ref.byte.med;				//refmed;
	_base[(0xD8 + 5) ] = ref.byte.hi;				//refhi;
	_base[(0xD8 + 6) ] = 0;					//div;
	_base[(0xD8 + 7) ] = 0;					//spare;

	_base[(0xD8 +  8) ] = _sync.byte.lo;		// synclo;
	_base[(0xD8 +  9) ] = _sync.byte.hi;		// synchi;
	_base[(0xD8 + 10) ] = _seqdelay.byte.lo;	// sequence delay lo
	_base[(0xD8 + 11) ] = _seqdelay.byte.hi;	// sequence delay hi
	_base[(0xD8 + 12) ] = _seqlen;		// sequence length

	_base[(0xD8 + 13) ] = 0;					// current sequence count
	_base[(0xD8 + 14) ] = _timingmode;	// timing mode

	//   timer_reset(timer);
}
/////////////////////////////////////////////////////
/* poll the command response byte and handle timeouts */
int	
PciTimer::timer_test()
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
	while((_base[0xFF] & 1))   /* @@@ was base[0xFF * 4] */
	{
		Sleep(1);
		if(time(NULL) > first)
		{
			printf("Timeout waiting for PCITIMER to respond to request\n");
			exit(0);
		}
	}
	_base[0xFF] = 0x02; /* @@@ was base[0xFF * 4] */
	return(0);   
}

/////////////////////////////////////////////////////
void 
PciTimer::timer_start()
{
	int	sec;

	if(FIRSTIMER)
	{
		if(!cp2timer_init(timer,0))
		{printf("could not find timer card\n"); exit(0);}
		FIRSTIMER = 0;
	}

	/* start the timer assuming the first pulse occured at unix time zero */
	//   if(_timingmode == TIMER_SYNC)
	//      {
	//      sec = half_second_wait();		/* wait until half sec, return time of next second */
	//	  _pulsenumber
	//      }

	// set request ID to START
	_base[0xFE] = TIMER_START;	/* @@@ *4 signal the timer that a request is pending */
	// set request flag
	_base[0xFF] = TIMER_RQST;	/* @@@ *4 signal the timer that a request is pending */
	timer_test();
	printf("Passed timer_start\n");
}

