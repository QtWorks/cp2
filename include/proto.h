/****************************************************/
/* various defines */
/****************************************************/
#pragma once
#include <windows.h>
#include "../PIRAQIII_RevD_Driver/piraq.h"
#include "../win32lib/pci_w32.h"
#include "dd_types.h"

#define NEW_INCLUDE // new definitions for RapidDOW DWELL, UDPHEADER, 4-byte data alignment. 

#define	MAGIC		0x12345678

#define SYSTEM_CLOCK		48e6        // SYSTEM_IF = 48.0MHz
#define SYSTEM_CLOCK_INT	48000000    // INTEGER EQUIVALENT
//#define SYSTEM_CLOCK		64e6        // SYSTEM_IF = 64.0MHz
//#define SYSTEM_CLOCK_INT	64000000    // INTEGER EQUIVALENT

#define	PIRAQ_DEVICE_ID 		0x9054
#define	PIRAQ_VENDOR_ID		0x10B5   

#define	PCITIMER_DEVICE_ID 	0x8504
#define	PCITIMER_VENDOR_ID	0x10E8   

#define DATUM_VENDORID 0x12e2
#define DATUM_DEVICEID 0x4013

#define DIO_VENDORID 0x1307
#define DIO_DEVICEID 0xB

#define NCAR_DRX	// NCAR drx: one piraq, no PMAC, no timer card, CP2exec.exe wait for piraq in subs.cpp\start()
#define PIRAQS				3 // RapidDOW: 3 piraqs
#define CHANNELS			2*PIRAQS // ?
#define CHANNEL_PARAMETERS	2 // #parameters currently specified by channel in config files

#define CP2_TESTING		// switch ON test code for CP2 
//#define UDPTESTLOCAL	// test broadcast functions on a local system; else use Ethernet. 

/* digitalrcv.c */
#define	CHANGE_NOCHANGE	-1
#define	CHANGE_NOTHING	0
#define	CHANGE_ILLDO		1
#define	CHANGE_RESTART	2

#if SYSTEM_CLOCK_INT == 48000000
#define	COUNTFREQ	6000000
#else
#define	COUNTFREQ	8000000
#endif

typedef struct  {float x,y;}   complex;

#define K2      0.93
#define C       2.99792458E8
#define M_PI    3.141592654 
#define TWOPI   6.283185307

#define	MAXPACKET	1200
#define	MAXGATES		2000  
//#define	MAXGATES		1000 //!!! test WinDSP IQDATA wraparound problem. see notes. 
#define MAXHITS 1024
#define PRODS_ELEMENTS  16      /* number of elements in prods array */
#define	DATA_TIMEOUT	1

/* define UDP packet types */
#define	UDPTYPE_CMD		1
#define	UDPTYPE_IQDATA	2
#define	UDPTYPE_ABPDATA	3
#define	UDPTYPE_PRQDATA	4
// for consistency make these equal to config dataformat values: 
#define	UDPTYPE_PIRAQ_ABPDATA				16
#define	UDPTYPE_PIRAQ_ABPDATA_STAGGER_PRT	17
#define	UDPTYPE_PIRAQ_CP2_TIMESERIES		18

/* global define of number of records in the various FIFO's */
#define	CMD_FIFO_NUM		5
//original: #define	PIRAQ_FIFO_NUM		250
//#define	PIRAQ_FIFO_NUM		 42		// fewer, larger packets.
//#define	PIRAQ_FIFO_NUM		58		// 136KB packets in 8MB shared memory
#define	PIRAQ_FIFO_NUM		120		// CP2: 64KB packets in 8MB shared memory
#define	IQDATA_FIFO_NUM		450 
#define	ABPDATA_FIFO_NUM	20

/* define the FIFO notification port numbers */
#define	CMD_RING_PORT		2000
#define	PIRAQ_RING_PORT		2100
#define	IQDATA_RING_PORT	2200
#define	ABPDATA_RING_PORT	2300

/* define the data communications port numbers */
#define	CMD_DATA_PORT			21050
#define	PIRAQ_DATA_PORT		    21055
#define	IQDATA_DATA_PORT		21060
#define	ABPDATA_DATA_PORT	21065

/****************************************************/
/* PIRAQ status register definitions and functions */
/****************************************************/

/* the bits of the status register */
#define STAT0_SW_RESET    0x0001
#define STAT0_TRESET      0x0002
#define STAT0_TMODE       0x0004
#define STAT0_DELAY_SIGN  0x0008
#define STAT0_EVEN_TRIG   0x0010
#define STAT0_ODD_TRIG    0x0020
#define STAT0_EVEN_TP     0x0040
#define STAT0_ODD_TP      0x0080
#define STAT0_PCLED       0x0100
#define STAT0_GLEN0       0x0200
#define STAT0_GLEN1       0x0400
#define STAT0_GLEN2       0x0800
#define STAT0_GLEN3       0x1000
//#define STAT0_GLEN4       0x1000
//#define STAT0_GATE0MODE   0x1000
//#define STAT0_SAMPLE_CTRL 0x1000

