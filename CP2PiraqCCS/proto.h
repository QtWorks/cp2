#ifndef _PROTO_H_
#define _PROTO_H_

#include "dd_types.h"

#define PCIBASE		0x3000000

#define	MAGIC		0x12345678

#define NEW_INCLUDE // new definitions for RapidDOW DWELL, UDPHEADER, 4-byte data alignment. 
#define STRUCTURE_ALIGNMENT 8 // must be a power of 2

#define	PIRAQ_DEVICE_ID 	0x9054
#define	PIRAQ_VENDOR_ID		0x10B5   

#define	COUNTFREQ	8000000

#define	true	1
#define	false	0

void
dmaTransfer(int channel, 
			int *src, 
			int *dst, 
			int transferCount,
			int globalReload); 

#define	UDPSENDSIZE	(unsigned int)65536	//	size of N-hit packets in system: PIRAQ through Qt applications

// 3 modes of data transfer from PIRAQ to host: first 2 are diagnostic
#define	SEND_CHA		0		// send CHA
#define	SEND_CHB		1		// send CHB
#define	SEND_COMBINED	2		// execute dynamic-range extension algorithm; send resulting combined data
#define	CHMODE_MASK		0x03	// extract channel mode bits

// 4 PIRAQ test-sinusoid frequency adjustments
#define	INCREMENT_TEST_SINUSIOD_COARSE	4
#define	INCREMENT_TEST_SINUSIOD_FINE	8
#define	DECREMENT_TEST_SINUSIOD_COARSE	0x0c
#define	DECREMENT_TEST_SINUSIOD_FINE	0x10
#define	FREQADJ_MASK	0x1c	// extract frequency-adjust bits

// 4 PIRAQ test-sinusoid amplitude adjustments
#define	INCREMENT_TEST_AMPLITUDE_COARSE	0x20
#define	INCREMENT_TEST_AMPLITUDE_FINE	0x40
#define	DECREMENT_TEST_AMPLITUDE_COARSE	0x60
#define	DECREMENT_TEST_AMPLITUDE_FINE	0x80
#define	AMPLADJ_MASK	0xe0	// extract amplitude-adjust bits

typedef struct pci_card_typ{
	int fnum;
	int bus;
	unsigned int base;
} PCI_CARD;

//typedef struct  {float x,y;}   complex;

//#define K2      0.93
//#define C       2.99792458E8
/* #define M_PI    3.141592654 */
#define		PI				3.1415926
//#define TWOPI   6.283185307

#define	MAXPACKET	1200
// DEBUG #define	MAXGATES	2000
//#define	MAXGATES	1000
#define	MAXGATES	400		// Added to test speed; most you can do w/clutterfilter
#define MAXHITS 1024
#define PRODS_ELEMENTS  16      /* number of elements in prods array */
#define	ERROR	-1
#define	DATA_TIMEOUT	1

/* define UDP packet types */
#define	UDPTYPE_CMD		1
#define	UDPTYPE_IQDATA	2
#define	UDPTYPE_ABPDATA	3

/* global define of number of records in the various FIFO's */
#define	CMD_FIFO_NUM			5
#define	PIRAQ_FIFO_NUM		250
#define	IQDATA_FIFO_NUM		450
#define	ABPDATA_FIFO_NUM	20

/* define the FIFO notification port numbers */
#define	CMD_RING_PORT		2000
#define	PIRAQ_RING_PORT		2100
#define	IQDATA_RING_PORT		2200
#define	ABPDATA_RING_PORT	2300

/* define the data communications port numbers */
#define	CMD_DATA_PORT			21050
#define	PIRAQ_DATA_PORT		21055
#define	IQDATA_DATA_PORT		21060
#define	ABPDATA_DATA_PORT	21065

/****************************************************/
/* set up the infra structure for software FIFO's  */
/****************************************************/

typedef	struct 
	{
	char	name[80];	/* name that identifies the type of FIFO buffer */
	int	magic;	/* magic number indicating the FIFO has been initialized */
	int	size;		/* total size of allocated memory for this FIFO */
	int	header_off;	/* offset to the user header (can't use absolute address here) */
	int	fifobuf_off;	/* offset to fifo base address */
	int	record_size;	/* size in bytes of each FIFO record */
	int	record_num;	/* number of records in FIFO buffer */
	int	head,tail;	/* indexes to the head and tail records */
	int	destroy_flag;	/* destroy-when-empty flag */
	int sock;				/* socket file descriptor for notification communications */
    int port;				/* UDP (datagram) socket port for notification */
    int clients;
	} FIFO;

