//
#include "qtdsp.h"
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qspinbox.h>
#include <qlineedit.h>

#include <iostream>

#include "Fifo.h"	//	

QtDSP::QtDSP():
m_pDataSocket(0),    
m_pDataSocketNotifier(0),
m_pSocketBuf(0),
_fifo1(0),	//	std::vector, float datatype
_fifo2(0),
_fifo3(0)
{
	m_receiving		= 0;		//	receive-data state
	m_receivePacketCount	= 0; 
	//	set default destination, notify ports:
	char	_postIt[10];		//	too small to be a scratchpad
	sprintf(_postIt, "%d", CP2EXEC_BASE_PORT);	_receivePortBase->setText( QString( _postIt ) ); 
	sprintf(_postIt, "%d", QTDSP_BASE_PORT);	_IQdestinationPortBase->setText( QString( _postIt ) ); 
	sprintf(_postIt, "%d", QTDSP_BASE_PORT + 10);	_ABPdestinationPortBase->setText( QString( _postIt ) ); 
	//	default Destination Hostname to this machine: 
	m_pDataSocket = new QSocketDevice(QSocketDevice::Datagram);	//	exit() below without this 
	char nameBuf[256];
	if (gethostname(nameBuf, sizeof(nameBuf))) {
		statusLog->append("gethostname failed");
		exit(1);
	}
	struct hostent* pHostEnt = gethostbyname(nameBuf);
	if (!pHostEnt) {
		statusLog->append("gethostbyname failed");
		exit(1);
	}
	QString myIPaddress = inet_ntoa(*(struct in_addr*)pHostEnt->h_addr_list[0]);
	_destinationHostname->setText(myIPaddress);
	delete	m_pDataSocket; 

	// set current status: 
	_currentStatus->setText( tr ("QtDSP initialized") ); 
	statusLog->append( tr ("QtDSP initialized") ); 
	// Start a timer to monitor data channel functioning 
	m_dataDisplayTimer = startTimer(QTDSP_MONITOR_INTERVAL);


	//	create FIFOs for 3 PIRAQs 
	sprintf(m_statusLogBuf, "FIFOs created for data channels 1, 2, 3: std::vector implementation"); 
	_fifo1 = new Fifo(0);
	_fifo2 = new Fifo(0);
	_fifo3 = new Fifo(0);
	rcvChannel[0]._fifo = _fifo1;
	rcvChannel[1]._fifo = _fifo2;
	rcvChannel[2]._fifo = _fifo3;
	for (int i = 0;  i < 3; i++) {
		rcvChannel[i]._fifo_hits = 0;
		rcvChannel[i].pnErrors = 0;
		rcvChannel[i].lastreadBufLen = -1;
	}
}

QtDSP::~QtDSP() {

	if	(m_receiving)	{	//	currently receiving on input: generalize
		for (int i = 0; i < QTDSP_RECEIVE_CHANNELS; i++) {	//	all receive channels
			terminateReceiveSocket(&rcvChannel[i]);
		}
	}
	delete _fifo1; 
	delete _fifo2; 
	delete _fifo3; 
}

