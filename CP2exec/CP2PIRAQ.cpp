#include "CP2PIRAQ.h"

#include <iostream>
#include "piraqComm.h"

#include "subs.h"
#include "timerlib.h"
#include "subs.h"

#include "piraq.h"
#include "plx.h"
#include "control.h"
#include "HPIB.h"

///////////////////////////////////////////////////////////////////////////
CP2PIRAQ::CP2PIRAQ(
				   QHostAddress* pHostAddr,
				   int portNumber,
				   QUdpSocket* pSocketDevice,
				   char* configFname, 
				   char* dspObjFname,
				   unsigned int pulsesPerPciXfer,
				   unsigned int pmacDpramAddr,
				   int boardnum,
				   RCVRTYPE rcvrType):
PIRAQ(),
_pHostAddr(pHostAddr),
_portNumber(portNumber),
_pSocketDevice(pSocketDevice),
_pulsesPerPciXfer(pulsesPerPciXfer), 
_lastPulseNumber(0),
_totalHits(0),
_pmacDpramAddr(pmacDpramAddr),
_boardnum(boardnum),
_rcvrType(rcvrType),
_PNerrors(0),
_eof(false),
_nPulses(0),
_sampleRate(0),
_resendCount(0),
_az(0),
_el(0),
_sweep(0),
_volume(0)
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

	printf("Circular buffer address is %p, recordsize = %d\n", _pFifo, _pFifo->record_size);
	int cbTotalSize = sizeof(CircularBuffer) + PIRAQ_FIFO_NUM*_pFifo->record_size;
	printf("Total circular buffer size = %d (0x%08x)\n", cbTotalSize, cbTotalSize); 

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
	_pConfigPacket->info.rcvrType = _rcvrType;
	// set the pmac dpram address
	_pConfigPacket->info.PMACdpramAddr = _pmacDpramAddr;

	char* d = new char[strlen(dspObjFname)+1];
	strcpy(d, dspObjFname);
	r_c = this->LoadDspCode(d);					// load entered DSP executable filename
	delete [] d;
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

	int pulses = 0;

	int cycle_fifo_hits = 0;
	PPACKET* p = (PPACKET *)cb_get_read_address(_pFifo, 0); 
	// take CYCLE_HITS beams from piraq:
	while((cb_hit(_pFifo)) > 0) { 
		// fifo hits ready: save #hits pending 
		cycle_fifo_hits++; 
		_totalHits++;
		if (!(_totalHits % 200)) {
			int currentTick = GetTickCount();
			double delta = currentTick - _lastTickCount;
			_sampleRate = 1000.0*200.0*_pulsesPerPciXfer/delta;
			_lastTickCount = currentTick;
		}

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

		// add all beams to the outgoing packet and to the pulse queue
		for (int i = 0; i < _pulsesPerPciXfer; i++) {
			PPACKET* ppacket = (PPACKET*)((char*)&_pFifoPiraq->info + i*piraqPacketSize);
			header.antAz     = ppacket->info.antAz;
			header.antEl     = ppacket->info.antEl;
			header.scanType  = ppacket->info.scanType;
			header.volNum    = ppacket->info.volNum;
			header.sweepNum  = ppacket->info.sweepNum;
			header.antSize   = ppacket->info.antSize;
			header.antTrans  = ppacket->info.antTrans;
			header.pulse_num = ppacket->info.pulse_num;
			header.gates     = ppacket->info.gates;
			header.hits      = ppacket->info.hits;
			header.status    = 0;
			if (ppacket->info.status & FIFO_EOF) {
				header.status |= PIRAQ_FIFO_EOF;
				_eof = true;
			}
			header.horiz     = ppacket->info.horiz;

			_az     = header.antAz;
			_el     = header.antEl;
			_volume = header.volNum;
			_sweep  = header.sweepNum;

			// add pulse to the outgoing packet
			_cp2Packet.addPulse(header, header.gates*2, ppacket->data);
			pulses++;

			// check for pulse number errors
			long long thisPulseNumber = ppacket->info.pulse_num;
			if (_lastPulseNumber != thisPulseNumber - 1 && _lastPulseNumber) {
				printf("pulse number out of sequence, delta %I64d\n", 
					thisPulseNumber - _lastPulseNumber);  
				_PNerrors++; 
			}
			_lastPulseNumber = thisPulseNumber; // previous PN

			_nPulses++;
			/**
			if (!(_nPulses %1000 )) {
			double factor = 360.0/65536;
			short az = ppacket->info.antAz;
			short el = ppacket->info.antEl;
			short scan = ppacket->info.scanType;
			short vol = ppacket->info.volNum;
			short sweep = ppacket->info.sweepNum;
			printf("piraq %d resends %06d az %6.1f el %6.1f sweep %05d scan type %05d vol %05d\n",
			_boardnum, _resendCount, az*factor, el*factor, sweep, scan, vol);
			}
			**/
		}

		int bytesSent = sendData(_cp2Packet.packetSize(),_cp2Packet.packetData());

		//////////////////////////////////////////////////////////////////////////
		//
		// return packet to the piraq fifo
		//
		cb_increment_tail(_pFifo);

	} // end	while(fifo_hit()

	return pulses;
}

///////////////////////////////////////////////////////////////////////////
int 
CP2PIRAQ::sendData(int size, 
				   void* data)
{
	int bytesSent;

	/// @todo Instead of just looping here, we should look into
	/// the logic of coming back later and doing the resend. I tried
	/// putting a Sleep(1) before each resend; for some strange reason
	/// this caused two of the three piraqs to stop sending data.
	do {
		bytesSent = _pSocketDevice->writeDatagram((const char*)data, size, *_pHostAddr, _portNumber);
		if (bytesSent != size) {
			_resendCount++;
		}
	} while (bytesSent != size);

	return bytesSent;
}

///////////////////////////////////////////////////////////////////////////
int
CP2PIRAQ::pnErrors()
{
	return _PNerrors;
}


///////////////////////////////////////////////////////////////////////////
double
CP2PIRAQ::sampleRate()
{
	return _sampleRate;;
}
///////////////////////////////////////////////////////////////////////////
void
CP2PIRAQ::antennaInfo(unsigned short& az, unsigned short& el, 
					  unsigned short& sweep, unsigned short& volume) {
	az     = _az;
	el     = _el;
	sweep  = _sweep;
	volume = _volume;
}

///////////////////////////////////////////////////////////////////////////
float
CP2PIRAQ::prt()
{
	return _prt;
}

///////////////////////////////////////////////////////////////////////////
float
CP2PIRAQ::xmit_pulsewidth()
{
	return _xmit_pulsewidth;
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
bool
CP2PIRAQ::eof() 
{
	bool retval = _eof;
	_eof = false;
	return retval;
}

///////////////////////////////////////////////////////////////////////////
int CP2PIRAQ::start(long long firstPulseNum)
{
	_pConfigPacket->info.pulse_num = firstPulseNum;	// set UNIX epoch pulsenum just before starting

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
		Sleep(500); 
		printf("still waiting for pkt->data.info.flag = 1\n"); 
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

	// save the current tick count for frame rate calculations
	_lastTickCount = GetTickCount();

	return(0);  /* everything is OK */
}

