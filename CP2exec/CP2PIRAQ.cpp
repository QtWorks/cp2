#include "CP2PIRAQ.h"
#include <iostream>

// The specification of the host <-> Piraq data communication:
#include "piraqComm.h"
// The PCI timer card interface:
#include "timerlib.h"
// Components of the Piraq driver:
#include "piraq.h"
#include "plx.h"
#include "control.h"
#include "HPIB.h"
#include "FirFilters.h"

///////////////////////////////////////////////////////////////////////////
CP2PIRAQ::CP2PIRAQ(
				   CP2UdpSocket* pPulseSocket,
				   char* configFname, 
				   char* dspObjFname,
				   unsigned int pulsesPerPciXfer,
				   unsigned int pmacDpramAddr,
				   int boardnum,
				   RCVRTYPE rcvrType,
				   bool doSimAngles,
				   SimAngles simAngles):
PIRAQ(),
_pPulseSocket(pPulseSocket),
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
_volume(0),
_doSimAngles(doSimAngles),
_simAngles(simAngles)
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
	printf("\n\nreading config file %s\n", configFname);
	
	readconfig(configFname, &_config);    

	_timing_mode     = _config.timingmode;
	_prt2            = _config.prt2;
	_prt             = _config.prt;
	_gates  	     = _config.gatesa;
	_hits		     = _config.hits;
	_xmit_pulsewidth = _config.xmit_pulsewidth * (8.0/(float)SYSTEM_CLOCK);;
	_prt			 = (float)_config.prt * (8.0/(float)SYSTEM_CLOCK); // SYSTEM_CLOCK=48e6 gives 6MHz timebase 
	_bytespergate    = 2*sizeof(float); 

	int r_c;   // generic return code

	r_c = this->Init(PIRAQ_VENDOR_ID,PIRAQ_DEVICE_ID); 
	if (r_c == -1) {  
		char errmsg[256]; 
		this->GetErrorString(errmsg); 
		printf("error: %s\n", errmsg); 
		return -1; 
	}

	// configure the GreyChip filters
	this->GetFilter()->ClearFilter();
	this->GetFilter()->Gaussian(SYSTEM_CLOCK, 1000000000*(8*_config.rcvr_pulsewidth/SYSTEM_CLOCK), 0);
	this->GetFilter()->StartFilter();

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
	stop_piraq();

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
	timerset(&_config);								// !note: also programs pll and FIR filter. 

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
			header.scanType  = ppacket->info.scanType;
			header.antSize   = ppacket->info.antSize;
			header.pulse_num = ppacket->info.pulse_num;
			header.gates     = ppacket->info.gates;
			header.hits      = ppacket->info.hits;
			header.status    = 0;
			header.prt       = _prt;
			header.xmit_pw   = _xmit_pulsewidth;
			if (ppacket->info.status & FIFO_EOF) {
				header.status |= PIRAQ_FIFO_EOF;
				_eof = true;
			}
			header.horiz     = ppacket->info.horiz;

			// set the antenna information. If simulating it, 
			// fabricate it using simAngles
			if (!_doSimAngles) {
				header.az        = ppacket->info.antAz * 360.0/65536;
				header.el        = ppacket->info.antEl * 360.0/65536;
				header.sweepNum  = ppacket->info.sweepNum;
				header.volNum    = ppacket->info.volNum;
				header.antTrans  = ppacket->info.antTrans;
			} else {
				_simAngles.nextAngle(header.az, header.el, header.antTrans, header.sweepNum, header.volNum);
			}

			// Save the current antenna information
			_az     = header.az;
			_el     = header.el;
			_volume = header.volNum;
			_sweep  = header.sweepNum;

			// add pulse to the outgoing packet
			_cp2Packet.addPulse(&header, header.gates*2, ppacket->data);
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
		bytesSent = _pPulseSocket->writeDatagram((const char*)data, size);
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
CP2PIRAQ::antennaInfo(double& az, double& el, 
					  unsigned int& sweep, unsigned int& volume) {
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
	stop_piraq();
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
		while(!STATUSRD1(this, STAT1_FIRST_TRIG))
			if(time(NULL) > first)
				timerRegisterSet(1,5,_prt2-2 ,this->timer);   /* odd prt (2) */
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

#define STOPDELAY 30

#define REF     100e3

#define NCAR_DRX

//void delay(int time); //!!!all calls replaced w/Sleep() w/no change to passed parameter


///////////////////////////////////////////////////////////////////////////
int  
CP2PIRAQ::timerset(CONFIG *config)
{
	int  S,D,N,i,clk,freq,glen,spare23;
	int  rcvr_pulsewidth; // local computed on 24/32 MHz timebase: config->rcvr_pulsewidth computed on 6/8 MHz

	rcvr_pulsewidth = config->rcvr_pulsewidth*4; 

	// use local rcvr_pulsewidth instead of config->rcvr_pulsewidth; local (and code below) assumes 24/32 MHz timebase. 
	/* check for mode within bounds */
	if(config->timingmode < 0 || config->timingmode > 2)
	{printf("TIMERSET: invalid mode %d\n",config->timingmode); return(0);}

	/* check for gate0mode within bounds */
	if(config->gate0mode < 0 || config->gate0mode > 1)
	{printf("TIMERSET: invalid gate0mode %d\n",config->gate0mode); return(0);}

	/* check for receiver pulsewidth within bounds */
	/* the pulsewidth defines the FIFO clock rate in 32MHz counts (or 24MHz) */
	if(rcvr_pulsewidth == 0)
	{printf("TIMERSET: invalid receiver pulsewidth count: %d\n",rcvr_pulsewidth); return(0);}

	if(rcvr_pulsewidth >= 32 && rcvr_pulsewidth < 64 && (rcvr_pulsewidth & 1))
	{printf("TIMERSET: invalid receiver pulsewidth count: %d\n",rcvr_pulsewidth);
	printf("            If receiver pulse width greater than 32, it must be even\n",rcvr_pulsewidth);	 return(0);}

	if(rcvr_pulsewidth >= 64 && (rcvr_pulsewidth & 3))
	{printf("TIMERSET: invalid receiver pulsewidth count: %d\n",rcvr_pulsewidth);
	printf("            If pulse width greater than 64, it must be divisible by four\n",rcvr_pulsewidth);	 return(0);}

	if((config->gatesa * rcvr_pulsewidth)<12)
	{printf("TIMERSET: number of Channel A gates X receiver pulsewidth must be 12 or greater\n"); return(0);}

	if((config->gatesb * rcvr_pulsewidth)<12)
	{printf("TIMERSET: number of Channel B gates X receiver pulsewidth must be 12 or greater\n"); return(0);}

	/* check for numbergates within bounds */
	if(config->gatesa == 0)
	{printf("TIMERSET: invalid number of gates for channel A: %d\n",config->gatesa); return(0);}

	/* check for numbergates within bounds */
	if(config->gatesb == 0)
	{printf("TIMERSET: invalid number of gates for channel B: %d\n",config->gatesb); return(0);}

	/* check to see that the sampling interval can be measured in 8 MHz counts */
	if((config->gatesa * rcvr_pulsewidth) & 3)
	{printf("TIMERSET: Invalid number of gates/receiver pulsewidth combination %d %d for Channel A\n",config->gatesa,rcvr_pulsewidth);
	printf("            number of gates X pulsewidth should be a multiple of 4\n");
	return(0);}

	if((config->gatesb * rcvr_pulsewidth) & 3)
	{printf("TIMERSET: Invalid number of gates/receiver pulsewidth combination %d %d for Channel B\n",config->gatesb,rcvr_pulsewidth);
	printf("            number of gates X pulsewidth should be a multiple of 4\n");
	return(0);}

	/* check for sync timecounts within bounds */
	if((config->sync == 0 || config->sync > 65535) && config->timingmode != 2)
	{printf("TIMERSET: invalid sync delay %d\n",config->sync); return(0);}

	/* check for delay timecounts within bounds */
	if((config->delay == 0 || config->delay > 65535) && config->timingmode != 2)
	{printf("TIMERSET: invalid delay %d\n",config->delay); return(0);}

	/* check for tpdelay timecounts within bounds */
	if(config->tpdelay >= 65536 || config->tpdelay <= -65536)
	{printf("TIMERSET: invalid testpulse delay %d\n",config->tpdelay); return(0);}

	/* check for prt timecounts within bounds */
	if(config->prt == 0 || config->prt > 65535)
	{printf("TIMERSET: invalid prt %d\n",config->prt); return(0);}

	/* check for prt2 timecounts within bounds */
	if(config->prt2 == 0 || config->prt2 > 65535)
	{printf("TIMERSET: invalid prt %d\n",config->prt2); return(0);}

	/* double check that all the gates and the delay fit into a prt */
	if(config->prt <= rcvr_pulsewidth * (config->gatesa + 1) / 4.0)
	{
		printf("TIMERSET: Gates, Delay, and EOF do not fit in PRT of %8.2f uS\n",config->prt/10.0); 
		printf("          Delay is %8.2f uS (%d 10 MHz counts)\n",config->delay/10.0,config->delay);
		printf("          %d gates at %8.2f uS (%7.1f m) + EOF = %8.2f uS\n"
			,config->gatesa
			,rcvr_pulsewidth/10.0
			,1e-6*C*.5*rcvr_pulsewidth/10.0
			,(config->gatesa + 1) * rcvr_pulsewidth / 10.0);
		return(0);
	}

	/* stop the timer */
	GetControl()->StopTimers();

	GetControl()->UnSetBit_StatusRegister1(STAT1_EXTTRIGEN);	

	Sleep(STOPDELAY);       /* wait for all timers to time-out */

	/* program all the timers */
	/* the three timers of chip 0 */
	switch(config->timingmode)
	{
	case 0:   /* software free running mode */
		// !!!
		printf("TIMERSET: config->prt = %d config->prt2 = %d rcvr_pulsewidth = %d\n",config->prt, config->prt2, rcvr_pulsewidth); 
		timerRegisterSet(0,5,config->prt-2                    ,timer);   /* even - odd prt */
		timerRegisterSet(1,5,config->prt2-2   ,timer); /* even prt (1) */

		printf("tpdelay = %d\n***********************************************************\n",config->tpdelay);
		if(config->tpdelay >= 0) /* handle the zero case here */
		{
			if(config->tpdelay > 0)
				timerRegisterSet(2,5,config->tpdelay,timer); /* delay from gate0 to tp */
			else
				timerRegisterSet(2,5,1,timer); /* delay from gate0 to tp */
		}
		else
		{
			timerRegisterSet(2,5,-config->tpdelay,timer); /* delay from tp to gate0 */
		}
		break;
	case 1:   /* external trigger mode */
		timerRegisterSet(0,5,config->prt-2,timer);   /* even - odd prt */
		if(config->tpdelay >= 0)        /* handle the 0 case here */
		{
			timerRegisterSet(1,5,config->delay  ,timer); /* delay from trig to gate0 */
			if(config->tpdelay == 0)
				timerRegisterSet(2,5,config->tpdelay+1,timer); /* delay from gate0 to tp */
			else
				timerRegisterSet(2,5,config->tpdelay  ,timer); /* delay from gate0 to tp */
		}
		else   /* if tp comes before gate0 */
		{
			if(config->delay + config->tpdelay > 0)
			{
				timerRegisterSet(1,5,config->delay + config->tpdelay,timer); /* delay from trig to tp */
				timerRegisterSet(2,5,-config->tpdelay,timer); /* delay from tp to gate0 */
			}
			else
			{
				if(config->delay > 1)
				{
					timerRegisterSet(1,5,1,timer); /* delay from trig to tp */
					timerRegisterSet(2,5,config->delay-1,timer); /* delay from tp to gate0 */
				}
				else
				{
					timerRegisterSet(1,5,1,timer); /* delay from trig to tp */
					timerRegisterSet(2,5,1,timer); /* delay from tp to gate0 */
				}
			}
		}
		break;
	case 2:   /* external sync free running mode */
		timerRegisterSet(0,5,config->prt-2,timer);   /* even - odd prt */
		timerRegisterSet(1,5,config->sync,timer); /* even prt (1) */
		if(config->tpdelay >= 0) /* handle the zero case here */
		{
			if(config->tpdelay > 0)
				timerRegisterSet(2,5,config->tpdelay,timer); /* delay from gate0 to tp */
			else
				timerRegisterSet(2,5,1,timer); /* delay from gate0 to tp */
		}
		else
		{
			timerRegisterSet(2,5,-config->tpdelay,timer); /* delay from tp to gate0 */
		}
		break;
	}

	/* the three timers of chip 1 */
	timerRegisterSet(0,1,config->tpwidth,timer + 8);   /* test pulse width */
	timerRegisterSet(1,5,(config->gatesa * rcvr_pulsewidth)/4-2 ,timer + 8);   /* number of A gates */
	timerRegisterSet(2,5,(config->gatesb * rcvr_pulsewidth)/4-2 ,timer + 8);   /* number of B gates */

	/* gate length control */
	/* depending on the gate length, the FIR is either in /1 , /2 or /4 mode */
	if(rcvr_pulsewidth < SYSTEM_CLOCK/2.0e6) // < 1uSec: divisor timebase counts/uSec
	{
		glen = ((rcvr_pulsewidth & 0x1F)-1) << 9; 
		spare23 = 0x000;
	}
	//!!!   else if(config->pulsewidth < 64) 
	else if(rcvr_pulsewidth < 2*(SYSTEM_CLOCK/2.0e6)) // < 2uSec 
	{
		glen = ((rcvr_pulsewidth & 0x3E)-2) << 8;
		spare23 = 0x400;
	}
	else
	{
		glen = ((rcvr_pulsewidth & 0x7C)-4) << 7;
		spare23 = 0x800;
	}
	printf("glen = %d rcvr_pulsewidth = %d\n",glen,rcvr_pulsewidth);

	/* set up all PIRAQ registers */ //???
	GetControl()->SetValue_StatusRegister0(
		glen | (config->testpulse << 6)   |	 /* odd and even testpulse bits */
		(config->trigger << 4)              |  /* odd and even trigger bits */
		((config->timingmode == 1) ? STAT0_TMODE : 0) |  /* tmode timing mode */
		//	 ((config->timingmode == 2) ? STAT0_TRIGSEL : 0) |  /* if sync mode, then choose 1PPS input */
		//         ((config->gate0mode == 1) ? (STAT0_GATE0MODE /* | STAT0_SWCTRL */) : 0) |
		//	 ((config->gate0mode == 1) ? (STAT0_GATE0MODE | STAT0_SWCTRL) : 0) |
		((config->tpdelay >= 0) ? STAT0_DELAY_SIGN : 0) | (STAT0_SW_RESET) // !TURN RESET OFF! mp 10-18-02 see singlepiraq ongoing
		//         STAT0_SWSEL | STAT0_SWCTRL       /* set to channel B (top channel) */
		);   /* start from scratch */

	GetControl()->SetValue_StatusRegister1(spare23);

	pll(10e6, SYSTEM_CLOCK, 50e3);
	Sleep(STOPDELAY);   //	??I,Q unequal amplitude on first powerup: 6-22-06 mp
	return(1);
}

///////////////////////////////////////////////////////////////////////////
void  
CP2PIRAQ::timerRegisterSet(int timernum,
						   int timermode,
						   int count,
						   unsigned short *iobase)
{
	if(timernum < 0 || timernum > 2)
	{printf("TIMER: invalid timernum %d\n",timernum); return;}
	if(timermode < 0 || timermode > 5)
	{printf("TIMER: invalid timermode %d\n",timermode); return;}

	Sleep(1);
	*((volatile short *)iobase +     3   ) = (0x30 + timernum * 0x40 + timermode * 2);
	Sleep(1);
	*((volatile short *)iobase + timernum) = count;
	Sleep(1);
	*((volatile short *)iobase + timernum) = (count >> 8);
}

///////////////////////////////////////////////////////////////////////////
void     
CP2PIRAQ::stop_piraq()
{
	int  i;

	/* stop the timer */
	//STATCLR0(piraq,STAT0_TRESET | STAT0_TMODE);
	GetControl()->UnSetBit_StatusRegister0(STAT0_TRESET | STAT0_TMODE);	

	/* make sure that all possible timer time-outs occur */
	Sleep(STOPDELAY);    /* wait some number of milliseconds */

	StopDsp();
}




///////////////////////////////////////////////////////////////////////////
void  
CP2PIRAQ::pll(double ref, double freq, double cmpfreq)
{
	int  i,data,r,n,a;

	/* initialize and configure the pll at least once */
	/* this needs to be done every time, following the convoluted QNX code, and experimental results! ... here simplified */
	plldata(0x10FA93);     /* r data with reset bit */

	/* compute the data to go into the register */
	n = 0.5 + freq / cmpfreq;
	a = n & 7;
	n /= 8;
	r = 0.5 + ref / cmpfreq;

	plldata(r << 2 | 0x100000);     /* r data with reset bit */
	plldata(n << 7 | a << 2 | 0x100001);     /* r data with reset bit */
	Sleep(1); // ape delay(1); on QNX box
}

///////////////////////////////////////////////////////////////////////////
void 
CP2PIRAQ::plldata(int data)
{
	int  i;
	/*----------------------*/
	/* PLL bits definition  */
	/*----------------------*/
	/*  2   |   1   |   0   */
	/*------+-------+-------*/
	/* DATA |  LE   | CLOCK */
	/*----------------------*/

	int Status1; 
	for(i=0; i<21; i++,data<<=1) {
		if (data & 0x100000) { // data bit set 
			GetControl()->SetBit_StatusRegister1(STAT1_PLL_DATA);   // set PLL data bit 
			GetControl()->SetBit_StatusRegister1(STAT1_PLL_CLOCK); // set PLL data bit, set clock  
		} 
		else { // data bit clear 
			GetControl()->UnSetBit_StatusRegister1(STAT1_PLL_DATA); // clear PLL data bit
			GetControl()->SetBit_StatusRegister1(STAT1_PLL_CLOCK);  // set clock
		} 
		GetControl()->UnSetBit_StatusRegister1(STAT1_PLL_CLOCK);    // clear PLL clock bit
	} 
	/* toggle LE signal */
	GetControl()->SetBit_StatusRegister1(STAT1_PLL_LE);   // set LE
	GetControl()->UnSetBit_StatusRegister1(STAT1_PLL_LE); // clear LE
}