int
QtDSP::getParameters( CP2_PIRAQ_DATA_TYPE[], int rcvChan)	// get program operating parameters from piraqx (or other) "housekeeping" header structure 
{
	char * _radarDesc; 
	//	local copies for use here; update struct at end
	uint4	_gates;
	uint4	_hits;
	uint4	_radarType;		//	set by field "desc", describing the radar function

	//	parameters from received data
	_gates = *(int *)((rcvChannel[rcvChan]._SocketBuf) + 108/sizeof(int));	//	gates: 32+28+48 as expected; 
	_hits =  *(int *)((rcvChannel[rcvChan]._SocketBuf) + 116/sizeof(int));	//	hits: 32+28+56  
	_radarDesc = (char *)(rcvChannel[rcvChan]._SocketBuf) + 60;	//	desc: 32+28+0

	//	working parameters for processing data: 
	_pulseStride = (sizeof(PACKETHEADER) + 8*_gates)/sizeof(float);	//	stride in floats; 2nd term DATASIZE: 8 bytes per gate wired for now 
	//	compute #hits combined by piraq: equal in both piraq executable (CP2_DCCS3_1.out) and all host applications
	_Nhits = UDPSENDSIZEMAX / (HEADERSIZE + (8*_gates));	//	8 = bytes per gate: define
	if	(_Nhits % 2)	//	odd #hits
		_Nhits--;		//	make it even: PIRAQ sends even #hits per packet
	_PNOffset = 32 + 28 + 32;	//	pulsenumber offset from begin of piraqx packet: sizeof(COMMAND) + sizeof(UDPHEADER) + offset to PN
	_IQdataOffset = sizeof(PACKETHEADER);	//	offset in bytes to begin of data in (piraqx) packet
	if	(!strncmp(_radarDesc,"SVH",3))	{	//	S-band alternating VHVHVH transmission
		_radarType = SVH;
	}
	else if	(!strncmp(_radarDesc,"XH",2))	{	//	X-band horizontal polarization 
		_radarType = XH;
	}
	else if	(!strncmp(_radarDesc,"XV",2))	{	//	X-band vertical polarization 
		_radarType = XV;
	}
	else
		_radarType = CP2RADARTYPES;				//	this is effectively an error code
	if	(_radarType == CP2RADARTYPES)	{	//	
		sprintf(m_statusLogBuf, "Radar Type = %s not supported", _radarDesc); statusLog->append( m_statusLogBuf ); 
		sprintf(m_statusLogBuf, "supported types SVH XV XH"); statusLog->append( m_statusLogBuf ); 
		//	deactivate this receiver channel
	}
	else
		sprintf(m_statusLogBuf, "Radar Type = %s", _radarDesc); statusLog->append( m_statusLogBuf ); 

	//	update the structure: 
	rcvChannel[rcvChan]._gates = _gates;	 
	rcvChannel[rcvChan]._hits = _hits;	 
	rcvChannel[rcvChan]._radarType = _radarType;	 
	//	report results to status window: 
	sprintf(m_statusLogBuf, "receive channel %d parameters:", rcvChan); statusLog->append( m_statusLogBuf ); 
	sprintf(m_statusLogBuf, "_gates = %d _hits = %d", _gates, _hits); statusLog->append( m_statusLogBuf ); 
	sprintf(m_statusLogBuf, "_Nhits = %d", _Nhits); statusLog->append( m_statusLogBuf ); 
	return(1);	//	condition this 
}

//	set parameters in ABP data packets produced by QtDSP
void
QtDSP::setABPParameters( char * ABPPacket, uint4 recordlen, uint4 dataformat )
{
	*(uint4 *)(ABPPacket + 64) = recordlen;		//	offset to recordlen: 32+28+4
	*(uint4 *)(ABPPacket + 84) = dataformat;	//	offset to dataformat: 32+28+24
}

void
QtDSP::connectUp()	//	connect processing signals to slots:
//	receive notifiers to data-receive slots
//	data-receive to ABP generators
{
	bool r_c;
	//	one SIGNAL/SLOT pair for each receive channel: 
	r_c = connect(
		rcvChannel[0]._DataSocketNotifier, SIGNAL(activated(int)), 
		this, SLOT(dataReceiveSocketActivatedSlot(int)));
	sprintf(m_statusLogBuf, "%d:connect() notifier 0 port %d r_c = %d", 
		rcvChannel[0]._DataSocketNotifier, 
		rcvChannel[0]._port, r_c); 
	statusLog->append( m_statusLogBuf ); 

	r_c = connect(
		rcvChannel[1]._DataSocketNotifier, SIGNAL(activated(int)), 
		this, SLOT(dataReceiveSocketActivatedSlot(int)));
	sprintf(m_statusLogBuf, "%d:connect() notifier 1 port %d r_c = %d", 
		rcvChannel[1]._DataSocketNotifier, 
		rcvChannel[1]._port, r_c); 
	statusLog->append( m_statusLogBuf ); 

	r_c = connect(
		rcvChannel[2]._DataSocketNotifier, SIGNAL(activated(int)), 
		this, SLOT(dataReceiveSocketActivatedSlot(int)));
	sprintf(m_statusLogBuf, "%d:connect() notifier 2 port %d r_c = %d", 
		rcvChannel[2]._DataSocketNotifier, 
		rcvChannel[2]._port, r_c); 
	statusLog->append( m_statusLogBuf ); 

}

