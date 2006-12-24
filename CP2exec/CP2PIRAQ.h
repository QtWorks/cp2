#ifndef CP2PIRAQINC_
#define CP2PIRAQINC_
#include "../include/proto.h"
#include "Piraq.h"
#include "piraqComm.h"
#include "CP2Net.h"

class CP2PIRAQ: public PIRAQ {

public:

	CP2PIRAQ( 
		struct sockaddr_in sockAddr,
			int socketFd,
		char* ipDestination,
		int outputPort,
		char* configFname,
		char* dspObjFnamefloat,
		unsigned int packetsPerPciXfer,
		int boardnum
		);

	~CP2PIRAQ();

	int start(__int64 firstPulseNum,
		      __int64 firstBeamNum);
	void stop();
	int poll();
	float prt();
	PINFOHEADER info();

protected:
	int init(char* configFname, char* dspObjFname);

	/// A configurtion which will be created from
	/// a supplied file name
	CONFIG _config;
	/// The FIFO that is used for data transfering from piraq to host
	CircularBuffer*   pFifo;
	/// This will be the first packet in the fifo. It appears that
	/// the piraq may read this structure?
	PPACKET* _pConfigPacket;
	/// The destination data port
	int outport;
	/// The socket for output data
	int outsock;
	/// The last pulse number received. Used to detect
	/// dropped pulses.
	__int64 _lastPulseNumber; 
	/// Cumulative pulse number errors
	int PNerrors;
	/// current azimuth
	float az;
	/// current elevation
	float el; 
	/// current scan
	unsigned int scan;
	/// current volume
	unsigned int volume;
	/// current sequence
	unsigned int seq;
	/// the number of bytes per gate
	int _bytespergate;
	/// the number of hits in each block transfer
	/// from the piraq.
	unsigned int _pulsesPerPciXfer;
	/// Socket specifications for datagrams
	struct sockaddr_in  _sockAddr;
	/// socket file descriptor
	int _socketFd;

	CP2Packet _cp2Packet;

	float _prt;				///< The prt, set by the host. Not sure why we need this here.
	int _prt2;   
	int _timing_mode;
	int _gates;
	int _hits;

	float _xmit_pulsewidth;	///< The transmit pulsewidth, set by the host. Not sure why we need this here.

	unsigned int _totalHits;

	int _boardnum;

	void cp2piraq_fifo_init(CircularBuffer * fifo, int headersize, int recordsize, int numRecords);
	int cp2start(PIRAQ *piraq, PPACKET * pkt);
	int sendData(int size, void* data);

};

#endif
