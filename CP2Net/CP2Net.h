#ifndef CP2NETINC_
#define CP2NETINC_

#include <map>
#include <vector>

/// A header for each pulse of data
typedef struct CP2PulseHeader {
    long long pulse_num;	///< Pulse number
    long long beam_num;		///< Beam number
    double az;			///< The azimuth
    double el;			///< The elevation
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

/// A header and data are combined to make one pulse.
typedef struct CP2Pulse {
	CP2PulseHeader header;///< The pulse header.
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

enum PRODUCT_TYPES {
	PROD_S_DBMHC,	///< S-band dBm horizontal co-planar
	PROD_S_DBMVC,	///< S-band dBm vertical co-planar
	PROD_S_DBZHC,	///< S-band dBz horizontal co-planar
	PROD_S_DBZVC,	///< S-band dBz vertical co-planar
	PROD_S_SNR,		///< S-band SNR
	PROD_S_VEL,		///< S-band velocity
	PROD_S_WIDTH,	///< S-band spectral width
	PROD_S_RHOHV,	///< S-band rhohv
	PROD_S_PHIDP,	///< S-band phidp
	PROD_S_ZDR,		///< S-band zdr
	PROD_X_DBMHC,	///< X-band dBm horizontal co-planar
	PROD_X_DBMVX,	///< X-band dBm vertical cross-planar
	PROD_X_DBZHC,	///< X-band dBz horizontal co-planar
	PROD_X_SNR,		///< X-band SNR
	PROD_X_VEL,		///< X-band velocity
	PROD_X_WIDTH,	///< X-band spectral width
	PROD_X_LDR		///< X-band LDR
};

/// A header for each beam product
typedef struct CP2ProductHeader {
	PRODUCT_TYPES prodType;	///< The product identifier
    int  gates;				///< The number of gates, set by the host.
	long long beamNum;		///< The beam number.
    double az;			///< The azimuth
    double el;			///< The elevation
} CP2ProductHeader;

/// A header and data are combined to make one product.
typedef struct CP2Product {
	CP2ProductHeader header;///< The pulse header.
	double data[1];			///< An array of data values will start here.
} CP2Product;

/// Interface for working with CP2 network data
/// transmission. When constructed, an area for
/// building a network packet is allocated.
/// A network packet will contain either CP2Pulse
/// or CP2Product structures.
class CP2Packet{
public:
	CP2Packet();
	virtual ~CP2Packet();
	/// Add a pulse to the packet. This is used for constructing
	/// a packet of pulses
	void addPulse(
		CP2PulseHeader& header,       ///< The pulse header information
		int numDataValues,			///< The number of data values.
		float* data					///< The data for the beam
		);
	/// Add a product to the packet. This is used for constructing
	/// a packet of products
	void addProduct(
		CP2ProductHeader& header,       ///< The pulse header information
		int numDataValues,			///< The number of data values.
		double* data					///< The data for the beam
		);
	/// Construct a pulse packet. Used when building a packet from a datagram.
	/// @return False if the data structure was self consistent, true if an
	/// error was detected.
	bool setPulseData(
		int size,					///< Size in bytes of the data
		void* data					///< The data packet
		);	
	/// Construct a product packet. Used when building a packet from a datagram.
	/// @return False if the data structure was self consistent, true if an
	/// error was detected.
	bool setProductData(
		int size,					///< Size in bytes of the data
		void* data					///< The data packet
		);	
	/// Empty the packet of data.
	void clear();
	/// @return The size (in bytes) of the CP2Packet.
	int packetSize();
	/// @return A pointer to the begining of the CP2Beam array.
	void* packetData();
	/// @return The number of pulses in the packet
	int numPulses();
	/// @return The number of produ in the packet
	int numProducts();
	/// Fetch a pulse
	/// @param i The pulse index. It must be less than 
	/// the value returned by numPulses(). 
	/// @return A pointer to beam i. If the index
	/// is illegal, null is returned.
	CP2Pulse* getPulse(int index);
	/// Fetch a product
	/// @param i The beam index. It must be less than 
	/// the value returned by numBeams(). 
	/// @return A pointer to beam i. If the index
	/// is illegal, null is returned.
	CP2Product* getProduct(int index);

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