void 
QtDSP::timerEvent(QTimerEvent *e) {	//	monitor receive-data functioning
	static int secs = 0;
	static uint8 PN; 

	_chan0PNerrs->setNum(rcvChannel[0].pnErrors);
	_chan1PNerrs->setNum(rcvChannel[1].pnErrors);
	_chan2PNerrs->setNum(rcvChannel[2].pnErrors);

	if	((!m_receiving) || (rcvChannel[0]._PN == 0))	//	not receiving data
		return;			//	no channels to monitor
	if	(PN == rcvChannel[0]._PN)	{	//	no data received on channel (0)
		secs++; 
	}
	else				//	PN moved: data received
		secs = 0;		//	restart interval measurement

	PN = rcvChannel[0]._PN;	//	maintain local copy most-recent received PN
	if	(secs >= 5)	{	//
		rcvChannel[0]._parametersSet = 0;
		statusLog->append( "Not Receiving" ); 
		sprintf(m_statusLogBuf, "_fifo hits remaining %d", rcvChannel[0]._fifo_hits); statusLog->append( m_statusLogBuf ); 
		_SABPgenBeginPN = 0;	//	tell SABPGen to re-sync to compute pulsepairs
		rcvChannel[0]._PN = 0;
		statusLog->append( "timerEvent 5 secs" ); 
	}
}


void
QtDSP::startStopReceiveSlot()	//	activated on either lostFocus or returnPressed
{
	if	(m_receiving)	{		//	currently receiving data
		for (int i = 0; i < QTDSP_RECEIVE_CHANNELS; i++) {	//	all receive channels
			terminateReceiveSocket(&rcvChannel[i]);
		}
		m_receiving = !m_receiving; 
		statusLog->append( "Not Receiving" ); 
		return;
	}
	//	toggle start/stop
	m_receiving = !m_receiving; 
	//	pick up user input
	m_destinationHostname = _destinationHostname->text(); 
	m_IQdestinationPortNumber = _IQdestinationPortBase->text(); 
	m_ABPdestinationPortNumber = _ABPdestinationPortBase->text(); 
	m_receivePortNumber = _receivePortBase->text(); 
	//	string contents do not appear in Debugger without this:
	std::cout << "->text() returned " << m_destinationHostname << std::endl;
	std::cout << "->text() returned " << m_IQdestinationPortNumber << std::endl;
	std::cout << "->text() returned " << m_ABPdestinationPortNumber << std::endl;
	std::cout << "->text() returned " << m_receivePortNumber << std::endl;

	sprintf(m_statusLogBuf, "startStopReceiveSlot(): m_destinationHostname %s", m_destinationHostname.ascii()); 
	statusLog->append( m_statusLogBuf );
	m_IQdestinationPort = atoi(m_IQdestinationPortNumber); 
	m_receivePort = atoi(m_receivePortNumber); 

	//	create the sockets that receive packets from CP2exec:   
	//	initialize the channels that receive packets from CP2exec:   
	for (int i = 0; i < QTDSP_RECEIVE_CHANNELS; i++)	{	//	MULTIPORT: all receive channels
		rcvChannel[i]._rcvChannel = i;	//	enum'd channel name
		rcvChannel[i]._port = m_receivePort + i;		//	define ports from base value 
		rcvChannel[i]._packetCount = rcvChannel[i]._packetCountErrors = 0; 
		rcvChannel[i]._sockBufSize = 1000000;	//	default buffer size; customize as necessary
		rcvChannel[i]._parametersSet = 0;		//	piraqx parameters initialized from received data (0 = not yet)
		rcvChannel[i]._PN = 0;		//	most-recent pulsenumber received
		rcvChannel[0]._secsSilent = 0;	//	seconds receive channel is silent
		//	piraqx parameters used by this channel:
		rcvChannel[i]._gates = 0;		//	gates, this channel
		rcvChannel[i]._hits = 0;		//	hits
		rcvChannel[i].pnErrors = 0;
		rcvChannel[i]._radarType = CP2RADARTYPES;	//	radar type end value: must be < this value
		//	operating parameters: 
		rcvChannel[i]._pulsesToProcess = 0;	
		rcvChannel[i].pnErrors = 0;
		rcvChannel[i].lastreadBufLen = -1;
		initializeReceiveSocket(&rcvChannel[i]); //	initialize receive socket
		_socketToChannelMap[rcvChannel[i]._DataSocket->socket()] = i;
	}
	//	send sockets for timeseries, ABP data: 
	for (int i = 0; i < QTDSP_SEND_CHANNELS; i++)	{	//	MULTIPORT: all send channels
		sendChannel[i]._sendChannel = i;	//	enum'd channel name
		sendChannel[i]._port = m_IQdestinationPort + i;		//	define ports from base value 
		sendChannel[i]._packetCount = sendChannel[i]._packetCountErrors = 0; 
		sendChannel[i]._sockBufSize = 1000000;	//	default buffer size; customize as necessary
		initializeSendSocket(&sendChannel[i]); //	initialize send socket
	}
	//	initialize counts, etc., used to compute APBs 
	//	!move inside rx struct, remove type-stuff from name: 
	_SABPgenBeginPN = 0;	//	1st PN in beam to compute pulsepairs

	connectUp(); 
	statusLog->append( "Receiving" ); 
}

