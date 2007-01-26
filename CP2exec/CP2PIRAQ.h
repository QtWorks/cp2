#ifndef CP2PIRAQINC_
#define CP2PIRAQINC_
#include "Piraq.h"
#include "piraqComm.h"
#include "CP2Net.h"
#include "config.h"
#include "subs.h"

#include <qsocketdevice.h>
#include <qhostaddress.h>

class CP2PIRAQ: public PIRAQ {

public:

	CP2PIRAQ( 
		QHostAddress* pHostAddr,
		int portNumber,
		QSocketDevice* pSocketDevice,
		char* configFname,
		char* dspObjFnamefloat,
		unsigned int pulsesPerPciXfer,
		unsigned int pmacDpramBusAddr,
		int boardnum,
		RCVRTYPE rcvrType
		);

	~CP2PIRAQ();

	int start(long long firstPulseNum);
	void stop();
	int poll();
	float prt();
	PINFOHEADER info();
	int pnErrors();
	double sampleRate();
	/// @return the current eof indicator flag. 
	/// The flag is cleared when this function is called.
	bool eof();

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
	/// Host address for datagram writes
	QHostAddress* _pHostAddr;
	/// Destination port.
	int _portNumber;
	/// outgoing socket
	QSocketDevice* _pSocketDevice;
	/// The pci bus address for the PMAC dpram; this
	/// is passed to the piraq so that it can read
	/// az and el angles directly from the PMAC.
	unsigned int _pmacDpramAddr;
	/// A packet that can be succesive filled with the
	/// blocked pulse data as it is read out of the PCI
	/// circular buffer.
	CP2Packet _cp2Packet;
	/// The last saved system tick count (milliseconds) 
	int _lastTickCount;
	/// The _eof flag is set when an EOF is discovered in
	/// a pulse. It is cleared when the eof() function is called.
	bool _eof;
	/// count pulses, for diagnostic purposes. this will
	/// be removed in production software
	long long _nPulses;

	float _prt;				
	int _prt2;   
	int _timing_mode;
	int _gates;
	int _hits;
	float _xmit_pulsewidth;	
	unsigned int _totalHits;
	int _boardnum;
	RCVRTYPE _rcvrType;
	double _sampleRate;

	int sendData(int size, void* data);

};

#endif
