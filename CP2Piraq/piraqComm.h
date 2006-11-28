#ifndef _PROTO_H_
#define _PROTO_H_

#include "dd_types.h"

#define PCIBASE		0x3000000

#define	MAGIC		0x12345678

#define STRUCTURE_ALIGNMENT 8 // must be a power of 2

#define	PIRAQ_DEVICE_ID 	0x9054
#define	PIRAQ_VENDOR_ID		0x10B5   

#define	true	1
#define	false	0

void
dmaTransfer(int channel, 
			int *src, 
			int *dst, 
			int transferCount,
			int globalReload); 

#define	PUDPSENDSIZE	(unsigned int)65536	//	size of N-hit packets in system: PIRAQ through Qt applications

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

#define		PI				3.1415926

#define	PMAXPACKET	1200
#define	PMAXGATES	400		// Added to test speed; most you can do w/clutterfilter

#pragma pack(4)
typedef	struct 
	{
	int	size;		/* total size of allocated memory for this FIFO */
	int	header_off;	/* offset to the user header (can't use absolute address here) */
	int	fifobuf_off;	/* offset to fifo base address */
	int	record_size;	/* size in bytes of each FIFO record */
	int	record_num;	/* number of records in FIFO buffer */
	int	head,tail;	/* indexes to the head and tail records */
	} CircularBuffer;

typedef struct pudp_header{
    int totalsize;      /* total amount of data only (don't count the size of this header) */
} PUDPHEADER;
#pragma STRUCT_ALIGN (PUDPHEADER, 8);

typedef struct pcommand {
	int		type;
	int		flag;		/* done, new, old, handshake, whatever, ..... */
	}	PCOMMAND;
#pragma STRUCT_ALIGN (PCOMMAND, 8);
		
typedef struct pinfoheader
{   
	char desc[4];			/* "DWLX" */
    uint4 channel;          ///< board number
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
    uint4 beam_num_low;
    uint4 beam_num_high;
#else
    uint8 pulse_num;   /*  keep this field on an 8 byte boundary */
    uint8 beam_num;	   /*  keep this field on an 8 byte boundary */
#endif
	uint4 packetsPerBlock;
    uint4 gates;
    uint4 hits;
    uint4 bytespergate; 
    float4 rcvr_pulsewidth;
#define PX_NUM_PRT 4
    float4 prt[PX_NUM_PRT];
    float4 az;   /* azimuth: referenced to 9550 MHz. possibily modified to be relative to true North. */
    float4 el;		/* elevation: referenced to 9550 MHz.  */ 
    uint4  packetflag; 
#define PPX_MAX_RADAR_NAME 16
    char radar_name[PPX_MAX_RADAR_NAME];
#define PPX_MAX_CHANNEL_NAME 16
    char channel_name[PPX_MAX_CHANNEL_NAME];
#define PPX_MAX_SITE_NAME 12
    char site_name[PPX_MAX_SITE_NAME];
    float4 frequency;
    float4 xmit_pulsewidth;
} PINFOHEADER;
#pragma STRUCT_ALIGN (PINFOHEADER, 8);

typedef struct pdatablock {		/* data that's in the PIRAQ1 FIFO */
	PINFOHEADER		info;
	float				data[PMAXGATES * 12];
	} PDATABLOCK;
#pragma STRUCT_ALIGN (PDATABLOCK, 8);

typedef struct ppacket {
	PUDPHEADER	udp;
	PCOMMAND		cmd;
	PDATABLOCK	data;
    } PPACKET;
#pragma STRUCT_ALIGN (PPACKET, 8);

typedef struct ppacket_header {			/* this structure must match the non-data portion of the PACKET structure */
	PUDPHEADER	udp;
	PCOMMAND		cmd;
	PINFOHEADER	info;
	} PPACKETHEADER;
#pragma STRUCT_ALIGN (PPACKETHEADER, 8);
	
#define	PHEADERSIZE		sizeof(PPACKETHEADER)
#define	PRECORDLEN(a)  (sizeof(PINFOHEADER) + (PDATASIZE(a)))
#define	PDATASIZE(a)   (a->data.info.gates * a->data.info.bytespergate)
//void createSineTestWaveform(float freq);	//	create test sine waveform of freq; store to SINstore
//void ChannelSelect(int ngates, float * restrict src, float * restrict dst, unsigned int channelMode);


#ifdef __cplusplus
extern "C" {  // only need to export C interface if
              // used by C++ source code
#endif
CircularBuffer *pfifo_create(char *name, int headersize, int recordsize, int recordnum);
CircularBuffer *pfifo_open(char *name);
int    pfifo_close(CircularBuffer *fifo);
void  *pfifo_get_last_address(CircularBuffer *fifo);
int    pfifo_increment_head(CircularBuffer *fifo);
void   pfifo_notify(CircularBuffer *fifo);
void  *pfifo_get_read_address(CircularBuffer *fifo, int offset);
void  *pfifo_get_write_address(CircularBuffer *fifo);
int    pfifo_increment_tail(CircularBuffer *fifo);
int    pfifo_hit(CircularBuffer *fifo);
int    pfifo_exhist(char *name);
void  *pfifo_get_header_address(CircularBuffer *fifo);
#ifdef __cplusplus
}
#endif

#endif
