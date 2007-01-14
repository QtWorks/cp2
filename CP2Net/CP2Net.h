#ifndef CP2NETINC_
#define CP2NETINC_

#include <map>
#include <vector>

/// A header for each pulse of data
typedef struct CP2PulseHeader {
    long long pulse_num;	///< Pulse number
    long long beam_num;		///< Beam number
    short antAz;			///< The azimuth
    short antEl;			///< The elevation
	short scanType;			
	short sweepNum;
	short volNum;
	short antSize;
	short antTrans;
    int  channel;			///< 
    int  gates;				///< The number of gates, set by the host.
    int  hits;				///< The number of hits in a beam, set by the host. Used
							///< to calculate beam number from pulse numbers.
	short status;           ///< Status that comes from the piraq for each pulse
	bool horiz;			    ///< set true for horizontal polarization, false for vertical
} CP2PulseHeader;

//Bit mask defines for the status field of CP2PulseHeader
#define PIRAQ_FIFO_EOF 1    ///< If the Piraq reported an EOF error.

/// A header and data are combined to make one beam.
typedef struct CP2Pulse {
	CP2PulseHeader header;///< The beam header.
	float data[2];			///< An array of data values will start here.
} CP2Pulse;

/// Use this class when the full pulse, with
/// a filled data array, needs to be preserved. 
/// Usually a pointer to CP2Pulse just points into
/// a CP2Packet which is going to be deleted, making
/// the CP2Pulse* invalid
class CP2FullPulse {
public:
	CP2FullPulse(CP2Pulse* pPulse);
	~CP2FullPulse();
	float* data();
	CP2PulseHeader* header();
protected:
	CP2Pulse* _cp2Pulse;

};

/// Interface for working with CP2 network data
/// transmission. When constructed, an area for
/// building a network packet is allocated.
/// A network packet will contain 
class CP2Packet{
public:
	CP2Packet();
	virtual ~CP2Packet();
	/// Add a data beam to the packet. This is used for constructing
	/// a packet.
	void addPulse(
		CP2PulseHeader& header,       ///< The beam header information
		int numDataValues,			///< The number of data values.
		float* data					///< The data for the beam
		);
	/// Deconstruct a data packet. This is used when 
	/// extracting beams from a datagram.
	/// @return False if the data structure was self consistent, true if an
	/// error was detected.
	bool setData(
		int size,					///< Size in bytes of the data packet
		void* data					///< The data packet
		);	/// Empty the packet of data.
	void clear();
	/// @return The size (in bytes) of the CP2Packet.
	int packetSize();
	/// @return A pointer to the begining of the CP2Beam array.
	void* packetData();
	/// @return The number of pulses in the packet
	int numPulses();
	/// Fetch a beam
	/// @param i The beam index. It must be less than 
	/// the value returned by numBeams(). 
	/// @return A pointer to beam i. If the index
	/// is illeagal, null is returned.
	CP2Pulse* getPulse(int index);

protected:
	/// The vector that will be expanded with beam data when 
	/// addBeam is called. _paketData is grown as beams are
	/// added to it. It won't be shrunk, so that space allocations
	/// are only done as necessary to met the maximum requested
	/// size.
	std::vector<unsigned char> _packetData;
	/// The amount of data currently in the _packet data
	int _dataSize;
	/// The offset to each pulse in the packet
	std::vector<int> _pulseOffset;
};

#ifndef PULSECOLLATORINC_
#define PULSECOLLATORINC_


#define PulseMap std::map<long long, CP2FullPulse*>

/// will delete pulses when the queue eceeds the queue size limit.
/// matching pulses are poped from the queue and it is up to the
/// caller to delete them.
class CP2PulseCollator {
public:
	CP2PulseCollator(int maxQueueSize);
	virtual ~CP2PulseCollator();
	void addPulse(CP2FullPulse* pPulse, int queueNumber);
	bool gotMatch(CP2FullPulse** pulse0, CP2FullPulse** pulse1);
	int discards();

protected:
	int _maxQueueSize;
	int _discards;

	PulseMap _queue0;
	PulseMap _queue1;


};


#endif

#endif
