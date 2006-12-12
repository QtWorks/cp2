#include <winsock2.h>
#include "CP2PIRAQ.h"
#include "../include/proto.h"
#include "piraqComm.h"
#include "../PiraqIII_RevD_Driver/piraq.h"
#include "../PiraqIII_RevD_Driver/plx.h"
#include "../PiraqIII_RevD_Driver/control.h"
#include "../PiraqIII_RevD_Driver/HPIB.h"

#define CYCLE_HITS 20

///////////////////////////////////////////////////////////////////////////
CP2PIRAQ::CP2PIRAQ(
struct sockaddr_in sockAddr,
	int socketFd,
	char* destIP,
	int outputPort_, 
	char* configFname, 
	char* dspObjFname,
	unsigned int Nhits_,
	int boardnum):
PIRAQ(),
_sockAddr(sockAddr),
_socketFd(socketFd),
Nhits(Nhits_), 
outport(outputPort_), 
bytespergate(2 * sizeof(float)),
_lastPulseNumber(0),
_totalHits(0),
_boardnum(boardnum),
PNerrors(0)
{

	//	if((outsock = open_udp_out(destIP)) ==  ERROR)			/* open one socket */
	//	{
	//		printf("Could not open output socket\n"); 
	//		exit(0);
	//	}
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
	pFifo = (CircularBuffer *)this->GetBuffer(); 

	// CP2: data packets sized at runtime.  + BUFFER_EPSILON
	cp2piraq_fifo_init(
		pFifo,"/PRQDATA", 
		sizeof(PINFOHEADER), 
		Nhits * (sizeof(PINFOHEADER) + (_config.gatesa * bytespergate)), 
		PIRAQ_FIFO_NUM); 

	if (!pFifo) { 
		printf("this fifo_create failed\n"); exit(0);
	}
	printf("pFifo = %p, recordsize = %d\n", pFifo, pFifo->record_size); 

	//////////////////////////////////////////////////////////////////

	// Get the start of the PCI memory space. The packet metadata will
	// be placed there for the Piraq to read.
	_pConfigPacket = (PPACKET *)cb_get_header_address(pFifo); 

	cp2struct_init(&_pConfigPacket->info, configFname);		// initialize the info structure
	_pConfigPacket->info.flag = 0;							// Preset the flags just in case
	_pConfigPacket->info.channel = _boardnum;				// set BOARD number

	r_c = this->LoadDspCode(dspObjFname);					// load entered DSP executable filename
	printf("loading %s: this->LoadDspCode returns %d\n", dspObjFname, r_c);  
	timerset(&_config, this);								// !note: also programs pll and FIR filter. 

	this->SetCP2PIRAQTestAction(SEND_CHA);					//	send CHA by default; SEND_COMBINED after dynamic-range extension implemented 
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
int 
CP2PIRAQ::start(__int64 firstPulseNum,
				__int64 firstBeamNum) 
{
	_pConfigPacket->info.pulse_num = firstPulseNum;	// set UNIX epoch pulsenum just before starting
	_pConfigPacket->info.beam_num = firstBeamNum; 
	_pConfigPacket->info.packetflag = 1;			// set to piraq: get header! 
	// start the PIRAQ: also points the piraq to the fifo structure 
	if (!cp2start(&_config, this, _pConfigPacket))
	{
		printf("Piraq DSP program not ready: pkt->cmd.flag != TRUE (1)\n");
		return -1;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
int
CP2PIRAQ::poll() 
{
	// sleep for a ms
	Sleep(1);
	int cycle_fifo_hits = 0;
	PPACKET* p = (PPACKET *)cb_get_read_address(pFifo, 0); 
	// take CYCLE_HITS beams from piraq:
	while((
		(cb_hit(pFifo)) > 0) && 
		(cycle_fifo_hits < CYCLE_HITS)) 
	{ 
		// fifo hits ready: save #hits pending 
		cycle_fifo_hits++; 
		_totalHits++;
		if (!(_totalHits % 200))
			printf("piraq %d packets %d (seq errors %d)\n", 
			_pConfigPacket->info.channel, 
			_totalHits,
			PNerrors);

		// get the next packet in the circular buffer
		PPACKET* pFifoPiraq = (PPACKET *)cb_get_read_address(pFifo, 0); 
		//		pFifoPiraq->info.bytespergate = bytespergate; 

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
		pFifoPiraq->info.az = az;  
		pFifoPiraq->info.el = el; // set in packet 
		//////////////////////////////////////////////////////////////////////////
		//
		// check for pulse numbers out of sequence.
		for (int i = 0; i < Nhits; i++) { // all hits in the packet 
			// compute pointer to datum in an individual hit, dereference and print. 
			// CP2 PCI Bus transfer size: Nhits * (PHEADERSIZE + (config1->gatesa * bytespergate))
			__int64 thisPulseNumber = *(__int64 *)((char *)&pFifoPiraq->info.pulse_num + 
				i*((sizeof(PINFOHEADER) + 
				(_config.gatesa * bytespergate)))); 

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

		// This assumes that the number of gates does not change
		int gates = pFifoPiraq->info.gates;
		int bytespergates = pFifoPiraq->info.bytespergate;
		int hits = pFifoPiraq->info.hits;
		char desc[4];
		for(int c = 0; c < 4; c++)
			desc[c] = pFifoPiraq->info.desc[c];

		int piraqPacketSize = 
			sizeof(PINFOHEADER) + 
			gates*bytespergate;

		// set up a header
		CP2NetBeamHeader header;
		header.channel = _boardnum;
		
		// empty the packet
		_cp2Packet.clear();

		// add all beams
		for (int i = 0; i < Nhits; i++) {
			PPACKET* ppacket = (PPACKET*)((char*)&pFifoPiraq->info + i*piraqPacketSize);
			header.az        = 10.0;
			header.el        = 3.2;
			header.beam_num  = ppacket->info.beam_num;
			header.pulse_num = ppacket->info.pulse_num;
			header.gates     = ppacket->info.gates;
			header.hits      = ppacket->info.hits;
			_cp2Packet.addBeam(header, header.gates*2, ppacket->data);
		}

		int bytesSent = sendData(_cp2Packet.packetSize(),_cp2Packet.packetData());

		int udpPacketSize = sizeof(UDPHEADER)+
			sizeof(COMMAND) + 
			sizeof(INFOHEADER) + 
			gates*bytespergate;

		char* udpOut = new char[Nhits*udpPacketSize];

		for (int i = 0; i < Nhits; i++) {

			PPACKET* ppacket = (PPACKET*)((char*)&pFifoPiraq->info + i*piraqPacketSize);
			PACKET*   packet = (PACKET*) (udpOut + i*udpPacketSize);

			packet->udp.type = UDPTYPE_PIRAQ_CP2_TIMESERIES;
			packet->udp.totalsize = Nhits*udpPacketSize; // set for all of them, although it is probably only needed for the first one?
			packet->udp.magic = MAGIC;
			packet->data.info.gates = gates;
			packet->data.info.bytespergate = bytespergate;
			packet->data.info.hits = hits;
			packet->data.info.prt[0] = _prt;

			for(int c = 0; c < 4; c++)
				packet->data.info.desc[c] = desc[c];

			for (int j = 0; j < 2*gates; j++) {
				packet->data.data[j] = ppacket->data[j];
			}
		}

		// for right, cast to UDPHEADER. This only works because the
		// PUDPHEADER and UDPHEADER are currently identical. When
		// we start triming down PUDPHEADER, we will need to 
		// put a translation from PUDPHEADER to UDPHEADER. Better
		// yet, jut plan to broadcast PUDPHEADER. 
//		seq = send_udp_packet(outsock, outport, seq, (UDPHEADER*)udpOut); 

		delete []udpOut;

		//////////////////////////////////////////////////////////////////////////
		//
		// return packet to the fifo
		//
		cb_increment_tail(pFifo);

	} // end	while(fifo_hit()

	return 0;
}

///////////////////////////////////////////////////////////////////////////
int 
CP2PIRAQ::sendData(int size, 
				   void* data)
{

	DWORD bytesSent;

	// send the datagram
	WSABUF DataBuf[1];
	DataBuf[0].buf = (char *)data;
	DataBuf[0].len = size; 
	int iError = WSASendTo(
		_socketFd,
		&DataBuf[0],
		1,
		&bytesSent,
		0,
		(sockaddr *)&_sockAddr,
		sizeof(_sockAddr),
		NULL,
		NULL);

	// check for an error
	if (iError != 0) { // send error, print it
		printf("Error:%d, size:%d Bytes: %d\n", iError, bytesSent, size);
		iError = WSAGetLastError ();
		printf("WSASendTo(): iError = 0x%x\n",iError);
		bytesSent = -1;
	}

	// return the number of bytes sent, or -1 if an error.
	return bytesSent;
}

///////////////////////////////////////////////////////////////////////////
float
CP2PIRAQ::prt()
{
	return _prt;
}

PINFOHEADER
CP2PIRAQ::info()
{
	return _pConfigPacket->info;
}

///////////////////////////////////////////////////////////////////////////
void
CP2PIRAQ::stop() 
{
	stop_piraq(&_config, this);
}

///////////////////////////////////////////////////////////////////////////
void 
CP2PIRAQ::cp2piraq_fifo_init(CircularBuffer * cb, char *name, int headersize, int recordsize, int recordnum)
{
	if(cb)
	{
		/* initialize the fifo structure */
		cb->header_off = sizeof(CircularBuffer);						/* pointer to the user header */
		cb->cbbuf_off = cb->header_off + headersize;		/* pointer to cb base address */
		cb->record_size = recordsize;						/* size in bytes of each cb record */
		cb->record_num = recordnum;							/* number of records in cb buffer */
		cb->head = cb->tail = 0;							/* indexes to the head and tail records */
	}
}
///////////////////////////////////////////////////////////////////////////
void 
CP2PIRAQ::cp2struct_init(PINFOHEADER *h, char *fname)
{
	RADAR	*radar;
	CONFIG	*config;
	struct synth synthinfo[CHANNELS]; 
	FILE * SynthAngleInfo; 

	int  i; 

	h->packetsPerBlock = Nhits;

	config  = (CONFIG *)malloc(sizeof(CONFIG));
	radar = (RADAR *)malloc(sizeof(RADAR));

	readconfig(fname,config);   /* read in config.dsp and set up all parameters */
	readradar(fname,radar);	/* read in the config.rdr file */   

	h->gates 		= config->gatesa;
	h->hits			= config->hits;
	h->gates 		= config->gatesa;
	h->hits			= config->hits;//
	_xmit_pulsewidth = config->xmit_pulsewidth;
	_prt			= (float)config->prt * (8.0/(float)SYSTEM_CLOCK); // SYSTEM_CLOCK=48e6 gives 6MHz timebase 
	h->bytespergate = 2*sizeof(float); // CP2: 2 fp I,Q per gate
	h->packetflag	= 0;	// clear: set to -1 by piraq on hardware EOF detect 
	strncpy(h->desc,radar->desc,PX_MAX_RADAR_DESC);

	free(config);
	free(radar);
}

#ifdef NCAR_DRX				// NCAR drx: one piraq, no PMAC, no timer card, go.exe wait for piraq in subs.cpp\start()
#define WAIT_FOR_PIRAQ	TRUE	// start() waits for piraq to begin executing
#endif

/* start a data session */
/* wait for a DSP reset signal so no data is missed */
//!!!int start(CONFIG *config,PIRAQ *piraq)
int CP2PIRAQ::cp2start(CONFIG *config,PIRAQ *piraq, PPACKET * pkt)
{
	int  d,cnt1,cnt2,i,first;
	char c;

	int temp;
	/* stop the timer */
	piraq->GetControl()->UnSetBit_StatusRegister0((STAT0_TRESET) | (STAT0_TMODE));

	if(config->timingmode == 1)
	{ 
		piraq->GetControl()->SetBit_StatusRegister0(STAT0_TMODE);
	}
	else
	{
		piraq->GetControl()->UnSetBit_StatusRegister0(STAT0_TMODE);
	}

	/* make sure that all possible timer time-outs occur */
	Sleep(1000);    /* wait some number of milliseconds */


	/* start the DSP */
	pkt->info.flag = 0; // clear location
	piraq->StartDsp();
#if WAIT_FOR_PIRAQ
	printf("waiting for pkt->data.info.flag = 1\n");
	i = 0; 
	while((pkt->info.flag != 1) && (i++ < 10)) { // wait for DSP program to set it
		printf("still waiting for pkt->data.info.flag = 1\n"); Sleep(500); 
	} 
#else
	printf("NOT waiting for pkt->info.flag = 1\n");
#endif

	//!!!   if (pkt->cmd.flag != 1) return(FALSE); // DSP program not yet ready
	/* clear the fifo */
	//   *piraq->fifoclr = 0;

	/* take timer state machine out of reset */
	//   STATSET(STAT0_TRESET);

	//temp = *piraq->status0;
	//delay(1);
	//*piraq->status0 = temp | STAT0_TRESET;
	piraq->GetControl()->SetBit_StatusRegister0(STAT0_TRESET);

	switch(config->timingmode)
	{
	case 2:   /* continuous with sync delay */
		first = time(NULL) + 3;   /* wait up to 3 seconds */
		while(!STATUSRD1(piraq,STAT1_FIRST_TRIG))
			if(time(NULL) > first)
				::timer(1,5,config->prt2-2 ,piraq->timer);   /* odd prt (2) */
		break;
	case 0:   /* continuous (software triggered) */
	case 1:   /* external trigger */
		break;
	}

	if(!config->timingmode)  /* software trigger for continuous mode */
	{
		piraq->GetControl()->SetBit_StatusRegister0(STAT0_TMODE);
		Sleep(1);
		piraq->GetControl()->UnSetBit_StatusRegister0(STAT0_TMODE);
	}

	return(1);  /* everything is OK */
}