#define STAT1_PLL_CLOCK   0x0001
#define STAT1_PLL_LE      0x0002
#define STAT1_PLL_DATA    0x0004
#define STAT1_WATCHDOG    0x0008
#define STAT1_PCI_INT     0x0010
#define STAT1_EXTTRIGEN   0x0020
#define STAT1_FIRST_TRIG  0x0040
#define STAT1_PHASELOCK   0x0080
#define STAT1_SPARE0      0x0100
#define STAT1_SPARE1      0x0200
#define STAT1_SPARE2      0x0400
#define STAT1_SPARE3      0x0800


/* macro's for writing to the status register */
//#define STATUS0(a)        *piraq->status0 = (a)
//#define STATUSRD0(a)      (*piraq->status0 & (a))
//#define STATSET0(a)       *piraq->status0 |= (a)
//#define STATCLR0(a)       *piraq->status0 &= ~(a)
//#define STATTOG0(a)       *piraq->status0 ^= (a)
//#define STATUS1(a)        *piraq->status1 = piraq->stat1 = (a)
//#define STATUSRD1(a)      (((*piraq->status1 & 0xFF) | (piraq->stat1 & 0xFF00)) & (a))
//#define STATSET1(a)       STATUS1(STATUSRD1(0xFFFF) |  (a))
//#define STATCLR1(a)       STATUS1(STATUSRD1(0xFFFF) & ~(a))
//#define STATTOG1(a)       STATUS1(STATUSRD1(0xFFFF) ^  (a))

/* macro's for writing to the status register */
#define STATUS0(card,a)        (card->GetControl()->SetValue_StatusRegister0(a)) // *card->status0 = (a)
#define STATUSRD0(card,a)      (card->GetControl()->GetBit_StatusRegister0(a)) // (*card->status0 & (a))
#define STATSET0(card,a)       (card->GetControl()->SetBit_StatusRegister0(a))  // *card->status0 |= (a)
#define STATCLR0(card,a)       (card->GetControl()->UnSetBit_StatusRegister0(a)) // *card->status0 &= ~(a)
#define STATTOG0(card,a)       (if(STATUSRD0(card,a)) { STATCLR0(card,a);}else{STATSET0(card,a);}) // *card->status0 ^= (a)
#define STATUS1(card,a)        (card->GetControl()->SetValue_StatusRegister1(a)) // *card->status0 = (a)
#define STATUSRD1(card,a)      (card->GetControl()->GetBit_StatusRegister1(a)) // (*card->status0 & (a))
#define STATSET1(card,a)       (card->GetControl()->SetBit_StatusRegister1(a))  // *card->status0 |= (a)
#define STATCLR1(card,a)       (card->GetControl()->UnSetBit_StatusRegister1(a)) // *card->status0 &= ~(a)
#define STATTOG1(card,a)       if(STATUSRD0(card,a)) { STATCLR1(card,a);}else{STATSET1(card,a);} // *card->status0 ^= (a)
#define STATPLL(card,a)        STATUS1(card,STATUSRD1(card,0xFFF8) |  (a))


/* FIR filter register definitions */
#define	APATH_REG0		0x00
#define	APATH_REG1		0x01
#define	BPATH_REG0		0x02
#define	BPATH_REG1		0x03
#define	CASCADE_REG	0x04
#define	COUNTER_REG	0x05
#define	GAIN_REG		0x06
#define	OUTPUT_REG	0x07
#define	SNAP_REGA		0x08
#define	SNAP_REGB		0x09
#define	SNAP_REGC		0x0A
#define	ONE_SHOT		0x0B
#define	NEW_MODES		0x0C
#define	PATHA_COEFF	0x80
#define	PATHB_COEFF	0xC0

/****************************************************/
/* define the various data formats                         */
/****************************************************/

