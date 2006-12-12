#ifndef CP2NETDATAINC_
#define CP2NETDATAINC_

/// A header for each beam of data
typedef struct CP2NetBeamHeader {
    int  channel;			///< 
    int  gates;				///< The number of gates, set by the host.
    int  hits;				///< The number of hits in a beam, set by the host. Used
							///< to calculate beam number from pulse numbers.
    double az;				///< The azimuth, set by Piraq.
    double el;				///< The elevation, set by Piraq.
    long long pulse_num;	///< Pulse number
    long long beam_num;		///< Beam number
} CP2NetBeamHeader;

/// A header and data are combined to make one beam.
typedef struct CP2NetBeam {
	CP2NetBeamHeader header;///< The beam header.
	int numDataValues;		///< The number of data values (note: not a byte count)
	float* data;			///< An array of data values will start here.
} CP2NetBeam;

/// Multiple CP2NetBeams are combined into one packet.
/// Consecutive beams do not need to have the same
/// data member length.
typdef struct CP2NetPacket {
	int numBeams;					///< The number of following beams in this packet
	CP2NetBeam* cp2NetBeams;		///< a vector of CP2NetBeams
} CP2NetPacket;

/// Interface for working with CP2 network data
/// transmission. When constructed, an area for
/// building a network packet is allocated.
/// A network packet will contain 
class CP2Packet{
public:
	CP2Packet();
	/// Add a data beam to the packet
	void addBeam(
		CP2NetBeamHeader& header,   ///< The beam header information
		int numDataValues,			///< The number of data values.
		float* data					///< The data for the beam
		);
	/// Empty the packet of data.
	void clear();
	/// @return The size (in bytes) of the CP2NetPacket.
	int size();
	/// @return A pointer to the begining of the CP2NetPacket array.
	CP2NetPacket* data();

protected:
	/// The vector that will be expanded with beam data when 
	/// addBeam is called. _paketData is grown as beams are
	/// added to it. It won't be shrunk, so that space allocations
	/// are only done as necessary to met the maximum requested
	/// size.
	std::vector<unsigned char> _packetData;
}
#endif