void 
QtDSP::dataReceiveSocketActivatedSlot(int socket)		
{
	int rcvChan = _socketToChannelMap[socket];

	rcvChannel[rcvChan]._packetCount++; 

	int readBufLen = rcvChannel[rcvChan]._DataSocket->readBlock(
		(char *)rcvChannel[rcvChan]._SocketBuf, 
		sizeof(short)*100000);

	if	(rcvChannel[rcvChan].lastreadBufLen != readBufLen &&
		rcvChannel[rcvChan].lastreadBufLen != -1)
	{	
		//	packet length varies: log it
		sprintf(m_statusLogBuf, 
			"len error %d: rBL %d lrBL %d", 
			rcvChannel[rcvChan]._packetCount, 
			readBufLen, 
			rcvChannel[rcvChan].lastreadBufLen); 

		statusLog->append( m_statusLogBuf); 
	}
	else	{	//	get piraqx parameters
		if	(!rcvChannel[rcvChan]._parametersSet)	{	
			//	piraqx parameters need to be initialized from received data
			int r_c = getParameters( rcvChannel[rcvChan]._SocketBuf, rcvChan );	//	 
			if	(r_c == 1)	{		//	success
				rcvChannel[rcvChan]._parametersSet = 1; 
				sprintf(m_statusLogBuf, "_parametersSet"); 
				statusLog->append( m_statusLogBuf ); 
				sprintf(m_statusLogBuf, "_pulseStride  %d", _pulseStride); 
				statusLog->append( m_statusLogBuf ); 
				//	using received-data parameters, set FIFO record size: 
				//	set size of Fifo records from rx data, in floats -- CP2 datatype
				rcvChannel[rcvChan]._fifo->_chunkSize = _pulseStride;	
			}
		}
	}
	rcvChannel[rcvChan].lastreadBufLen = readBufLen;
	//	report packet count
	if (!(rcvChannel[rcvChan]._packetCount % 200))	{
		if(readBufLen > 0) {
			sprintf(m_statusLogBuf, 
				"rcvChan %d: pkts %d", 
				rcvChan, 
				rcvChannel[rcvChan]._packetCount); 
			statusLog->append( m_statusLogBuf );
		}
	}
	if	(rcvChannel[rcvChan]._parametersSet)	{	
		//	operating parameters set, test PNs sequential
		for	(int hit = 0; hit < _Nhits; hit++)	{	
			//	all hits in packet
			//	std::vector FIFO implementation: 
			float* _fifo1src = (float *)rcvChannel[rcvChan]._SocketBuf + (hit*_pulseStride); 
			//	push pulse
			rcvChannel[rcvChan]._fifo->push_back(_fifo1src);	 
			rcvChannel[rcvChan]._fifo_hits++;
			if	(rcvChannel[rcvChan]._pulsesToProcess > 3*rcvChannel[rcvChan]._hits)	{	
				//	some timeseries data has accumulated
				//	SVH timeseries data
				if	(rcvChannel[rcvChan]._radarType == SVH)	{	
					SABPgen(rcvChan);	
				}
				//	XV, XH here
			}
		}
		rcvChannel[rcvChan]._pulsesToProcess += _Nhits;	//	maintain 
	}
	//	send rcv'd data to destination port: !send from receive buffer, use its length: 
	sendChannel[rcvChan]._DataSocket->writeBlock((char *)rcvChannel[rcvChan]._SocketBuf, 
		readBufLen, sendChannel[rcvChan]._sendqHost, 
		sendChannel[rcvChan]._port);
}

