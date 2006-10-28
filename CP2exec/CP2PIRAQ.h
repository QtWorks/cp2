#ifndef CP2PIRAQINC_
#define CP2PIRAQINC_

#include "proto.h"

#define PIRAQ3D_SCALE	1.0/pow(2,31)	// normalize 2^31 data to 1.0 full scale using multiplication

class CP2PIRAQ: public PIRAQ {

public:

	CP2PIRAQ( 
		CONFIG* pConfig,
		unsigned int Nhits,
		int outputPort,
		char* configFname,
		char* dspObjFnamefloat,
		int bytespergate,
		char* cmdFifoName);

	~CP2PIRAQ();

	int poll(int julian_day);
	int start();
	float prt();
	INFOHEADER info();

protected:
	int init();

		CONFIG* pConfig;
		   FIFO* pFifo; 
		   FIFO* pCmd;
		   UDPHEADER* udp; 
		   PACKET* pPkt;
		   int cmd_notifysock; 
		   int beamnum;
		   int pulsenum;
		   int outport;
		   int outsock;
		   int cycle_fifo_hits; 
		   int fifo_hits;
		   __int64 lastpulsenumber; 
		   int PNerrors;
		   float az;
		   float el; 
		   unsigned int scan; 
		   unsigned int volume; 
		   unsigned int seq;
		   int bytespergate;
		   unsigned int Nhits;
		   char* cmdFifoName;
		   char* configFname;
		   char* dspObjFname;
		   float _prt;
		INFOHEADER _info;

};

#endif