#ifdef NEW_INCLUDE // new UDPHEADER 
// !!! REPLACE int w/uint4 below: 
typedef struct udp_header{
    int magic;             /* must be 'MAGIC' value above */
    int type;             /* e.g. DATA_SIMPLEPP, defined in piraq.h */
    int sequence_num;     /* increments every beam */
    int totalsize;      /* total amount of data only (don't count the size of this header) */
    int pagesize;       /* amount of data in each page INCLUDING the size of the header */
    int pagenum;		/* packet number : 0 - pages-1 */
    int pages;     /* how many 'pages' (packets) are in this group */
} UDPHEADER;
#pragma STRUCT_ALIGN (UDPHEADER, 8);
#else				// old UDPHEADER 
typedef struct {
	int		magic;
	int		type;
	int		totalsize;  /* total amount of data only (don't count the size of this header) */
	int		pagesize;	/* amount of data in each page INCLUDING the size of the header */
	int		pagenum;
	int		pages;
	} UDPHEADER;
#endif 

/****************************************************/
/* set up the infrastructure for intraprocess communication */
/****************************************************/

typedef struct command {
	int		type;
	int		count;
	int		flag;		/* done, new, old, handshake, whatever, ..... */
	int		arg[5];
	}	COMMAND;
#pragma STRUCT_ALIGN (COMMAND, 8);
		
typedef struct udp_packet {
	UDPHEADER		udphdr;
	char				buf[MAXPACKET];
	} UDPPACKET;
#pragma STRUCT_ALIGN (UDPHEADER, 8);

typedef struct {
	UDPHEADER		udphdr;
	int			cnt;
	} FIFORING;


/****************************************************/
/* completely define radar operation with a single structure */
/****************************************************/

#define	CTRL_CLUTTERFILTER	0x00000001
#define	CTRL_TIMESERIES		0x00000002
#define	CTRL_PHASECORRECT	0x00000004
#define	CTRL_SCAN_TRANSITION	0x00000008
#define	CTRL_TXON		0x00000010
#define	CTRL_SECONDTRIP		0x00000020
#define	CTRL_VELOCITYFOLD	0x00000040
#define	CTRL_CALMODE		0x00000080
#define	CTRL_CALSIMPLE		0x00000100
#define CTRL_OBSERVING          0x00000200

typedef	struct {
	char		type;				// dsp type 0 = AB  1 = P  2 = stokes
    char		start;				// starting at this hit referenced to the begining of the sequence
	char		Apol,Bpol;			// polarizations of the two halves of a pair (numbered H,V,+,-,R,L)
	char		Astep,ABstep;		// step from one A to the next and step from A to B
    } PRIMARYDSP;

#define MAXNUM 1200

//#define PIRAQX_CURRENT_REVISION 1
//struct piraqX_header_rev1
typedef struct infoheader
{	// JVA changes rec'd 4-22-03
	/* /code/oye/solo/translate/piraq.h
         * all elements start on 4-byte boundaries
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
    float4 spare[20];

    /*
    // always append new items so the alignment of legacy variables
    // won't change
    */
} INFOHEADER;
#pragma STRUCT_ALIGN (INFOHEADER, 8);

typedef struct datablock {		/* data that's in the PIRAQ1 FIFO */
	INFOHEADER		info;
	float				data[MAXGATES * 12];
	} DATABLOCK;
#pragma STRUCT_ALIGN (DATABLOCK, 8);

typedef struct packet {
	UDPHEADER	udp;
	COMMAND		cmd;
	DATABLOCK	data;
    } PACKET;
#pragma STRUCT_ALIGN (PACKET, 8);

typedef struct packet_header {			/* this structure must match the non-data portion of the PACKET structure */
	UDPHEADER	udp;
	COMMAND		cmd;
	INFOHEADER	info;
	} PACKETHEADER;
#pragma STRUCT_ALIGN (PACKETHEADER, 8);
	

#define	HEADERSIZE		sizeof(PACKETHEADER)
#define	IQSIZE				(sizeof(float) * 2 * MAXGATES)
#define	ABPSIZE			(sizeof(float) * 12 * MAXGATES)
#define	DATASIZE(a)		(a->data.info.numgates * a->data.info.bytespergate)
#define	RECORDLEN(a)		(sizeof(INFOHEADER) + DATASIZE(a))
#define	TOTALSIZE(a)		(sizeof(COMMAND) + RECORDLEN(a))


void createSineTestWaveform(float freq);	//	create test sine waveform of freq; store to SINstore
void ChannelSelect(int ngates, float * restrict src, float * restrict dst, unsigned int channelMode);
FIFO *fifo_create(char *name, int headersize, int recordsize, int recordnum);
FIFO *fifo_open(char *name);
int fifo_close(FIFO *fifo);
void *fifo_get_last_address(FIFO *fifo);
int fifo_increment_head(FIFO *fifo);
void fifo_notify(FIFO *fifo);
void *fifo_get_read_address(FIFO *fifo, int offset);
int fifo_increment_tail(FIFO *fifo);
int fifo_hit(FIFO *fifo);
int fifo_exhist(char *name);
   
#endif