#define DATA_SIMPLEPP   0 /* simple pulse pair ABP */
#define DATA_POLYPP     1 /* poly pulse pair ABPAB */
#define DATA_DUALPP     2 /* dual prt pulse pair ABP,ABP */
#define DATA_POL1       3 /* dual polarization pulse pair ABP,ABP */
#define DATA_POL2       4 /* more complex dual polarization ??????? */
#define DATA_POL3       5 /* almost full dual polarization with log integers */
#define DATA_SIMPLE16   6 /* 16 bit magnitude/angle representation of ABP */
#define DATA_DOW        7 /* dow data format */
#define DATA_FULLPOL1   8 /* 1998 full dual polarization matrix for alternating H-V */
#define DATA_FULLPOLP   9 /* 1999 full dual polarization matrix plus for alternating H-V */
#define DATA_MAXPOL    10 /* 2000 full dual polarization matrix plus for alternating H-V */
#define DATA_HVSIMUL   11 /* 2000 copolar matrix for simultaneous H-V */
#define DATA_SHRTPUL   12 /* 2000 full dual pol matrix w/gate averaging no Clutter Filter */
#define DATA_SMHVSIM   13 /* 2000 DOW4 copolar matrix for simultaneous H-V (no iq average) */
#define	DATA_STOKES	   14 /* NEC Stokes parameters */
#define	DATA_FFT	   15 /* NEC FFT mean velocity and power */
#define	PIRAQ_ABPDATA  16 /* ABP data computed in piraq3: rapidDOW project */
#define	PIRAQ_ABPDATA_STAGGER_PRT 17 /* Staggered PRT ABP data computed in piraq3: rapidDOW project */
#define	PIRAQ_CP2_TIMESERIES 18 /* Simple timeseries: CP2 project */
#define DATA_TYPE_MAX PIRAQ_CP2_TIMESERIES /* limit of data types */ 

#define	tptest()		if(teststate)   {err++; printf("expecting \"delay\" or \"width\" keyword in line %d\n",linenum); teststate = 0;}
#define	stptest()	if(stepstate)   {err++; printf("expecting \"pulseenable\", \"period\", \"polarization\", or \"phasedata\", keyword in line %d\n",linenum); stepstate = 0;}


/****************************************************/
/* set up the infra structure for software FIFO's  */
/****************************************************/
#pragma pack(4) // set 4-byte alignment of structures for compatibility w/destination computers
typedef	struct
	{
	char	name[80];	/* name that identifies the type of FIFO buffer */
	int	magic;	/* magic number indicating the FIFO has been initialized */
	int	size;		/* total size of allocated memory for this FIFO */
//GRG
//!!!	int	fifodes;	/* unique sequencial integer for resolving multiple FIFO's */
	int	header_off;	/* offset to the user header (can't use absolute address here) */
	int	fifobuf_off;	/* offset to fifo base address */
	int	record_size;	/* size in bytes of each FIFO record */
	int	record_num;	/* number of records in FIFO buffer */
	int	head,tail;	/* indexes to the head and tail records */
	int	destroy_flag;	/* destroy-when-empty flag */
	int sock;				/* socket file descriptor for notification communications */
    int port;				/* UDP (datagram) socket port for notification */
    int clients;
//	int	write_process;	/* process number of writing routine */
//	int	read_process;	/* process number of reading routine */
	} FIFO;

typedef struct {
    uint4 magic;             /* must be 'MAGIC' value above */
    uint4 type;             /* e.g. DATA_SIMPLEPP, defined in piraq.h */
    uint4 sequence_num;     /* increments every beam */
    uint4 totalsize;      /* total amount of data only (don't count the size of this header) */
    uint4 pagesize;       /* amount of data in each page INCLUDING the size of the header */
    uint4 pagenum;		/* packet number : 0 - pages-1 */
    uint4 pages;     /* how many 'pages' (packets) are in this group */
} UDPHEADER;


/****************************************************/
/* set up the infrastructure for intraprocess communication */
/****************************************************/

typedef struct {
	int		type;
	int		count;
	int		flag;		/* done, new, old, handshake, whatever, ..... */
	int		arg[5];
	}	COMMAND;
		
typedef struct {
	UDPHEADER		udphdr;
	char				buf[MAXPACKET];
	} UDPPACKET;

typedef struct {
	UDPHEADER		udphdr;
	int					cnt;
	} FIFORING;


/****************************************************/
/* completely define radar operation with a single structure */
/****************************************************/

#define	CTRL_CLUTTERFILTER		0x00000001
#define	CTRL_TIMESERIES			0x00000002
#define	CTRL_PHASECORRECT		0x00000004
#define	CTRL_SCAN_TRANSITION		0x00000008
#define	CTRL_TXON				0x00000010
#define	CTRL_SECONDTRIP			0x00000020
#define	CTRL_VELOCITYFOLD		0x00000040
#define	CTRL_CALMODE				0x00000080
#define	CTRL_CALSIMPLE			0x00000100
#define CTRL_OBSERVING          0x00000200

typedef	struct {
	char		type;				// dsp type 0 = AB  1 = P  2 = stokes
    char		start;				// starting at this hit referenced to the begining of the sequence
	char		Apol,Bpol;			// polarizations of the two halves of a pair (numbered H,V,+,-,R,L)
	char		Astep,ABstep;		// step from one A to the next and step from A to B
    } PRIMARYDSP;


#define MAXNUM 1200