//	compute S-band APBs over 1 beam 
//	global allocations: move inside ABPgen class: 
#if 1
//	allocate 4 record buffers for 4 sequential pulses
CP2_PIRAQ_DATA_TYPE * V1 = new CP2_PIRAQ_DATA_TYPE[10000];	 //	replace throughout w/CP2_DATA_TYPE
float * H1 = new CP2_PIRAQ_DATA_TYPE[10000];	 
float * V2 = new CP2_PIRAQ_DATA_TYPE[10000];	 
float * H2 = new CP2_PIRAQ_DATA_TYPE[10000];	
//	allocate storage for 1 beam
float * SABP = new CP2_PIRAQ_DATA_TYPE[65536]; 
//	pointers to pulse buffers, by function, for calculating ABPs: 
//	initialize ONCE; after this, lagging-leading designation of these pointers on entry depends on operating #hits
float * VlagPtr = V1; 
float * HlagPtr = H1;
float * VPtr = V2;
float * HPtr = H2; 
#endif

//	std::vector FIFO implementation 
void 
QtDSP::SABPgen(int rcvChan) {	//	generate S-band ABPs on rcvChan data
	uint8 SABPgenPN;
	//	pointers to I and Q in each record, for calculating ABPs: 
	float * VlagDataI, * HlagDataI, * VDataI, * HDataI;
	float * VlagDataQ, * HlagDataQ, * VDataQ, * HDataQ;
	//	pointer to SVH ABP data
	float * SABPData; 
	const void * PulseReadPtr;
	float	ABPscale = (float)pow(2,31)*(float)pow(2,31)*(rcvChannel[rcvChan]._hits/2);	//	hits/2 = #sums performed per beam. 
	//	temp send 
	int lastsendBufLen, SABPLen;
	int errcount = 0;	//	PNs out of sequence
	uint8	PN;
	static	uint8* PNptr;	//	32 + 28 + 32 = _PNOffset in piraqx
	float * buf;	//	pointer to 1 PACKET pop'd from front of FIFO
	int i, j;

	if	(!_SABPgenBeginPN)	{	//	first time: get a suitable PN to begin beam calculations (need BN-2 so two lags are correct)
		while(1)	{	//	until  PN = (BN * _hits) - 2 is found
			buf = &(rcvChannel[rcvChan]._fifo->fpfront()[0]);	//	refer to oldest FIFO entry
			PNptr = (uint8*)((char *)(&buf[0]) + _PNOffset);	//	compute PN offset in it
			PN = *PNptr; 
			if	((PN % rcvChannel[rcvChan]._hits) == (rcvChannel[rcvChan]._hits - 2))	{	//	found it: keep it in Vlag
				_SABPgenBeginPN = PN; 
				sprintf(m_statusLogBuf, "_SABPgenBeginPN = %I64d", _SABPgenBeginPN); statusLog->append( m_statusLogBuf );
				//	get Vlag
				memmove(VlagPtr, buf, _pulseStride*(sizeof(float)));	
				break;	
			}
			rcvChannel[rcvChan]._fifo->pop_fpfront();		//	drop the FIFO entry
			rcvChannel[rcvChan]._fifo_hits--;				//	maintain size
		}
		rcvChannel[rcvChan]._fifo->pop_fpfront();		//	drop Vlag FIFO entry
		rcvChannel[rcvChan]._fifo_hits--;	
		//	get Hlag
		buf = &(rcvChannel[rcvChan]._fifo->fpfront()[0]);	//	
		memmove(HlagPtr, buf, _pulseStride*(sizeof(float)));	
		PNptr = (uint8*)((char *)(&buf[0]) + _PNOffset);	//	compute PN offset in it
		PN = *PNptr; 
		rcvChannel[rcvChan]._fifo->pop_fpfront();		//	drop Hlag FIFO entry
		rcvChannel[rcvChan]._fifo_hits--;	
	}
	//	get pointer to 1st pulse of beam BN:
	buf = &(rcvChannel[rcvChan]._fifo->fpfront()[0]);	//	
	//	first initialize the beam w/header 
	memmove(SABP, buf, _pulseStride*(sizeof(float)));	
	SABPData = SABP + _IQdataOffset/sizeof(float); 
	for	(i = 0; i < SVHABP_STRIDE*rcvChannel[rcvChan]._gates; i++)	{	//	all data in beam packet
		*SABPData++ = 0.0;				//	clear it 
	}
	//	compute SABP packet length for send
	SABPLen = sizeof(PACKETHEADER) + SVHABP_STRIDE*(sizeof(CP2_PIRAQ_DATA_TYPE))*rcvChannel[rcvChan]._gates;
	//	compute the pulsepairs: 
	SABPgenPN = _SABPgenBeginPN; 
	SABPData = SABP + _IQdataOffset/sizeof(float); 

	for	(i = 0; i < rcvChannel[rcvChan]._hits/2; i++)	{	//	all hits in the beam taken 2 at a time
		//	get V: 
		buf = &(rcvChannel[rcvChan]._fifo->fpfront()[0]);	//	
		PNptr = (uint8*)((char *)(&buf[0]) + _PNOffset);	//	compute PN offset in it
		PN = *PNptr;	
		rcvChannel[rcvChan]._PN = PN;	//	retain most-recent PN received
		if	(rcvChannel[rcvChan].lastPN != PN - 1)	{	//	PNs not in sequence; ABP calculation not possible
			rcvChannel[rcvChan].pnErrors++;
			//	dump the beam altogether
		}
		rcvChannel[rcvChan].lastPN = PN;	
		memmove(VPtr, buf, _pulseStride*(sizeof(float)));
		VDataI = VPtr + _IQdataOffset/sizeof(float); VDataQ = VDataI + 1;	//	set pointers to data; _IQdataOffset in bytes
		rcvChannel[rcvChan]._fifo->pop_fpfront();		//	drop FIFO entry
		rcvChannel[rcvChan]._fifo_hits--;	
		//	get H:
		buf = &(rcvChannel[rcvChan]._fifo->fpfront()[0]);	//	
		PNptr = (uint8*)((char *)(&buf[0]) + _PNOffset);	//	compute PN offset in it
		PN = *PNptr; 
		rcvChannel[rcvChan]._PN = PN;	//	retain most-recent PN received
		if	(rcvChannel[rcvChan].lastPN != PN - 1)	{	//	PNs not in sequence; ABP calculation not possible
			rcvChannel[rcvChan].pnErrors++;
			//	dump the beam altogether
		}
		rcvChannel[rcvChan].lastPN = PN;	
		memmove(HPtr, buf, _pulseStride*(sizeof(float)));	
		HDataI = HPtr + _IQdataOffset/sizeof(float); HDataQ = HDataI + 1;	//	set pointers to data; _IQdataOffset in bytes
		rcvChannel[rcvChan]._fifo->pop_fpfront();		//	drop IFO entry
		rcvChannel[rcvChan]._fifo_hits--;	
		//	set Vlag, Hlag data pointers: 
		VlagDataI = VlagPtr + _IQdataOffset/sizeof(float); VlagDataQ = VlagDataI + 1;	//	point to V1 I,Q data; offset in bytes
		HlagDataI = HlagPtr + _IQdataOffset/sizeof(float); HlagDataQ = HlagDataI + 1;	//	offset in bytes
		for	(j = 0; j < rcvChannel[rcvChan]._gates; j++)	{	//	all gates, this pair of hits
			//	order calculations to produce same as SPol ABP set: 
			//	compute H-V AB plus Pv
			// 1.
			*SABPData += *HlagDataI * *VDataI + *HlagDataQ * *VDataQ; SABPData++; 
			//			*SABPData += *HlagDataI * *VDataQ - *HlagDataQ * *VDataI; SABPData++; 
			// 2.
			*SABPData += -(*HlagDataI * *VDataQ - *HlagDataQ * *VDataI); SABPData++;	//	sign change gets correct velocity...
			// 3.
			*SABPData += *VDataI * *VDataI + *VDataQ * *VDataQ; SABPData++; 
			//	compute V-H AB plus Ph
			// 4.
			*SABPData += *VDataI * *HDataI + *VDataQ * *HDataQ; SABPData++; 
			//			*SABPData += *VDataI * *HDataQ - *VDataQ * *HDataI; SABPData++; 
			// 5.
			*SABPData += -(*VDataI * *HDataQ - *VDataQ * *HDataI); SABPData++;	//	sign change gets correct velocity...
			// 6.
			*SABPData += *HDataI * *HDataI + *HDataQ * *HDataQ; SABPData++; 
			//	compute "ab of second lag"
			// 7.
			*SABPData += (*HlagDataI * *HDataI + *HlagDataQ * *HDataQ) + (*VlagDataI * *VDataI + *VlagDataQ * *VDataQ); SABPData++; 
			// 8.
			*SABPData += (*HlagDataI * *HDataQ - *HlagDataQ * *HDataI) + (*VlagDataI * *VDataQ - *VlagDataQ * *VDataI); SABPData++; 
			//	advance IQ data pointers by 1 gate
			VDataI += 2; VDataQ += 2; HDataI += 2; HDataQ += 2; VlagDataI += 2; VlagDataQ += 2; HlagDataI += 2; HlagDataQ += 2; 
		}	//	end for all gates
		SABPData = SABP + _IQdataOffset/sizeof(float);	//	start again at Gate 0
		//	rotate pointers to process next two hits: alternate buffer new hits stored to -- once leading, now lagging
		if	(VPtr == V1)	{	
			VPtr = V2; HPtr = H2; VlagPtr = V1; HlagPtr = H1; 
		}
		else	{	//
			VPtr = V1; HPtr = H1; VlagPtr = V2; HlagPtr = H2; 
		}
	}	//	end for all hits
	for	(j = 0; j < SVHABP_STRIDE*rcvChannel[rcvChan]._gates; j++)	{	//	all data, each gate 
		*SABPData++ /= ABPscale;		//	scale the data 
	}
	SABPData = SABP + _IQdataOffset/sizeof(float);	//	start again at Gate 0
	//	print data for gate 0: 
	if	((rcvChannel[rcvChan]._packetCount % 100) == 0) {	//	less than always -- test print beam header, data, etc.. 
		SABPData = SABP + (_IQdataOffset/sizeof(float) + 0);	//	compute pointer to a gate 
		sprintf(m_statusLogBuf, "ABPV-H0: %+8.3e %+8.3e %+8.3e %4.3f", *(SABPData+3), *(SABPData+4), *(SABPData+5), 10.0*log10(*(SABPData+5))); statusLog->append( m_statusLogBuf ); 
		sprintf(m_statusLogBuf, "ABH2V20: %+8.3e %+8.3e", *(SABPData+6), *(SABPData+7)); statusLog->append( m_statusLogBuf ); 
	}
	SABPData = SABP + _IQdataOffset/sizeof(float);	//	start again at Gate 0
	//	et voila! a beam!
	rcvChannel[rcvChan]._pulsesToProcess -= rcvChannel[rcvChan]._hits; 
	_SABPgenBeginPN =  SABPgenPN;		//	begin of next beam
	//	set record length, dataformat in ABP packet for consumers: 
	setABPParameters((char *)SABP, SABPLen, PIRAQ_CP2_SVHABP); 
	//	send ABP data to destination port -- numbered immediately after timeseries ports. send from SABP buffer, use its length: 
	sendChannel[DATA_CHANNELS + rcvChan]._DataSocket->writeBlock((char *)SABP, SABPLen, sendChannel[QTDSP_SEND_SBAND_ABP]._sendqHost, sendChannel[DATA_CHANNELS + rcvChan]._port);
	return;
}

