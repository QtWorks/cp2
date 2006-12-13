#ifndef CP2NETDATAINC_
#define CP2NETDATAINC_

#include <vector>

/// A header for each beam of data
typedef struct CP2BeamHeader {
    long long pulse_num;	///< Pulse number
    long long beam_num;		///< Beam number
    double az;				///< The azimuth, set by Piraq.
    double el;				///< The elevation, set by Piraq.
    int  channel;			///< 
    int  gates;				///< The number of gates, set by the host.
    int  hits;				///< The number of hits in a beam, set by the host. Used
							///< to calculate beam number from pulse numbers.
} CP2BeamHeader;

/// A header and data are combined to make one beam.
typedef struct CP2Beam {
	CP2BeamHeader header;///< The beam header.
	int numDataValues;		///< The number of data values (note: not a byte count)
	float* data;			///< An array of data values will start here.
} CP2Beam;

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
	void addBeam(
		CP2BeamHeader& header,       ///< The beam header information
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
		);
	/// Empty the packet of data.
	void clear();
	/// @return The size (in bytes) of the CP2Packet.
	int packetSize();
	/// @return A pointer to the begining of the CP2Beam array.
	void* packetData();
	/// @return The number of beams in the packet
	int numBeams();
	/// Fetch a beam
	/// @param i The beam index. It must be less than 
	/// the value returned by numBeams(). 
	/// @return A pointer to beam i. If the index
	/// is illeagal, null is returned.
	CP2Beam* getBeam(int index);

protected:
	/// The vector that will be expanded with beam data when 
	/// addBeam is called. _paketData is grown as beams are
	/// added to it. It won't be shrunk, so that space allocations
	/// are only done as necessary to met the maximum requested
	/// size.
	std::vector<unsigned char> _packetData;
	/// The amount of data currently in the _packet data
	int _dataSize;
	/// The offset to each beam in the packet
	std::vector<int> _beamOffset;
};
#endif
