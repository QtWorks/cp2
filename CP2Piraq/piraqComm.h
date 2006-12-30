#ifndef _PROTO_H_
#define _PROTO_H_

#define	PIRAQ_FIFO_NUM		120
//
// ALL OF THE FOLLOWING SHOULD MAKE SENSE, BUT IT DOESN'T
// WORK. IF THE CIRCULAR BUFFER IS LARGER THAN 8MB,
// WINDOWS CRASHES. SERIOUSLY. THE GOOD OLD FASHIONED
// BLUE SCREEN OF DEATH. SO FOR NOW, PIRAQ_FIFO_NUM = 120.
// IT HAS SOMETHING TO DO WITH THE 8MB LIMIT THAT IS 
// CONFUSUINGLY DOCUMENTED IN THE PIRAQ  DRIVER CODE.
//
// PIRAQ_FIFO_NUM is number of entries in the 
// circular buffer, in host ram. Recall 
// that the circular buffer is accesed by both 
// the host and the Piraq (via PCI).
// 
// The total size of the circular buffer 
// will be PIRAQ_FIFO_NUM times the 
// size of each PPACKET, plus the
// (small) amount of book keeping (head and
// tail pointer, etc.) kept at the head of the
// circular buffer. This total must not exceed
// 16MB (0x1000000), which is the size of the PCI 
// address space that the Piraq can access. It also
// can not exceed the size of the reserved ram that
// the PLX code has reserved from windows. 
// The amount that the PLX code will allocate
// is specified in a registry entry, and the
// registry is configured currently to ask for 
// 48MB (0x3000000), but perhaps it should
// should only ask for 16MB, since that is all that
// the Piraq can use anyway. 
//
// The Piraq driver should verify that at least
// 16MB was allocated; it doesn't currently do 
// this, and thus unexpected system lockups 
// can be expected if the memory cannot be allocated.
//
// Roughly estimating:
//    250 * 1000 gates 
///       * 8 pulses per packet 
//        * 8 bytes per gate 
//        = 16,000,000.
// This does not account for the size of PINFOHEADER, nor
// the circular buffer book keeping. It does seem 
// like a reasonable margin, since
// 16MB = 16,777,216.

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

//--------------------------------------------------------------------

#pragma pack(4)
/// A circular buffer that is filled on the Piraq side and
/// emptied on the host side. The Piraq will increment head whenever
/// records are added, and the host will increment tail whenever 
/// they are removed. Management of this structure is performed
/// by the cb_ routines.
typedef	struct CircularBuffer
	{
	int	header_off;		///< offset to the user header (can't use absolute address here) 
	int	cbbuf_off;		///< offset to cb base address 
	int	record_size;	///< size in bytes of each cb record 
	int	record_num;		///< number of records in cb buffer 
	int	head,tail;		///< indices to the head and tail records 
	} CircularBuffer;

//--------------------------------------------------------------------

/// The three types of CP2 receivers
typedef enum RCVRTYPE {
	SHV,	///< Sband alternating H and V
	XH,		///< X band coplanar horizontal
	XV		///< X band cross polar vertical
} RCVRTYPE;

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
/// Pulsenumber (pulse_num) is the number of transmitted pulses
/// since Jan 1970. It is a 64 bit number. It is assumed
/// that the first pulse (pulsenumber = 0) falls exactly
/// at the midnight Jan 1,1970 epoch. To get unix time,
/// multiply by the PRT. The PRT is a rational number a/b.
/// More specifically N/Fc where Fc is the counter clock (PIRAQ_CLOCK_FREQ),
/// and N is the divider number. So you can get unix time
/// without roundoff error by:
///    secs = pulsenumber * N / Fc. 
/// The computation is done with 64 bit arithmetic. No
/// rollover will occur.
///
/// The 'nanosecs' field is derived without roundoff
/// error by: 100 * (pulsenumber * N % Fc).
///
/// Beamnumber is the number of beams since Jan 1,1970.
/// The first beam (beamnumber = 0) was completed exactly
/// at the epoch. beamnumber = pulsenumber / hits. 
///
/// An EOF was detected on one of the Piraq A/D fifos.
#define FIFO_EOF 1
typedef struct PINFOHEADER
{   
	int	   flag;			///< command indicator: done, new, old, handshake, whatever, ..... 
    short  status;			///< Flag for communicating status back to host.
	short  rcvrType;		///< The type of this receiver (RCVRTYPE)
	short  horiz;			///< Set true if horizontal, false if vertical
    uint4  channel;         ///< board number, set by the host
	uint4  packetsPerBlock;	///< The number of PPACKETS per PCI transfer, set by the host.
    uint4  gates;			///< The number of gates, set by the host.
    uint4  hits;			///< The number of hits in a beam, set by the host. Used
							///< to calculate beam number from pulse numbers.
    uint4  bytespergate;	///< The number of bytes per gate (2*sizeof(float)),  set by the host.
	uint4  PMACdpramAddr;   ///< The PCI bus address of the PMAC dpram
    short  antAz;			///< The azimuth read by the Piraq from the PMAC
    short  antEl;			///< The elevation read by the Piraq from the PMAC
	short  scanType;		///< The scan type read by the Piraq from the PMAC
	short  sweepNum;		///< The sweep number read by the Piraq from the PMAC
	short  volNum;			///< The volume number read by the Piraq from the PMAC
	short  antSize;			///< The size (whatever this means) read by the Piraq from the PMAC
	short  antTrans;		///< The antenna transition flag read by the Piraq from the PMAC
#ifdef _TMS320C6X			///        TI doesn't support long long 
    uint4 pulse_num_low;	///< Pulse number least significant word, on Piraq
    uint4 pulse_num_high;	///< Pulse number most significant word, on Piraq
#else
    uint8 pulse_num;		///< Pulse number on host, keep this field on an 8 byte boundary
#endif
} PINFOHEADER;
#pragma STRUCT_ALIGN (PINFOHEADER, 8);

/// PPACKET combines the header info from PINFOHEADER with the data array.
typedef struct PPACKET {
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
CircularBuffer* cb_create(int headersize, int recordsize, int recordnum);
CircularBuffer* cb_open();
int             cb_close(CircularBuffer *cb);
void*           cb_get_last_address(CircularBuffer *cb);
int             cb_increment_head(CircularBuffer *cb);
void            cb_notify(CircularBuffer *cb);
void*           cb_get_read_address(CircularBuffer *cb, int offset);
void*           cb_get_write_address(CircularBuffer *cb);
int             cb_increment_tail(CircularBuffer *cb);
int             cb_hit(CircularBuffer *cb);
int             cb_exhist(char *name);
void*           cb_get_header_address(CircularBuffer *cb);
#ifdef __cplusplus
}
#endif

#endif
