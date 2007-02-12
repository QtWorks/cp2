#pragma once

#define DATA_SIMPLEPP              0 ///< simple pulse pair ABP 
#define DATA_POLYPP                1 ///< poly pulse pair ABPAB 
#define DATA_DUALPP                2 ///< dual prt pulse pair ABP,ABP 
#define DATA_POL1                  3 ///< dual polarization pulse pair ABP,ABP 
#define DATA_POL2                  4 ///< more complex dual polarization ??????? 
#define DATA_POL3                  5 ///< almost full dual polarization with log integers 
#define DATA_SIMPLE16              6 ///< 16 bit magnitude/angle representation of ABP 
#define DATA_DOW                   7 ///< dow data format 
#define DATA_FULLPOL1              8 ///< 1998 full dual polarization matrix for alternating H-V 
#define DATA_FULLPOLP              9 ///< 1999 full dual polarization matrix plus for alternating H-V 
#define DATA_MAXPOL               10 ///< 2000 full dual polarization matrix plus for alternating H-V 
#define DATA_HVSIMUL              11 ///< 2000 copolar matrix for simultaneous H-V 
#define DATA_SHRTPUL              12 ///< 2000 full dual pol matrix w/gate averaging no Clutter Filter 
#define DATA_SMHVSIM              13 ///< 2000 DOW4 copolar matrix for simultaneous H-V (no iq average) 
#define	DATA_STOKES	              14 ///< NEC Stokes parameters 
#define	DATA_FFT	              15 ///< NEC FFT mean velocity and power 
#define	PIRAQ_ABPDATA             16 ///< ABP data computed in piraq3: rapidDOW project 
#define	PIRAQ_ABPDATA_STAGGER_PRT 17 ///< Staggered PRT ABP data computed in piraq3: rapidDOW project 
#define	PIRAQ_CP2_TIMESERIES 18      ///< Simple timeseries: CP2 project 
#define DATA_TYPE_MAX PIRAQ_CP2_TIMESERIES ///< limit of data types  

typedef struct CONFIG
{
	int             gatesa, gatesb, hits, rcvr_pulsewidth, timeseries, gate0mode, startgate;
	int             prt, timingmode, delay, sync;
	int             ts_start_gate, ts_end_gate, pcorrect, clutterfilter, xmit_pulsewidth; 
	int	            clutter_start, clutter_end; 
	int             prt2, velsign, hitcount;
	char            outfilename[80], outpath[80];
	float           afcgain, afchi, afclo, locktime;
	int             trigger, testpulse, tpwidth, tpdelay, intenable;
	float           stalofreq;
	signed char     int_phase;
	float           int_dac;
	unsigned int    int_sync, int_period;
	unsigned int    ethernet;  /* the ethernet address */
	int             dataformat;
	int             boardnum;  /* which of n cards are we refering to */
	float           meters_to_first_gate; 
	float           gate_spacing_meters; 
	int				pcitimermode;  // timing mode for the pci timer card
} CONFIG;

void readconfig(char *fname, CONFIG *config);