//	compute X-band V APBs over 1 beam 
void 
QtDSP::XVABPgen(int rcvChan) {
	return;
}

//	compute X-band H APBs over 1 beam 
void 
QtDSP::XHABPgen(int rcvChan) {
	return;
}


void
QtDSP::initializeReceiveSocket(receiveChannel * rcvChannel)	{	//	receiveChannel struct w/pointers to new objects, etc. 
	int	rxBufSize = 0; 

	rcvChannel->_DataSocket = new QSocketDevice(QSocketDevice::Datagram);
	QHostAddress qHost;

	//	confirm winsock2 available: this works -- no exit
	WSADATA w;
	// Must be done at the beginning of every WinSock program
	int error = WSAStartup (0x0202, &w);   // Fill in w
	if (error) exit(0); 
	if (w.wVersion != 0x0202)  exit(0); 
	char nameBuf[1000];
	if (gethostname(nameBuf, sizeof(nameBuf))) {
		statusLog->append("gethostname failed");
		exit(1);
	}

	struct hostent* pHostEnt = gethostbyname(nameBuf);
	if (!pHostEnt) {
		statusLog->append("gethostbyname failed");
		exit(1);
	}
	rcvChannel->_SocketBuf = new CP2_PIRAQ_DATA_TYPE[1000000];

	std::string myIPname = nameBuf;
	std::string myIPaddress = inet_ntoa(*(struct in_addr*)pHostEnt->h_addr_list[0]);
	std::cout << "ip name: " << myIPname.c_str() << ", ip address  " << myIPaddress.c_str() << std::endl;
	sprintf(m_statusLogBuf, "ip name %s ip address %s", myIPname.c_str(), myIPaddress.c_str());
	statusLog->append( m_statusLogBuf );

	qHost.setAddress(myIPaddress.c_str());
	m_receivePacketCount = 0; 

	std::cout << "qHost:" << qHost.toString() << std::endl;
	std::cout << "opened datagram port:" << rcvChannel->_port << std::endl;
	if (!rcvChannel->_DataSocket->bind(qHost, rcvChannel->_port)) {
		sprintf(m_statusLogBuf, "Unable to bind to %s:%d", qHost.toString().ascii(), rcvChannel->_port);
		statusLog->append( m_statusLogBuf );
		exit(1); 
	}
	int sockbufsize = 20000000;

	int result = setsockopt (rcvChannel->_DataSocket->socket(),
		SOL_SOCKET,
		SO_RCVBUF,
		(char *) &sockbufsize,
		sizeof sockbufsize);
	rcvChannel->_DataSocket->setReceiveBufferSize (sockbufsize); 
	if (result) {
		statusLog->append("Set receive buffer size for socket failed");
		exit(1); 
	}
	result = rcvChannel->_DataSocket->receiveBufferSize(); 
	sprintf(m_statusLogBuf, "rx buffer size %d", result); 
	statusLog->append( m_statusLogBuf );

	rcvChannel->_DataSocketNotifier = 
		new QSocketNotifier(rcvChannel->_DataSocket->socket(), QSocketNotifier::Read);
	sprintf(m_statusLogBuf, "init rec port %d socket %d", 
		rcvChannel->_port, 
		rcvChannel->_DataSocket->socket()); 
	statusLog->append( m_statusLogBuf );

	rxBufSize = rcvChannel->_DataSocket->receiveBufferSize(); 
	sprintf(m_statusLogBuf, "port %d rxBufSize %d", 
		rcvChannel->_port, rxBufSize); 
	statusLog->append( m_statusLogBuf );

}