#define PIRAQX_CURRENT_REVISION 1
//struct piraqX_header_rev1
typedef struct 
{
#if 1
		/* all elements start on 4-byte boundaries
         * 8-byte elements start on 8-byte boundaries
         * character arrays that are a multiple of 4
         * are welcome
         */
    char desc[4];			/* "DWLX" */
    uint4 recordlen;        /* total length of record - must be the second field */
    uint4 channel;          /* e.g., RapidDOW range 0-5 */
    uint4 rev;		        /* format revision #-from RADAR structure */
    uint4 one;			    /* always set to the value 1 (endian flag) */
    uint4 byte_offset_to_data;
    uint4 dataformat;

    uint4 typeof_compression;	/*  */
/*
      Pulsenumber (pulse_num) is the number of transmitted pulses
since Jan 1970. It is a 64 bit number. It is assumed
that the first pulse (pulsenumber = 0) falls exactly
at the midnight Jan 1,1970 epoch. To get unix time,
multiply by the PRT. The PRT is a rational number a/b.
More specifically N/Fc where Fc is the counter clock (PIRAQ_CLOCK_FREQ),
and N is the divider number. So you can get unix time
without roundoff error by:
secs = pulsenumber * N / Fc. The
computations is done with 64 bit arithmatic. No
rollover will occur.

The 'nanosecs' field is derived without roundoff
error by: 100 * (pulsenumber * N % Fc).

Beamnumber is the number of beams since Jan 1,1970.
The first beam (beamnumber = 0) was completed exactly
at the epoch. beamnumber = pulsenumber / hits. 
*/
    

#ifdef _TMS320C6X   /* TI doesn't support long long */
    uint4 pulse_num_low;
    uint4 pulse_num_high;
#else
    uint8 pulse_num;   /*  keep this field on an 8 byte boundary */
#endif
#ifdef _TMS320C6X   /* TI doesn't support long long */
    uint4 beam_num_low;
    uint4 beam_num_high;
#else
    uint8 beam_num;	/*  keep this field on an 8 byte boundary */
#endif
    uint4 gates;
    uint4 start_gate;
    uint4 hits;
/* additional fields: simplify current integration */
    uint4 ctrlflags; /* equivalent to packetflag below?  */
    uint4 bytespergate; 
    float4 rcvr_pulsewidth;
#define PX_NUM_PRT 4
    float4 prt[PX_NUM_PRT];
    float4 meters_to_first_gate;  

    uint4 num_segments;  /* how many segments are we using */
#define PX_MAX_SEGMENTS 8
    float4 gate_spacing_meters[PX_MAX_SEGMENTS];
    uint4 gates_in_segment[PX_MAX_SEGMENTS]; /* how many gates in this segment */
    
    

#define PX_NUM_CLUTTER_REGIONS 4
    uint4 clutter_start[PX_NUM_CLUTTER_REGIONS]; /* start gate of clutter filtered region */
    uint4 clutter_end[PX_NUM_CLUTTER_REGIONS];  /* end gate of clutter filtered region */
    uint4 clutter_type[PX_NUM_CLUTTER_REGIONS]; /* type of clutter filtering applied */

#define PIRAQ_CLOCK_FREQ 10000000  /* 10 Mhz */

/* following fields are computed from pulse_num by host */
    uint4 secs;     /* Unix standard - seconds since 1/1/1970
                       = pulse_num * N / ClockFrequency */
    uint4 nanosecs;  /* within this second */
    float4 az;   /* azimuth: referenced to 9550 MHz. possibily modified to be relative to true North. */
    float4 az_off_ref;   /* azimuth offset off reference */ 
    float4 el;		/* elevation: referenced to 9550 MHz.  */ 
    float4 el_off_ref;   /* elevation offset off reference */ 

    float4 radar_longitude;
    float4 radar_latitude;
    float4 radar_altitude;
#define PX_MAX_GPS_DATUM 8
    char gps_datum[PX_MAX_GPS_DATUM]; /* e.g. "NAD27" */
    
    uint4 ts_start_gate;   /* starting time series gate , set to 0 for none */
    uint4 ts_end_gate;     /* ending time series gate , set to 0 for none */
    
    float4 ew_velocity;

    float4 ns_velocity;
    float4 vert_velocity;

    float4 fxd_angle;		/* in degrees instead of counts */
    float4 true_scan_rate;	/* degrees/second */
    uint4 scan_type;
    uint4 scan_num;
    uint4 vol_num;

    uint4 transition;
    float4 xmit_power;

    float4 yaw;
    float4 pitch;
    float4 roll;
    float4 track;
    float4 gate0mag;  /* magnetron sample amplitude */
    float4 dacv;
    uint4  packetflag; 

    /*
    // items from the depricated radar "RHDR" header
    // do not set "radar->recordlen"
    */

    uint4 year;             /* e.g. 2003 */
    uint4 julian_day;
    
#define PX_MAX_RADAR_NAME 16
    char radar_name[PX_MAX_RADAR_NAME];
#define PX_MAX_CHANNEL_NAME 16
    char channel_name[PX_MAX_CHANNEL_NAME];
#define PX_MAX_PROJECT_NAME 16
    char project_name[PX_MAX_PROJECT_NAME];
#define PX_MAX_OPERATOR_NAME 12
    char operator_name[PX_MAX_OPERATOR_NAME];
#define PX_MAX_SITE_NAME 12
    char site_name[PX_MAX_SITE_NAME];
    

    uint4 polarization;
    float4 test_pulse_pwr;
    float4 test_pulse_frq;
    float4 frequency;

    float4 noise_figure;
    float4 noise_power;
    float4 receiver_gain;
    float4 E_plane_angle;  /* offsets from normal pointing angle */
    float4 H_plane_angle;
    

    float4 data_sys_sat;
    float4 antenna_gain;
    float4 H_beam_width;
    float4 V_beam_width;

    float4 xmit_pulsewidth;
    float4 rconst;
    float4 phaseoffset;

    float4 zdr_fudge_factor;

    float4 mismatch_loss;
    float4 rcvr_const;

    float4 test_pulse_rngs_km[2];
    float4 antenna_rotation_angle;   /* S-Pol 2nd frequency antenna may be 30 degrees off vertical */
    
#define PX_SZ_COMMENT 64
    char comment[PX_SZ_COMMENT];
    float4 i_norm;  /* normalization for timeseries */
    float4 q_norm;
    float4 i_compand;  /* companding (compression) parameters */
    float4 q_compand;
    float4 transform_matrix[2][2][2];
    float4 stokes[4]; 
    
    
    float4 i_offset;  /* dc offset, one for each channel? */
    float4 q_offset;
    float4 vi_offset; 
    float4 vq_offset;

    float4 vnoise_power;	/* 'v' second piraq channel */ 
    float4 vreceiver_gain;
    float4 vaz_off_ref;      /* SynthAngle output for 'v' channel */
    float4 vel_off_ref;
    float4 vfrequency;
//    float4 spare[20];
    float4 spare[11];

    /*
    // always append new items so the alignment of legacy variables
    // won't change
    */
#endif

} INFOHEADER;

