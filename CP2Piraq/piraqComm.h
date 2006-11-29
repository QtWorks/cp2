#ifndef _PROTO_H_
#define _PROTO_H_

#include "dd_types.h"

#define PCIBASE		0x3000000

#define STRUCTURE_ALIGNMENT 8 // must be a power of 2

#define	PIRAQ_DEVICE_ID 	0x9054
#define	PIRAQ_VENDOR_ID		0x10B5   

// 3 modes of data transfer from PIRAQ to host: first 2 are diagnostic
#define	SEND_CHA		0		// send CHA
#define	SEND_CHB		1		// send CHB
#define	SEND_COMBINED	2		// execute dynamic-range extension algorithm; send resulting combined data
#define	CHMODE_MASK		0x03	// extract channel mode bits

#pragma pack(4)
/// A circular buffer that is filled on the Piraq side and
/// emptied on the host side. The Piraq will increment head whenever
/// records are added, and the host will increment tail whenever 
/// they are removed. Management of this structure is performed
/// by the pfifo_ routines.
typedef	struct 
	{
	int	header_off;		///< offset to the user header (can't use absolute address here) 
	int	fifobuf_off;	///< offset to fifo base address 
	int	record_size;	///< size in bytes of each FIFO record 
	int	record_num;		///< number of records in FIFO buffer 
	int	head,tail;		///< indices to the head and tail records 
	} CircularBuffer;

///
/// Pulsenumber (pulse_num) is the number of transmitted pulses
/// since Jan 1970. It is a 64 bit number. It is assumed
/// that the first pulse (pulsenumber = 0) falls exactly
/// at the midnight Jan 1,1970 epoch. To get unix time,
/// multiply by the PRT. The PRT is a rational number a/b.
/// More specifically N/Fc where Fc is the counter clock (PIRAQ_CLOCK_FREQ),
/// and N is the divider number. So you can get unix time
/// without roundoff error by:
///    secs = pulsenumber * N / Fc. 
/// The computations is done with 64 bit arithmatic. No
/// rollover will occur.
///
/// The 'nanosecs' field is derived without roundoff
/// error by: 100 * (pulsenumber * N % Fc).
///
/// Beamnumber is the number of beams since Jan 1,1970.
/// The first beam (beamnumber = 0) was completed exactly
/// at the epoch. beamnumber = pulsenumber / hits. 
    
/// PINFOHEADER is a header structure that contains 
/// metadata for each individual beam. It also contains
/// a field (flag) that is used to transmit 
/// commands to the host from the Piraq, and status
/// from the Piraq to the host.
///
/// A complete data packet is defined by PPACKET, and  contains
/// a PINFOHEADER followed by an array of floats, with an 
/// I&Q value for each gate.
///
/// The PINFOHEADER is also used to transfer operating
/// parameters from the host to the Piraq. Thus at startup,
/// the first packet in the circular buffer is filled with
/// a completed PINFOHEADER by the host. The Piraq will
/// extract useful fields (gates, etc.) from this, to initialize 
/// itself.
///
typedef struct pinfoheader
{   
	int	   flag;			///< command indicator: done, new, old, handshake, whatever, ..... 
    uint4  packetflag;		///< Flag for communicating status back to host.
	char   desc[4];			///< not sure what this is for; may be able to discard.
    uint4  channel;         ///< board number, set by the host
	uint4  packetsPerBlock;	///< The number of PPACKETS per PCI transfer, set by the host.
    uint4  gates;			///< The number of gates, set by the host.
    uint4  hits;			///< The number of hits in a beam, set by the host. Used
							///< to calculate beam number from pulse numbers.
    uint4  bytespergate;	///< The number of bytes per gate (2*sizeof(float)),  set by the host.
    float4 az;				///< The azimuth, set by Piraq.
    float4 el;				///< The elevation, set by Piraq.
#ifdef _TMS320C6X			///        TI doesn't support long long 
    uint4 pulse_num_low;	///< Pulse number least significant word, on Piraq
    uint4 pulse_num_high;	///< Pulse number most significant word, on Piraq
    uint4 beam_num_low;		///< Beam number least significant word, on Piraq
    uint4 beam_num_high;	///< Beam number most significant word, on Piraq
#else
    uint8 pulse_num;		///< Pulse number on host, keep this field on an 8 byte boundary
    uint8 beam_num;			///< Beam number on host,  keep this field on an 8 byte boundary */
#endif
} PINFOHEADER;
#pragma STRUCT_ALIGN (PINFOHEADER, 8);

/// PPACKET combines the header info from PINFOHEADER with the data array.
typedef struct ppacket {
	PINFOHEADER		info;		///< The header information in the packet.
	float			data[2];	///< place holder for the data array.
} PPACKET;
#pragma STRUCT_ALIGN (PPACKET, 8);

//-------------------------------------------------------------------
//
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

//-------------------------------------------------------------------
//
// The interfce to the circular buffer management routines
//
//
#ifdef __cplusplus
extern "C" {  // only need to export C interface if
              // used by C++ source code
#endif
CircularBuffer* pfifo_create(char *name, int headersize, int recordsize, int recordnum);
CircularBuffer* pfifo_open(char *name);
int             pfifo_close(CircularBuffer *fifo);
void*           pfifo_get_last_address(CircularBuffer *fifo);
int             pfifo_increment_head(CircularBuffer *fifo);
void            pfifo_notify(CircularBuffer *fifo);
void*           pfifo_get_read_address(CircularBuffer *fifo, int offset);
void*           pfifo_get_write_address(CircularBuffer *fifo);
int             pfifo_increment_tail(CircularBuffer *fifo);
int             pfifo_hit(CircularBuffer *fifo);
int             pfifo_exhist(char *name);
void*           pfifo_get_header_address(CircularBuffer *fifo);
#ifdef __cplusplus
}
#endif

#endif
