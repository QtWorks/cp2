/************************ timer card structure ***********************/
#pragma once
#define	MAXSEQUENCE		38
#define	MAXTESTPULSE		6

/*typedef 	union {	unsigned int		himedlo;
	   			struct {unsigned char		lo,med,hi,na;} byte;} HIMEDLO;

typedef 	union {	unsigned short	hilo;
	   			struct {unsigned char	lo,hi;} byte;} HILO;*/

typedef	struct {
    HILO		period;
    int		pulseenable;
    int		polarization;
    int		phasedata;
    int		index;
	} SEQUENCE;
	
typedef	struct {
	HILO		delay;
	HILO		width;
	} TESTPULSE;
	
typedef	struct {
	SEQUENCE			seq[MAXSEQUENCE];
	TESTPULSE		tp[8];
	HILO				seqdelay;
	int				timingmode,seqlen;
	float			clockfreq,reffreq,phasefreq;
	char				*base;
	}	TIMER;

	