typedef struct piraqX_header_rev1 PIRAQX;

///* this structure gets recorded for each dwell */
//typedef struct  {                
////		PIRAQX          header;
//		INFOHEADER      header;
//		float           abp[MAXNUM * 4]; /* a,b,p + time series */
//} XDWELL;

/****************************************************/
/* now use this infrastructure to define the records in the */
/* various types of FIFO's used for interprocess communication */
/****************************************************/

typedef struct {		/* data that's in the PIRAQ1 FIFO */
	INFOHEADER		info;
	float				data[MAXGATES * 12];
	} DATABLOCK;

typedef struct {
	UDPHEADER	udp;
	COMMAND		cmd;
	DATABLOCK	data;
    } PACKET;

typedef struct {			/* this structure must match the non-data portion of the PACKET structure */
	UDPHEADER	udp;
	COMMAND		cmd;
	INFOHEADER	info;
	} PACKETHEADER;

struct synth { /* array of structures containing SynthAngle output for all channels */ 
	float el_off_ref; 
	float az_off_ref; 
	float xmit_freq; 
}; 

//synthinfo_ SYNTHINFO[CHANNELS]; 

#pragma pack(8) // return to default 8-byte alignment of structures 

#define	HEADERSIZE		sizeof(PACKETHEADER)
#define	IQSIZE			(sizeof(short) * 4 * MAXGATES) 
#define	ABPSIZE			(sizeof(float) * 12 * MAXGATES)
#define	DATASIZE(a)		(a->data.info.gates * a->data.info.bytespergate)
#define	RECORDLEN(a)		(sizeof(INFOHEADER) + (DATASIZE(a)))
#define	TOTALSIZE(a)		(sizeof(COMMAND) + (RECORDLEN(a)))
//???:#define	TOTALSIZE(a)		(sizeof(UDPHEADER) + sizeof(COMMAND) + (RECORDLEN(a)))

// CP2: 
#define BUFFER_EPSILON	0	// CP2 space between hits in buffer and within N-hit PCI packet

#define	SET(t,b)			(t |= (b))
#define	CLR(t,b)			(t &= ~(b))

#define DEG2RAD (M_PI/180.0)

#define DIO_OUT	 0
#define DIO_IN	 1

/************************ timer card structures ***********************/
#define	MAXSEQUENCE		38
#define	MAXTESTPULSE		6

typedef 	union {	unsigned int		himedlo;
	   			struct {unsigned char		lo,med,hi,na;} byte;} HIMEDLO;

