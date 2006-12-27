#include <winsock2.h>
#include "CP2PIRAQ.h"

#include "piraqComm.h"

#include "subs.h"
#include "timerlib.h"
#include "subs.h"

#include "piraq.h"
#include "plx.h"
#include "control.h"
#include "HPIB.h"

#define CYCLE_HITS 20

///////////////////////////////////////////////////////////////////////////
CP2PIRAQ::CP2PIRAQ(
struct sockaddr_in sockAddr,
	int socketFd,
	char* destIP,
	int outputPort_, 
	char* configFname, 
	char* dspObjFname,
	unsigned int pulsesPerPciXfer,
	unsigned int pmacDpramAddr,
	int boardnum):
PIRAQ(),
_sockAddr(sockAddr),
_socketFd(socketFd),
_pulsesPerPciXfer(pulsesPerPciXfer), 
_outport(outputPort_),
_lastPulseNumber(0),
_totalHits(0),
_pmacDpramAddr(pmacDpramAddr),
_boardnum(boardnum),
_PNerrors(0)
{
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

	_timing_mode     = _config.timingmode;
	_prt2            = _config.prt2;
	_prt             = _config.prt;
	_gates  	     = _config.gatesa;
	_hits		     = _config.hits;
	_xmit_pulsewidth = _config.xmit_pulsewidth;
	_prt			 = (float)_config.prt * (8.0/(float)SYSTEM_CLOCK); // SYSTEM_CLOCK=48e6 gives 6MHz timebase 
	_bytespergate    = 2*sizeof(float); // CP2: 2 fp I,Q per gate

	int r_c;   // generic return code

	r_c = this->Init(PIRAQ_VENDOR_ID,PIRAQ_DEVICE_ID); 
	if (r_c == -1) {  
		char errmsg[256]; 
		this->GetErrorString(errmsg); 
		printf("error: %s\n", errmsg); 
		return -1; 
	}

	/* put the DSP into a known state where HPI reads/writes will work */
	this->ResetPiraq();  
	this->GetControl()->UnSetBit_StatusRegister0(STAT0_SW_RESET);
	Sleep(1);
	this->GetControl()->SetBit_StatusRegister0(STAT0_SW_RESET);
	Sleep(1);

	// read and print the eprom values
	unsigned int EPROM1[128]; 
	this->ReadEPROM(EPROM1);
	for(int y = 0; y < 16; y++) {
		printf("%08lX %08lX %08lX %08lX\n",EPROM1[y*4],EPROM1[y*4+1],EPROM1[y*4+2],EPROM1[y*4+3]); 
	}

	//	send CHA by default; SEND_COMBINED after dynamic-range extension implemented
	this->SetCP2PIRAQTestAction(SEND_CHA);	 
	stop_piraq(&_config, this);

	// create the data fifo and initialize members.
	_pFifo = (CircularBuffer *)this->GetBuffer(); 
	if (!_pFifo) { 
		printf("this fifo_create failed\n"); exit(0);
	}

	_pFifo->header_off = sizeof(CircularBuffer);						/* pointer to the user header */
	_pFifo->cbbuf_off = _pFifo->header_off + sizeof(PINFOHEADER);		/* pointer to cb base address */
	_pFifo->record_size = _pulsesPerPciXfer * (sizeof(PINFOHEADER) 
		+ (_gates * _bytespergate));    /* size in bytes of each cb record */
	_pFifo->record_num = PIRAQ_FIFO_NUM;							/* number of records in cb buffer */
	_pFifo->head = _pFifo->tail = 0;							/* indexes to the head and tail records */

	printf("_pFifo = %p, recordsize = %d\n", _pFifo, _pFifo->record_size); 

	//////////////////////////////////////////////////////////////////

	// Get the start of the PCI memory space. The config packet will
	// be placed there for the Piraq to read.
	_pConfigPacket = (PPACKET *)cb_get_header_address(_pFifo); 

	_pConfigPacket->info.gates = _gates;
	_pConfigPacket->info.hits  = _hits;
	_pConfigPacket->info.bytespergate = 2*sizeof(float);              // CP2: 2 fp I,Q per gate
	_pConfigPacket->info.packetsPerBlock = _pulsesPerPciXfer;
	_pConfigPacket->info.flag = 0;							          // Preset the flags just in case
	_pConfigPacket->info.channel = _boardnum;				          // set BOARD number
	// set the pmac dpram address
	_pConfigPacket->info.PMACdpramAddr = _pmacDpramAddr;

	r_c = this->LoadDspCode(dspObjFname);					// load entered DSP executable filename
	printf("loading %s: this->LoadDspCode returns %d\n", dspObjFname, r_c);  
	timerset(&_config, this);								// !note: also programs pll and FIR filter. 

	this->SetCP2PIRAQTestAction(SEND_CHA);					//	send CHA by default; SEND_COMBINED after dynamic-range extension implemented 
	return 0;
}


