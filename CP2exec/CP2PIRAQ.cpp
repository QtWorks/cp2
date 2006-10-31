#include <stdafx.h>
#include <winsock2.h>
#include "CP2PIRAQ.h"
#include "get_julian_day.h"
#include "proto.h"

#define CYCLE_HITS 20

///////////////////////////////////////////////////////////////////////////
CP2PIRAQ::CP2PIRAQ(
				   unsigned int Nhits_, 
				   int outputPort_, 
				   char* configFname, 
				   char* dspObjFname,
				   int bytespergate_):
PIRAQ(),
Nhits(Nhits_), 
outport(outputPort_), 
bytespergate(bytespergate_),
_lastPulseNumber(0)
{
	if((outsock = open_udp_out("192.168.3.255")) ==  ERROR)			/* open one socket */
	{
		printf("Could not open output socket\n"); 
		exit(0);
	}
	printf("udp socket opens; outsock1 = %d\n", outsock); 

	init(configFname, dspObjFname);
}

///////////////////////////////////////////////////////////////////////////
CP2PIRAQ::~CP2PIRAQ()
{
	stop();
}

/////////////////////////////////////////////////////////////////////////////
int
CP2PIRAQ::init(char* configFname, char* dspObjFname)	 
{
	
	readconfig(configFname, &_config);    

	int r_c;   // generic return code
	int y;

	r_c = this->Init(PIRAQ_VENDOR_ID,PIRAQ_DEVICE_ID); 
	printf("this->Init() r_c = %d\n", r_c); 

	if (r_c == -1) { // how to use GetErrorString() 
		char errmsg[256]; 
		this->GetErrorString(errmsg); printf("error: %s\n", errmsg); 
		return -1; 
	}

	/* put the DSP into a known state where HPI reads/writes will work */
	this->ResetPiraq(); // !!!redundant? 
	this->GetControl()->UnSetBit_StatusRegister0(STAT0_SW_RESET);
	Sleep(1);
	this->GetControl()->SetBit_StatusRegister0(STAT0_SW_RESET);
	Sleep(1);
	printf("this reset\n"); 
	unsigned int EPROM1[128]; 
	this->ReadEPROM(EPROM1);
	for(y = 0; y < 16; y++) {
		printf("%08lX %08lX %08lX %08lX\n",EPROM1[y*4],EPROM1[y*4+1],EPROM1[y*4+2],EPROM1[y*4+3]); 
	}

	this->SetCP2PIRAQTestAction(SEND_CHA);	//	send CHA by default; SEND_COMBINED after dynamic-range extension implemented 
	stop_piraq(&_config, this);

	// create the data fifo.
	pFifo = (FIFO *)this->GetBuffer(); 

	// CP2: data packets sized at runtime.  + BUFFER_EPSILON
	piraq_fifo_init(
		pFifo,"/PRQDATA", 
		HEADERSIZE, 
		Nhits * (HEADERSIZE + (_config.gatesa * bytespergate) + BUFFER_EPSILON), 
		PIRAQ_FIFO_NUM); 
	printf("hit size = %d computed Nhits = %d\n", 
		(HEADERSIZE + (_config.gatesa * bytespergate) + BUFFER_EPSILON), 
		Nhits); 

	if (!pFifo) { 
		printf("this fifo_create failed\n"); exit(0);
	}
	printf("pFifo = %p, recordsize = %d\n", pFifo, pFifo->record_size); 

	//////////////////////////////////////////////////////////////////
	//
	// Not sure what the following is about; maybe the 
	// piraq will read this information?
	_pConfigPacket = (PACKET *)fifo_get_header_address(pFifo); 
	_pConfigPacket->cmd.flag = 0;                    // Preset the flags just in case
	_pConfigPacket->data.info.channel = 0;			// set BOARD number

	struct_init(&_pConfigPacket->data.info, configFname);   /* initialize the info structure */

	r_c = this->LoadDspCode(dspObjFname); // load entered DSP executable filename
	printf("loading %s: this->LoadDspCode returns %d\n", dspObjFname, r_c);  
	timerset(&_config, this); // !note: also programs pll and FIR filter. 

	this->SetCP2PIRAQTestAction(SEND_CHA);	//	send CHA by default; SEND_COMBINED after dynamic-range extension implemented 
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
int 
CP2PIRAQ::start(__int64 firstPulseNum,
				__int64 firstBeamNum) 
{
	_pConfigPacket->data.info.pulse_num = firstPulseNum;	// set UNIX epoch pulsenum just before starting
	_pConfigPacket->data.info.beam_num = firstBeamNum; 
	_pConfigPacket->data.info.packetflag = 1;			// set to piraq: get header! 
	// start the PIRAQ: also points the piraq to the fifo structure 
	if (!::start(&_config, this, _pConfigPacket))
	{
		printf("Piraq DSP program not ready: pkt->cmd.flag != TRUE (1)\n");
		return -1;
	}
	return 0;
}
/////////////////////////////////////////////////////////////////////////////
int
CP2PIRAQ::poll(int julian_day) 
{
	// sleep for a ms
	Sleep(1);
	int cycle_fifo_hits = 0;
	// take CYCLE_HITS beams from piraq:
	while((
		(fifo_hit(pFifo)) > 0) && 
		(cycle_fifo_hits < CYCLE_HITS)) 
	{ 
		// fifo hits ready: save #hits pending 
		cycle_fifo_hits++; 

		// get the next packet in the circular buffer
		PACKET* pFifoPiraq = (PACKET *)fifo_get_read_address(pFifo, 0); 

		// and set the magic number and datatype in it.
		// What is the purpose of MAGIC?
		UDPHEADER* udp = &pFifoPiraq->udp;
		udp->magic = MAGIC;
		udp->type = UDPTYPE_PIRAQ_CP2_TIMESERIES; 

		// set the data size
		pFifoPiraq->data.info.bytespergate = 2 * (sizeof(float)); // Staggered PRT ABPDATA

		// set a time stamp. Why is this necessary?
		unsigned __int64 temp = 
			pFifoPiraq->data.info.pulse_num * 
			(unsigned __int64)(_pConfigPacket->data.info.prt[0] * 
			(float)COUNTFREQ + 0.5);
		pFifoPiraq->data.info.secs = 
			temp / COUNTFREQ;
		pFifoPiraq->data.info.nanosecs = 
			((unsigned __int64)10000 * (temp % ((unsigned __int64)COUNTFREQ))) 
			/ (unsigned __int64)COUNTFREQ;
		pFifoPiraq->data.info.nanosecs *= 
			(unsigned __int64)100000; // multiply by 10**5 to get nanoseconds

		// bogus up pointing information for right now.
		az += 0.5;  
		if(az > 360.0) { // full scan 
			az -= 360.0; // restart azimuth angle 
			el += 7.0;		// step antenna up 
			scan++; // increment scan count
			if (el >= 21.0) {	// beyond allowed step
				el = 0.0; // start over at horizon
				volume++; // finish volume
			}
		} 
		pFifoPiraq->data.info.az = az;  
		pFifoPiraq->data.info.el = el; // set in packet 
		pFifoPiraq->data.info.scan_num = scan;
		pFifoPiraq->data.info.vol_num = volume;  
		pFifoPiraq->data.info.julian_day = julian_day; 

#ifdef	DRX_PACKET_TESTING	// define to activate data packet resizing for throughput testing. 
		//					fifopiraq1->data.info.recordlen = (fifopiraq1->data.info.clutter_end[0])*fifopiraq1->data.info.gates*24 + (RECORDLEN(fifopiraq1)); // vary multiplier; note CPU usage in Task Manager. 24 = 2*bytespergate. 
		pFifoPiraq->data.info.recordlen = Nhits*(RECORDLEN(pFifoPiraq)+BUFFER_EPSILON); /* this after numgates corrected */
#else
		pFifoPiraq->data.info.recordlen = RECORDLEN(pFifoPiraq); /* this after numgates corrected */
#endif

		//////////////////////////////////////////////////////////////////////////
		//
		// check for pulse numbers out of sequence.
		for (int i = 0; i < Nhits; i++) { // all hits in the packet 
			// compute pointer to datum in an individual hit, dereference and print. 
			// CP2 PCI Bus transfer size: Nhits * (HEADERSIZE + (config1->gatesa * bytespergate) + BUFFER_EPSILON)
			__int64 thisPulseNumber = *(__int64 *)((char *)&pFifoPiraq->data.info.pulse_num + 
				i*((HEADERSIZE + 
				(_config.gatesa * bytespergate) + 
				BUFFER_EPSILON))); 

			if (_lastPulseNumber != thisPulseNumber - 1) { // PNs not sequential
				printf("pulse number out of sequence, last PN %I64d, this PN %I64d\n", 
					_lastPulseNumber, thisPulseNumber);  
				PNerrors++; 
			}
			_lastPulseNumber = thisPulseNumber; // previous hit PN
		}

		//////////////////////////////////////////////////////////////////////////
		//
		// send data out on the socket
		
		pFifoPiraq->udp.totalsize = Nhits*(TOTALSIZE(pFifoPiraq) + BUFFER_EPSILON);
		seq = send_udp_packet(outsock, outport, seq, udp); 

		//////////////////////////////////////////////////////////////////////////
		//
		// return packet to the fifo
		//
		fifo_increment_tail(pFifo);

	} // end	while(fifo_hit()

return 0;
}
///////////////////////////////////////////////////////////////////////////
float
CP2PIRAQ::prt()
{
	return _pConfigPacket->data.info.prt[0];
}

INFOHEADER
CP2PIRAQ::info()
{
	return _pConfigPacket->data.info;
}

///////////////////////////////////////////////////////////////////////////
void
CP2PIRAQ::stop() 
{
	stop_piraq(&_config, this);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