typedef 	union {	unsigned short	hilo;
	   			struct {unsigned char	lo,hi;} byte;} HILO;

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
	HILO				seqdelay,sync;
	int				timingmode,seqlen;
	float			clockfreq,reffreq,phasefreq;
	char				*base;
	}	TIMER;

#define	TIMER_CONTINUOUS		0
#define	TIMER_TRIGGERED		1
#define	TIMER_SYNC			2

#ifdef ALLEGRO_H
// The vector struct
typedef struct vector_typ{
	double x,y,angle,length,x2,y2,x3,x4,y3,y4,bx,by,bx2,by2;
	BITMAP *dirty;
	unsigned int color;
	struct vector_typ *next,*prev;
}VECTOR;
#endif

/*************************************************************/
/* structures for hardware and configuration specific parameters */
/*************************************************************/

/* structure to define all parameters that are PIRAQ related */
typedef struct
	{
	short  *FIRCH1_I,*FIRCH1_Q,*FIRCH2_I,*FIRCH2_Q;
	unsigned short  *timer,*status0,*status1;
	unsigned short  *HPICLO,*HPICHI,*HPIALO,*HPIAHI,*HPIDLO,*HPIDHI;
	FIFO				*fifo;
	int	            stat1; /* remember the data in the status register */
    unsigned int 	base_add,fifoadd;
	int             *phase,*dac;
	int             imask; /* istat mask */
	int             intnum,irqmask;   /* PC hardware IRQ, PC PIC mask */
	int             boardnumber;  /* for multiple piraq boards */
	int             eofstatus,hitcount;
	UINT64       oldbeamnum,beamnum,pulsenum;
	float           numdiscrim,dendiscrim,g0invmag;
	float           *ts,*abp,*delay_ptr;
	float           pllfreq;
    } PIRAQ_INFO;

/* structure to completely define the operation of the PIRAQ */
/* and data system at the lowest level */
typedef struct
	{
	int     gatesa, gatesb, hits, rcvr_pulsewidth, timeseries, gate0mode, startgate;
	int     prt, timingmode, delay, sync;
	int     ts_start_gate, ts_end_gate, pcorrect, clutterfilter, xmit_pulsewidth; /* xmit_pulsewidth used for discrim only */
	int		clutter_start, clutter_end; 
	int     prt2, velsign, hitcount;
	char    outfilename[80], outpath[80];
	float   afcgain, afchi, afclo, locktime;
	int     trigger, testpulse, tpwidth, tpdelay, intenable;
	float   stalofreq;
	signed char     int_phase;
	float           int_dac;
	unsigned int    int_sync, int_period;
	unsigned int    ethernet;  /* the ethernet address */
	int     dataformat;
	int     boardnum;  /* which of n cards are we refering to */
	float	meters_to_first_gate; 
	float	gate_spacing_meters; 
	} CONFIG;

/* structure gets filled from a file to specify default radar parameters */
/* modified: channel-specific receiver gain, noise power, etc. vreceiver_gain, etc., removed 1-10-05 mp */ 
typedef struct  {
		char    desc[4];
		short   recordlen;
		short   rev;
		short   year;           /* this is also in the dwell as sec from 1970 */
		char    radar_name[PX_MAX_RADAR_NAME];
		char    polarization;   /* H or V */
		float   test_pulse_pwr; /* power of test pulse (refered to antenna flange) */
		float   test_pulse_frq; /* test pulse frequency */
		float   frequency;      /* transmit frequency */
		float   peak_power;     /* typical xmit power (at antenna flange) read from config.rdr file */
		float   noise_figure;
		float   noise_power[CHANNELS];    /* for subtracting from data */
		float   receiver_gain[CHANNELS];  /* chan gain from antenna flange to VIRAQ input */
		float   data_sys_sat;   /* VIRAQ input power required for full scale */
		float   antenna_gain;
		float   horz_beam_width;
		float   vert_beam_width;
		float   xmit_pulsewidth; /* transmitted pulse width */
		float   rconst;         /* radar constant */
		float   phaseoffset;    /* offset for phi dp */
//		float   vreceiver_gain; /* ver chan gain from antenna flange to VIRAQ */
		float   vtest_pulse_pwr; /* ver test pulse power refered to antenna flange */
		float   vantenna_gain;  /* vertical antenna gain */
//		float   vnoise_power;   /* for subtracting from data */
		float   zdr_fudge_factor; /* what else? */
		float   missmatch_loss;   /* receiver missmatch loss (positive number) */
		float	E_plane_angle;   /* E field plane angle */ 
		float	H_plane_angle;   /* H field plane angle */ 
	    float	antenna_rotation_angle;   /* S-Pol 2nd frequency antenna may be 30 degrees off vertical */
		float	latitude; 
		float	longitude;
		float	altitude;
		float	i_offset;		/* dc offset to remove from I data; currently stored in data.info.spare[0] for use py piraq */ 
		float	q_offset;		/* dc offset to remove from Q data; currently stored in data.info.spare[0] for use py piraq */ 
		float   misc[3];        /* 3 more misc floats */
		char	channel_name[PX_MAX_CHANNEL_NAME];
	    char	project_name[PX_MAX_PROJECT_NAME];
		char	operator_name[PX_MAX_OPERATOR_NAME];
	    char	site_name[PX_MAX_SITE_NAME];
		char    text[960];		/* size as it was before channel-specific additions: no need to change */ 
		} RADAR;