/////////////////////////////////////////////////////////////////////////////
int
CP2PIRAQ::poll() 
{
	// sleep for a ms
	Sleep(1);
	int cycle_fifo_hits = 0;
	PPACKET* p = (PPACKET *)cb_get_read_address(_pFifo, 0); 
	// take CYCLE_HITS beams from piraq:
	while((
		(cb_hit(_pFifo)) > 0) && 
		(cycle_fifo_hits < CYCLE_HITS)) 
	{ 
		// fifo hits ready: save #hits pending 
		cycle_fifo_hits++; 
		_totalHits++;
		if (!(_totalHits % 200))
			printf("piraq %d packets %d (seq errors %d)\n", 
			_pConfigPacket->info.channel, 
			_totalHits,
			_PNerrors);

		// get the next packet in the circular buffer
		PPACKET* _pFifoPiraq = (PPACKET *)cb_get_read_address(_pFifo, 0); 

		// send data out on the socket
		int piraqPacketSize = 
			sizeof(PINFOHEADER) +  
			_pFifoPiraq->info.gates*_pFifoPiraq->info.bytespergate;

		// set up a header
		CP2PulseHeader header;
		header.channel = _boardnum;

		// empty the packet
		_cp2Packet.clear();

		// add all beams to the outgoing packet
		for (int i = 0; i < _pulsesPerPciXfer; i++) {
			PPACKET* ppacket = (PPACKET*)((char*)&_pFifoPiraq->info + i*piraqPacketSize);
			header.antAz     = ppacket->info.antAz;
			header.antEl     = ppacket->info.antEl;
			header.scanType  = ppacket->info.scanType;
			header.volNum    = ppacket->info.volNum;
			header.sweepNum  = ppacket->info.sweepNum;
			header.antSize   = ppacket->info.antSize;
			header.antTrans  = ppacket->info.antTrans;
			header.beam_num  = ppacket->info.beam_num;
			header.pulse_num = ppacket->info.pulse_num;
			header.gates     = ppacket->info.gates;
			header.hits      = ppacket->info.hits;
			_cp2Packet.addPulse(header, header.gates*2, ppacket->data);

			// check for pulse number errors
			long long thisPulseNumber = ppacket->info.pulse_num;
			if (_lastPulseNumber != thisPulseNumber - 1) {
				printf("pulse number out of sequence, last PN %I64d, this PN %I64d\n", 
					_lastPulseNumber, thisPulseNumber);  
				_PNerrors++; 
			}
			_lastPulseNumber = thisPulseNumber; // previous hit PN
		}

		int bytesSent = sendData(_cp2Packet.packetSize(),_cp2Packet.packetData());

		CP2Packet readBack;
		bool error = readBack.setData(_cp2Packet.packetSize(), _cp2Packet.packetData());
		if (error) {
			printf("error decoding packet\n");
		} else {
			for (int i = 0; i < readBack.numPulses(); i++) {
				CP2Pulse* pPulse = readBack.getPulse(i);
				long long pulse = pPulse->header.pulse_num;
				long long beam = pPulse->header.beam_num;
				double az = pPulse->header.antAz;
				double el = pPulse->header.antEl;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		//
		// return packet to the piraq fifo
		//
		cb_increment_tail(_pFifo);

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

int CP2PIRAQ::start(long long firstPulseNum,
					long long firstBeamNum)
{
	_pConfigPacket->info.pulse_num = firstPulseNum;	// set UNIX epoch pulsenum just before starting
	_pConfigPacket->info.beam_num = firstBeamNum; 
	_pConfigPacket->info.packetflag = 1;			// set to piraq: get header! 

	int  d,cnt1,cnt2,i,first;
	char c;

	int temp;
	/* stop the timer */
	this->GetControl()->UnSetBit_StatusRegister0((STAT0_TRESET) | (STAT0_TMODE));

	if(_timing_mode == 1)
	{ 
		this->GetControl()->SetBit_StatusRegister0(STAT0_TMODE);
	}
	else
	{
		this->GetControl()->UnSetBit_StatusRegister0(STAT0_TMODE);
	}

	/* make sure that all possible timer time-outs occur */
	Sleep(1000);    /* wait some number of milliseconds */


	/* start the DSP */
	_pConfigPacket->info.flag = 0; // clear location
	this->StartDsp();

	// wait for the piraq to signal that it is ready.
	printf("waiting for pkt->data.info.flag = 1\n");
	i = 0; 
	while((_pConfigPacket->info.flag != 1) && (i++ < 10)) { // wait for DSP program to set it
		printf("still waiting for pkt->data.info.flag = 1\n"); 
		Sleep(500); 
	} 

	this->GetControl()->SetBit_StatusRegister0(STAT0_TRESET);

	switch(_timing_mode)
	{
	case 2:   /* continuous with sync delay */
		first = time(NULL) + 3;   /* wait up to 3 seconds */
		while(!STATUSRD1(this,STAT1_FIRST_TRIG))
			if(time(NULL) > first)
				::timer(1,5,_prt2-2 ,this->timer);   /* odd prt (2) */
		break;
	case 0:   /* continuous (software triggered) */
	case 1:   /* external trigger */
		break;
	}

	if(!_timing_mode)  /* software trigger for continuous mode */
	{
		this->GetControl()->SetBit_StatusRegister0(STAT0_TMODE);
		Sleep(1);
		this->GetControl()->UnSetBit_StatusRegister0(STAT0_TMODE);
	}

	return(0);  /* everything is OK */
}

