#pragma once

#include "../CP2Piraq/piraqComm.h"
#include "config.h"

#define SYSTEM_CLOCK		48e6        // SYSTEM_IF = 48.0MHz
#define SYSTEM_CLOCK_INT	48000000    // INTEGER EQUIVALENT

#define	PIRAQ_DEVICE_ID 		0x9054
#define	PIRAQ_VENDOR_ID		0x10B5   

#define	PCITIMER_DEVICE_ID 	0x8504
#define	PCITIMER_VENDOR_ID	0x10E8   

#if SYSTEM_CLOCK_INT == 48000000
#define	COUNTFREQ	6000000
#else
#define	COUNTFREQ	8000000
#endif

#define	TIMER_RESET	0
#define	TIMER_STOP	1
#define	TIMER_START	2

#define	TIMER_RQST	1

#define	MAXSEQUENCE  38
#define	MAXTESTPULSE  6

typedef 
union HIMEDLO
{	
	unsigned int		himedlo;
	struct 
	{
		unsigned char		lo,med,hi,na;
	} byte;
} HIMEDLO;

typedef 
union HILO {	
	unsigned short	hilo;
	struct
	{
		unsigned char	lo,hi;
	} byte;
} HILO;

typedef	struct SEQUENCE {
	HILO		period;
	int		pulseenable;
	int		polarization;
	int		phasedata;
	int		index;
} SEQUENCE;

typedef	struct TESTPULSE {
	HILO		delay;
	HILO		width;
} TESTPULSE;

typedef	struct TIMER {
	SEQUENCE    seq[MAXSEQUENCE];
	TESTPULSE   tp[8];
	HILO        seqdelay,sync;
	int         timingmode,seqlen;
	float       clockfreq,reffreq,phasefreq;
	char*       base;
}	TIMER;


static	int	FIRSTIMER=1;
#define	OFFSET	10

int  cp2timer_config(CONFIG *config, TIMER *timer, PINFOHEADER *info, float radar_prt, float xmit_pulsewidth);
int  cp2timer_init(TIMER *timer, int boardnumber);
void cp2timer_reset(TIMER *timer);
void cp2timer_set(TIMER *timer);
void cp2timer_start(TIMER *timer);
void cp2timer_stop(TIMER *timer);
int	 cp2timer_test(TIMER *timer);