// defines for build only; taken from display globals.h: 
#define PRODS_ELEMENTS  16      // number of elements in prods array 
#define MAXPLANES  22               // number of display planes (there is one for the overlay and one for the screen) 

// structure that defines communication parameters to the display 
typedef struct
	{
	int     displayparm,fakeangles,recording,type;
	double  threshold,rgpp,offset;
	double  dbzhi,dbzlo,powlo,powhi,zdrlo,zdrhi,phaselo,phasehi;
	int     mousex,mousey,mousebutton,mousegate;
	double  mouserange,mouseangle;
	int     xc,yc;  // location of radar in pixel coordinates 
	char    title[40];
	double  lat,lon,range,angle,g0off,gatelen;
	void    *plane[MAXPLANES];     // display and overlay bitmaps (0 = overlay) 
	int     dispnum;        // number of parameters calculated 
	int     dispscale[PRODS_ELEMENTS]; // which products get which display parameters
	int		fullscreen;
	} DISPLAY;

/****************************************************/
/* function prototypes used for radar processor system */
/****************************************************/

/* shared_mem.c */
void *shared_mem_create(char *name, int size);
void *shared_physmem_create(char *name, int size,int physaddr);
void *shared_mem_open(char *name);
void shared_mem_close(char *name, void *addr, int size);
int  shared_mem_size(char *name);
int	shared_mem_exhist(char *name);

/* fifo.c */
FIFO *fifo_create(char *name, int headersize, int recordsize, int recordnum);
//!!!
void piraq_fifo_init(FIFO * fifo, char *name, int headersize, int recordsize, int recordnum); 
FIFO *fifo_open(char *name);
int fifo_close(FIFO *fifo);
void fifo_clear(FIFO *fifo);
void *fifo_get_last_address(FIFO *fifo);
void *fifo_index_from_last(FIFO *fifo, int index);
int fifo_increment_head(FIFO *fifo);
void fifo_notify(FIFO *fifo);
void *fifo_get_read_address(FIFO *fifo, int offset);
void *fifo_get_write_address(FIFO *fifo);
int fifo_increment_tail(FIFO *fifo);
int fifo_hit(FIFO *fifo);
int fifo_exhist(char *name);
void *fifo_get_header_address(FIFO *fifo);
   
/* udpsock.c */
int open_udp_out(char *name);
unsigned int send_udp_packet(int sock, int port, unsigned int sequence_num, UDPHEADER *udp);
int receive_udp_packet(int sock, UDPHEADER *udp, int maxbufsize);
int open_udp_in(int port);
void close_udp(int sock);

/* keylib.c */
int	keyvalid(void);
int	keyexit(void);
int keyraw(void);

/* glib.c */
void    gmode(void);
void    tmode(void);
void    plot(int x,int y,int c)
;
void    gclear(void)
;
void    setcolor(int color);
void    glibinit(void);

void    undisp(void)
;
void    disp(float data[], int n)
;
void    pdisp(float data[], int n);
void    cursor(double xfract, double data)
;
void    uncursor(double xfract, double data)
;
void    display_axis(void)
;
void    drawgraphscale(char *xbuf,double xlo, double xhi, char *ybuf, double ylo, double yhi)
;
void    gmove(int x,int y)
;
void    draw(int x,int y)
;
void    grputs(int x,int y,const char *string)
;
void    avedisp(float array[],int n,double filter)
;
void    polardisp(float array[],int n,double filter)
;

/* structinit.c */
void struct_init(INFOHEADER *h, char *fname);

