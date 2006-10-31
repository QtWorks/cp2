#ifndef CP2PIRAQINC_
#define CP2PIRAQINC_

#include "proto.h"

#define PIRAQ3D_SCALE	1.0/pow(2,31)	// normalize 2^31 data to 1.0 full scale using multiplication

class CP2PIRAQ: public PIRAQ {

public:

	CP2PIRAQ( 
		unsigned int Nhits,
		int outputPort,
		char* configFname,
		char* dspObjFnamefloat,
		int bytespergate);

	~CP2PIRAQ();

	int poll(int julian_day);
	int start(__int64 firstPulseNum,
		__int64 firstBeamNum);
	void stop();
	float prt();
	INFOHEADER info();

protected:
	int init(char* configFname, char* dspObjFname);

	/// A configurtion which will be created from
	/// a supplied file name
	CONFIG _config;
    /// The FIFO that is used for data transfering from piraq to host
	FIFO* pFifo; 
	/// This will be the first packet in the fifo. It appears that
	/// the piraq may read this structure?
	PACKET* _pConfigPacket;
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
	int bytespergate;
	/// the number of hits in each block transfer
	/// from the piraq.
	unsigned int Nhits;
};

#endif
