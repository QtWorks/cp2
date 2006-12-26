#ifndef CP2PIRAQINC_
#define CP2PIRAQINC_
#include "Piraq.h"
#include "piraqComm.h"
#include "CP2Net.h"
#include "config.h"
#include "subs.h"

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

	int start(long long firstPulseNum,
		      long long firstBeamNum);
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
	CircularBuffer*   _pFifo;
	/// This will be the first packet in the fifo. It appears that
	/// the piraq may read this structure?
	PPACKET* _pConfigPacket;
	/// The destination data port
	int _outport;
	/// The socket for output data
	int _outsock;
	/// The last pulse number received. Used to detect
	/// dropped pulses.
	long long _lastPulseNumber; 
	/// Cumulative pulse number errors
	int _PNerrors;
	/// current azimuth
	float az;
	/// current elevation
	float el; 
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
	float _prt;				
	int _prt2;   
	int _timing_mode;
	int _gates;
	int _hits;
	float _xmit_pulsewidth;	
	unsigned int _totalHits;
	int _boardnum;

	int sendData(int size, void* data);

};

#endif