/* subs.c */
int timerset(CONFIG *config, PIRAQ *piraq)
;
void timer(int timernum,int timermode,int count,unsigned short *iobase)
;
void    stop_piraq(CONFIG *config,PIRAQ *piraq)
;
//!!!int start(CONFIG *config,PIRAQ *piraq)
int start(CONFIG *config,PIRAQ *piraq,PACKET * pkt)
;
void    setpiraqptr(PIRAQ *piraq, int n)
;
void pll(double ref, double freq, double cmpfreq, PIRAQ *piraq)
;
void plldata(int data, PIRAQ *piraq)
;
int  pwait(CONFIG *config,PIRAQ *piraq);
UINT64 calc_pulsenum(int seconds,int  pri)
;
UINT64 calc_dwellcount(int seconds, int  pri, int hits)
;
int calc_hitcount(int seconds, int  pri, int hits)
;
int     calc_delay(int seconds,int  pri)
;
void computetime(CONFIG *config, UINT64 beamnum,unsigned int *time,unsigned short *subsec,UINT64 *pulsenum)
;
int hpird(PIRAQ *piraq, int address);
void hpiwrt(PIRAQ *piraq, int address, int tdata)
;
void FIR_gaussian(CONFIG *config, PIRAQ *piraq, float tbw, float pw);
double	sinc(double arg);
void FIR_halfband(CONFIG *config, PIRAQ *piraq, int decimation);
int	map_piraq(PIRAQ *piraq, int addr);

/* products.c */
void products(DATABLOCK *beam, float *prods)
;
void simplepp(DATABLOCK *beam, float *prods);

/* fft.c */
void fft(complex c[],int n);
void arcfft(complex c[], int n);
void hann(complex a[], int n);
void hamm(complex a[], int n);
void dolookup(int    n);

/* dsplib.c */
void dsp_stokes(DATABLOCK *a, FIFO *b)
;
void dsp_simplefft(DATABLOCK *a, FIFO *b)
;
void dsp(DATABLOCK *abpdata, FIFO *fifo)
;
void dsp_simplepp(DATABLOCK *abpdata, FIFO *fifo)
;
void dsp_hvsimul(DATABLOCK *abpdata,FIFO *fifo)
;
void correctphase(int gates, float *iqptr)
;
void c_filter(int gates, float *coeff, float *delay_ptr, float *iqptr)
;
void pulsepair(int a, int gates,short *iqptr,short *lagbuf,float *abp)
;
void hvpulsep(int gates,short *hviqptr,short *lagbuf,float *hvabp)
;
void dsp_simple16(DATABLOCK *abpdata,FIFO *fifo)
;
void fftmoments(complex *a, DATABLOCK *b, float *c)
;
void stokes(int a, int b, short *c, float d, float *e);

/* pci.c */
int setup_pci(void);	// Sets up the PCI hardware
PCI_CARD *init_pci_card(int deviceid,int vendorid,int n);	// Initializes a card on the PCI bus
int pci_card_base(PCI_CARD *pcicard, int reg, int size);

void shutdown_pci(void);

/* dio.c */
float to_angle(unsigned char byte1,unsigned char byte2);
void to_bytes(float angle,unsigned char *byte1,unsigned char *byte2);
int get_dio_base(PCI_CARD *dio);
void set_dio_mode(PCI_CARD *dio,int mode);
int write_angles(PCI_CARD *dio,float az,float el);
int read_angles(PCI_CARD *card,float *az,float *el);
int set_oper(int pattern, int mask);

/* structinit stuff */
void readradar(char *fname, RADAR *radar);
RADAR   *initradar(char *filename);
void readconfig(char *fname, CONFIG *config);
void readdisplay(char *fname, DISPLAY *disp);

/* digitalrcv.c */
int	important_change(INFOHEADER *old, INFOHEADER *New);

/* timerlib.c */
void start_timer_card(TIMER *timer,INFOHEADER *header); 
void timer_start(TIMER *timer);
void timer_stop(TIMER *timer);
void timer_reset(TIMER *timer);
void timer_dmp(TIMER *timer);
void timer_set(TIMER *timer);
int timer_init(TIMER *timer, int boardnumber);
void timer_read(char *fname, TIMER *timer);
int timer_config(TIMER *timer, INFOHEADER *info);
int	half_second_wait(void);
int	timer_test(TIMER *timer); 

// structinit stuff
void set(char *str, char *fmt, void *parm, char *keyword, int line, char *fname);
int parse(char *keyword,char *parms[]);
void getkeyval(char *line,char *keyword,char *value);

#ifdef ALLEGRO_H
/* vector.c  Prototypes */
VECTOR *CreateVector(double x,double y,double angle, double length);
void DestroyVector(VECTOR *v);
void UpdateDirtyVector(VECTOR *v,BITMAP *dest);
void DrawVector(VECTOR *v,BITMAP *dest);
void DrawDirtyVector(VECTOR *v,BITMAP *dest);
void AddVector(VECTOR *head,VECTOR *add);
void RemoveVector(VECTOR *v);
void DrawVectorList(VECTOR *head,BITMAP *dest);
void DrawDirtyVectorList(VECTOR *head,BITMAP *dest);
void UpdateDirtyVectorList(VECTOR *head,BITMAP *dest);
void DestroyVectorList(VECTOR *head);
#endif

/* UNIX standard file#s */ 

#ifndef STDIN_FILENO
#define STDIN_FILENO	0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO	1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO	2
#endif

