#include "PciTimer.h"
#include "pci_w32.h"
#include <time.h>

#define	OFFSET	10

/////////////////////////////////////////////////////
PciTimerConfig::PciTimerConfig(int systemClock,
							   int timingMode):
_systemClock(systemClock),
_timingMode(timingMode)
{
	// initialize
	for (int i = 0; i < 6; i++) {
		setBpulse(i, 0, 0);
	}
}

/////////////////////////////////////////////////////
PciTimerConfig::~PciTimerConfig()
{
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
							int phase) 
{
	struct Sequence s;
	s.length.hilo = length;
	s.bpulseMask = pulseMask;
	s.polarization = polarization;
	s.phase = phase;
	_sequences.push_back(s);
}
/////////////////////////////////////////////////////
void
PciTimerConfig::addPrt(float radar_prt, 
					   unsigned char pulseMask,
					   int polarization,
					   int phase) 
{
	struct Sequence s;
	s.length.hilo = (unsigned short)(radar_prt  * COUNTFREQ + 0.5);
	s.bpulseMask = pulseMask;
	s.polarization = polarization;
	s.phase = phase;
	_sequences.push_back(s);
}


/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
PciTimer::PciTimer(PciTimerConfig config):
_config(config),
_error(false)
{
	PCI_CARD	*pcicard;
	int		reg_base;

	// initialize the TvicHW32 system
	init_pci(); 

	// find the PCI timer card.
	int boardnumber = 0;
	pcicard = find_pci_card(PCITIMER_VENDOR_ID,
		PCITIMER_DEVICE_ID,
		boardnumber);
	if (!pcicard) {
		_error = true;
		return;
	}
	// Get the phys2 address.
	reg_base = pcicard->phys2;

	// Map the card's memory
	_base = (char *)pci_card_membase(pcicard,256);

	/* initialize the PCI registers for proper board operation */
	out32(reg_base + 0x60, 0xA2A2A2A2);  /* pass through register */
	//   out32(reg_base + 0x38, 0x00010000);  /* enable addint interrupts */
	//   out32(reg_base + 0x3C, 0xFFFFE004);  /* reset control register */
	//   out32(reg_base + 0x60, 0x81818181);  /* pass through register */

	// print the registers from the PCI card.
	printf("PCI Configuration registers as seen from timer_read\n");
	for(int j = 0; j < 8; j++) {
		printf("%02X ",4*4*j);
		for(int i = 0; i < 4; i++) {
			unsigned int val = pci_read_config32(pcicard,4*(i+4*j));  /* read 32 bit value at requested register */
			printf("%08lX ", val);
		}
		printf("\n");
	}
	printf("\nI/O Base: %8X\n",reg_base);
	for(int j = 0; j < 8; j++) {
		printf("%02X ",4*4*j);
		for(int i = 0; i < 4; i++)
			printf("%08lX ",in32(reg_base + 4*(i+4*j)));
		printf("\n");
	}

	// For now, override the BPULSE signals in _config, 
	// and set to be a staggered 
	/// series of pulses at a fixed pulse width.
	/// @todo Remove this so that  BPULSE configuration
	/// comes from _config.
	TIMERHILO width;
	width.hilo = (unsigned short)(COUNTFREQ * 1.0e-6 + 0.5);
	for(int i = 0; i< 6; i++) {
		TIMERHILO delay;
		delay.hilo = OFFSET + i * width.hilo;
		_config._bpulse[i].delay.byte.lo = delay.byte.lo;
		_config._bpulse[i].delay.byte.hi = delay.byte.hi;
		_config._bpulse[i].width.byte.lo = width.byte.lo;
		_config._bpulse[i].width.byte.hi = width.byte.hi;
	}

	// Set the _sync flag, based on the timing mode.
	/// @todo  What is the sync flag?
	printf("timingmode = %d\n", _config._timingMode);
	if (_config._timingMode == 1) {
		_sync.byte.lo = 1;		// synclo;  
		_sync.byte.hi = 0;		// synchi; 
	} else {
		_sync.hilo = 1;		// sync;  
	}

	// save the clock frequency, refference frequency and phase frequency.
	_clockfreq = (float)_config._systemClock;
	_reffreq = 10.0E6;
	_phasefreq = 50.0E3;

	// set the timer registers
	set();
}

/////////////////////////////////////////////////////
PciTimer::~PciTimer() {
	// reset the timer
	reset();
}

/////////////////////////////////////////////////////
void 
PciTimer::reset()
{
	// set request ID to STOP
	_base[0xFE] = TIMER_RESET; /* @@@ was base[0xFE * 4] */
	// set request flag
	_base[0xFF] = TIMER_RQST;	/* @@@ *4 signal the timer that a request is pending */
	test();
	printf("Passed timer_reset\n");
}

/////////////////////////////////////////////////////
void 
PciTimer::stop()
{
	// set request ID to STOP
	_base[0xFE] = TIMER_STOP; /* @@@ was base[0xFF * 4] */
	// set request flag
	_base[0xFF] = TIMER_RQST;	/* @@@ *4 signal the timer that a request is pending */
	test();
	printf("Passed timer_stop\n");
}

/////////////////////////////////////////////////////
/* open the timer board (once) and configure it with the timer structure */
void 
PciTimer::set()
{
	int			a,r,n;
	TIMERHIMEDLO		ref,freq;

	//   timer_stop(timer);

	/* clear the sequence mem */
	for( int i = 0; i < 0xC0; i++)
		_base[i] = 0; /* @@@ *4 */

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
	for(unsigned int i = 0; i < _config._sequences.size(); i++)
	{
		_base[(0x00 + i)] = _config._sequences[i].length.byte.lo;
		_base[(0x10 + i)] = _config._sequences[i].length.byte.hi;
		_base[(0x20 + i)] = _config._sequences[i].bpulseMask ^ 0x3F;
		_base[(0x30 + i)] = _config._sequences[i].polarization; 
		_base[(0x40 + i)] = _config._sequences[i].phase;
	}

	/* now set the pulsewidths a delays for the 6 timers */
	for(int i = 0; i < 6; i++)
	{ /* @@@ took out ) * 4] on all of these */
		_base[(0xC0 + i * 4 + 0) ] = _config._bpulse[i].delay.byte.lo;
		_base[(0xC0 + i * 4 + 1) ] = _config._bpulse[i].delay.byte.hi;
		_base[(0xC0 + i * 4 + 2) ] = _config._bpulse[i].width.byte.lo;
		_base[(0xC0 + i * 4 + 3) ] = _config._bpulse[i].width.byte.hi;
	}

	/* compute the data to go into the register */
	n = (int)(0.5 + _clockfreq / _phasefreq);
	a = n & 7;
	n /= 8;
	r = (int) (0.5 + _reffreq / _phasefreq);
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
	_base[(0xD8 + 12) ] = (char)(_config._sequences.size());		// sequence length

	_base[(0xD8 + 13) ] = 0;					// current sequence count
	_base[(0xD8 + 14) ] = _config._timingMode;	// timing mode

	//   timer_reset(timer);
}
/////////////////////////////////////////////////////
/* poll the command response byte and handle timeouts */
int	
PciTimer::test()
{
	time_t	first;

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
PciTimer::start()
{
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
	test();
	printf("Passed timer_start\n");
}

/////////////////////////////////////////////////////
bool
PciTimer::error()
{
	return _error;
}