void
QtDSP::initializeSendSocket(transmitChannel * sendChannel)	
{	//	 
	sendChannel->_DataSocket = new QSocketDevice(QSocketDevice::Datagram);

	char nameBuf[1000];
	strcpy(nameBuf,m_destinationHostname.ascii());
	if (gethostname(nameBuf, sizeof(nameBuf))) {
		statusLog->append("gethostname failed");
		//exit(1);
	}

	struct hostent* pHostEnt = gethostbyname(nameBuf);
	if (!pHostEnt) {
		statusLog->append("gethostbyname failed");
		//exit(1);
	}
	sprintf(m_statusLogBuf, 
		"Destination Hostname %s", 
		m_destinationHostname.ascii());
	statusLog->append( m_statusLogBuf );

	sendChannel->_SocketBuf = new CP2_PIRAQ_DATA_TYPE[1000000];	
	std::string destIPname = m_destinationHostname.ascii();
	std::string destIPaddress = inet_ntoa(*(struct in_addr*)pHostEnt->h_addr_list[0]);
	std::cout << "ip name: " << destIPname.c_str() 
		<< ", ip address  " 
		<< destIPaddress.c_str() 
		<< std::endl;

	_sendqHost.setAddress(destIPaddress.c_str()); // works
	sendChannel[0]._sendqHost.setAddress(destIPaddress.c_str()); //?
	std::cout << "opened datagram port:" 
		<< sendChannel->_port << std::endl;
	int sockbufsize = 1000000;

	int result = setsockopt (sendChannel->_DataSocket->socket(),
		SOL_SOCKET,
		SO_SNDBUF,
		(char *) &sockbufsize,
		sizeof sockbufsize);

	if (result) {
		statusLog->append("Set send buffer size for socket failed");
		exit(1); 
	}
}

void 
QtDSP::terminateReceiveSocket( receiveChannel * rcvChannel )	{	//	
	if (rcvChannel->_DataSocketNotifier)
		delete rcvChannel->_DataSocketNotifier;

	if (rcvChannel->_DataSocket)
		delete rcvChannel->_DataSocket;

	if (rcvChannel->_SocketBuf)
		delete [] rcvChannel->_SocketBuf;

	sprintf(m_statusLogBuf, "terminating rcv %d rec'd %d packets", rcvChannel->_rcvChannel, rcvChannel->_packetCount);	//	report final packet count
	statusLog->append(m_statusLogBuf);
}
