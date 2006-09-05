//
#include "qtdsp.h"
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qspinbox.h>
#include <qlineedit.h>

#include <iostream>

#include "..\CP2Classes\fifo.h"	//	!include path

QtDSP::QtDSP():
m_pDataSocket(0),    
m_pDataSocketNotifier(0),
m_pSocketBuf(0)
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
	FIFO1 = new FIFO ();	//	channel 1, 2, 3
	FIFO2 = new FIFO ();	
	FIFO3 = new FIFO ();	
	sprintf(m_statusLogBuf, "FIFOs created for data channels 1, 2, 3: size %d bytes", RECORDSIZE_MAX * RECORDNUM_MAX ); 

}

QtDSP::~QtDSP() {

	if	(m_receiving)	{	//	currently receiving on input: generalize
		for (int i = 0; i < QTDSP_RECEIVE_CHANNELS; i++) {	//	all receive channels
		    terminateReceiveSocket(&rcvChannel[i]);
		}
	}
	delete FIFO1; delete FIFO2; delete FIFO3; 
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
	r_c = connect(rcvChannel[0]._DataSocketNotifier, SIGNAL(activated(int)), this, SLOT(dataReceiveSocketActivatedSlot0(int)));
	sprintf(m_statusLogBuf, "%d:connect() notifier 0 port %d r_c = %d", rcvChannel[0]._DataSocketNotifier, rcvChannel[0]._port, r_c); statusLog->append( m_statusLogBuf ); 
	r_c = connect(rcvChannel[1]._DataSocketNotifier, SIGNAL(activated(int)), this, SLOT(dataReceiveSocketActivatedSlot1(int)));
	sprintf(m_statusLogBuf, "%d:connect() notifier 1 port %d r_c = %d", rcvChannel[1]._DataSocketNotifier, rcvChannel[1]._port, r_c); statusLog->append( m_statusLogBuf ); 
	r_c = connect(rcvChannel[2]._DataSocketNotifier, SIGNAL(activated(int)), this, SLOT(dataReceiveSocketActivatedSlot2(int)));
	sprintf(m_statusLogBuf, "%d:connect() notifier 2 port %d r_c = %d", rcvChannel[2]._DataSocketNotifier, rcvChannel[2]._port, r_c); statusLog->append( m_statusLogBuf ); 

}

#if 1
void 
QtDSP::timerEvent(QTimerEvent *e) {	//	monitor receive-data functioning
	static int secs = 0;
	static uint8 PN; 

return;	
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
//clear active FIFOs		SFIFO->clearData();		//	start over with data collection 
//_SpulsesToProcess = 0;	//	clear hits-to-process totals 
_SABPgenBeginPN = 0;	//	1st PN in beam to compute pulsepairs
rcvChannel[0]._PN = 0;
secs = 0;
//		statusLog->append( "SFIFO->clearData()" ); 
	}
}

#else
void 
QtDSP::timerEvent(QTimerEvent *e) {	//	monitor receive-data functioning
//	static int secs = 0;
	static uint8 PN; 

	if	((!m_receiving) || (rcvChannel[0]._PN == 0))	//	QtDSP not receiving data, or channel silent 
		return;			//	no channels to monitor
	if	(PN == rcvChannel[0]._PN)	{	//	no data received on channel (0)
		rcvChannel[0]._secsSilent++; 
	}
	else				//	PN moved: data received
		rcvChannel[0]._secsSilent = 0;		//	restart interval measurement

	PN = rcvChannel[0]._PN;	//	maintain local copy most-recent received PN
	if	(rcvChannel[0]._secsSilent >= 5)	{	//
		rcvChannel[0]._parametersSet = 0;
		statusLog->append( "Not Receiving" ); 
//		SFIFO->clearData();		//	start over with data collection 
//_SpulsesToProcess = 0;	//	clear hits-to-process totals 
_SABPgenBeginPN = 0;	//	1st PN in beam to compute pulsepairs
rcvChannel[0]._PN = 0;
//		statusLog->append( "SFIFO->clearData()" ); 
	}
}
#endif

void
QtDSP::startStopReceiveSlot()	//	activated on either lostFocus or returnPressed
{
	if	(m_receiving)	{		//	currently receiving data
		for (int i = 0; i < QTDSP_RECEIVE_CHANNELS; i++) {	//	all receive channels
		    terminateReceiveSocket(&rcvChannel[i]);
		}
		m_receiving = !m_receiving; 
		statusLog->append( "Not Receiving" ); 
		FIFO1->clearData();		//	start over with data collection 
		statusLog->append( "FIFO1->clearData()" ); 
		FIFO2->clearData();		 
		statusLog->append( "FIFO2->clearData()" ); 
		FIFO3->clearData();		 
		statusLog->append( "FIFO3->clearData()" ); 
		//	add 2, 3
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
	//does not work:	m_destinationPort = atoi(_destinationPort->text()); 
	m_IQdestinationPort = atoi(m_IQdestinationPortNumber); 
	m_receivePort = atoi(m_receivePortNumber); 

	//	create the sockets that receive packets from CP2exec:   
	//	initialize the channels that receive packets from CP2exec:   
	for (int i = 0; i < QTDSP_RECEIVE_CHANNELS; i++)	{	//	MULTIPORT: all receive channels
		rcvChannel[i]._rcvChannel = i;	//	enum'd channel name
		rcvChannel[i]._port = m_receivePort + i;		//	define ports from base value 
		rcvChannel[i]._packetCount = rcvChannel[i]._packetCountErrors = 0; 
		rcvChannel[i]._sockBufSize = 1000000;	//	default buffer size; customize as necessary
		rcvChannel[i]._parametersSet = 0;		//	piraqx parameters not yet initialized from received data
		rcvChannel[i]._PN = 0;		//	most-recent pulsenumber received
		rcvChannel[0]._secsSilent = 0;	//	seconds receive channel is silent
		//	piraqx parameters used by this channel:
		rcvChannel[i]._gates = 0;		//	gates, this channel
		rcvChannel[i]._hits = 0;		//	hits
		rcvChannel[i]._radarType = CP2RADARTYPES;	//	radar type end value: must be < this value
		//	operating parameters: 
		rcvChannel[i]._pulsesToProcess = 0;	
		initializeReceiveSocket(&rcvChannel[i]); //	initialize receive socket
	}
	//	initialize pointers to receive-data FIFOs
	rcvChannel[0].rxFIFO = FIFO1; rcvChannel[1].rxFIFO = FIFO2; rcvChannel[2].rxFIFO = FIFO3;
	//	send sockets for timeseries, ABP data: 
	for (int i = 0; i < QTDSP_SEND_CHANNELS; i++)	{	//	MULTIPORT: all send channels
		sendChannel[i]._sendChannel = i;	//	enum'd channel name
		sendChannel[i]._port = m_IQdestinationPort + i;		//	define ports from base value 
		sendChannel[i]._packetCount = sendChannel[i]._packetCountErrors = 0; 
		sendChannel[i]._sockBufSize = 1000000;	//	default buffer size; customize as necessary
		initializeSendSocket(&sendChannel[i]); //	initialize send socket
	}
	//	initialize counts, etc., used to compute APBs 
	//	!move inside rx struct, remove type-stuff from names: 
	_SABPgenBeginPN = 0;	//	1st PN in beam to compute pulsepairs

	connectUp(); 
	statusLog->append( "Receiving" ); 
}

void 
QtDSP::dataReceiveSocketActivatedSlot0(int socket)	//	socket not used ... pass ? for MULTIPORT implementation
{
	static	int errcount = 0;
	static	int readBufLen, lastreadBufLen;
	static	int rcvChan = 0;	//	receive channel 
	//	temp send 
	static	int sendBufLen, lastsendBufLen;

	//	!test PNs sequential: from here ...
	int hit = 0;		//	index into N-hit packet
	int	gates;
	int	r_c;	//	return_code
	//	define p: pointer to packet data 
	float* p = ((float*)(rcvChannel[rcvChan]._SocketBuf)) + _pulseStride*hit + (sizeof(PACKETHEADER)/sizeof(float));	//	
	uint8* PNptr = (uint8*)((char *)(rcvChannel[rcvChan]._SocketBuf) + 32 + 28 + 32);	//	32 + 28 + 32 = _PNOffset in piraqx
	static	uint8	PN, lastPN;
//	... to here
	rcvChannel[rcvChan]._packetCount++; 

	readBufLen = rcvChannel[rcvChan]._DataSocket->readBlock((char *)rcvChannel[rcvChan]._SocketBuf, sizeof(short)*100000);
	if	(lastreadBufLen != readBufLen)	{	//	packet length varies: log it
		sprintf(m_statusLogBuf, "len error %d: rBL %d lrBL %d", rcvChannel[rcvChan]._packetCount, readBufLen, lastreadBufLen); statusLog->append( m_statusLogBuf ); 
	}
	else	{	//	get piraqx parameters
		if	(!rcvChannel[rcvChan]._parametersSet)	{	//	piraqx parameters need to be initialized from received data
			r_c = getParameters( rcvChannel[rcvChan]._SocketBuf, rcvChan );	//	 
			if	(r_c == 1)	{		//	success
				rcvChannel[rcvChan]._parametersSet = 1; 
				sprintf(m_statusLogBuf, "_parametersSet"); statusLog->append( m_statusLogBuf ); 
				sprintf(m_statusLogBuf, "_pulseStride  %d", _pulseStride); statusLog->append( m_statusLogBuf ); 
				//	set FIFO1 parameters: 
				rcvChannel[rcvChan].rxFIFO->set_recordNum( 1000 );	//	FIFO number of records 
				rcvChannel[rcvChan].rxFIFO->set_sequenceOffset( _PNOffset );	//	FIFO record offset to sequence number (here PN)in bytes
			}
		}
	}

	lastreadBufLen = readBufLen;
#if 0	//	report packet count
	if (!(rcvChannel[rcvChan]._packetCount % 100))	{
		if(readBufLen > 0) {
			sprintf(m_statusLogBuf, "socket %d: pkts %d", socket, rcvChannel[rcvChan]._packetCount); statusLog->append( m_statusLogBuf );
		}
	}
#endif
	//	test PNs sequential; log if not:
	//	p is set to 1st pulse in packet
	const void * testReqSNReadPtr;
	if	(rcvChannel[rcvChan]._parametersSet)	{	//	operating parameters set, test PNs sequential
		for	(hit = 0; hit < _Nhits; hit++)	{	//	all hits in packet
			PN	= *PNptr;
			//	maintain p, detect full set, break
			p += _pulseStride; 
			if	(PN != lastPN + 1)	{	//	we'll always get 1 at begin
				errcount++;	
				sprintf(m_statusLogBuf, "hit%d: lastPN = %I64d PN = %I64d errors = %d", hit, lastPN, PN, errcount); statusLog->append( m_statusLogBuf );
			}	
			//	move pulse data to FIFO. _pulseStride = length of move in floats ... rebase this to length in chars
			rcvChannel[rcvChan].rxFIFO->writeRecord( _pulseStride*sizeof(float), (char *)rcvChannel[rcvChan]._SocketBuf + (hit*_pulseStride*sizeof(float))); 
			lastPN = PN; 
			rcvChannel[rcvChan]._PN = lastPN;	//	make most-recent PN visible
			PNptr = (uint8 *)((char *)PNptr + _pulseStride*sizeof(float)); 
			p = ((float*)(rcvChannel[rcvChan]._SocketBuf)) + _pulseStride*hit + (sizeof(PACKETHEADER)/sizeof(float));	//	point to data
			if	(rcvChannel[rcvChan]._pulsesToProcess > 3*rcvChannel[rcvChan]._hits)	{	//	some timeseries data has accumulated
				if	(rcvChannel[rcvChan]._radarType == SVH)	//	SVH timeseries data
					SABPgen(rcvChan); 
				//	XV, XH here
			}
		}

		rcvChannel[rcvChan]._pulsesToProcess += _Nhits;	//	maintain 
	}
	//	send rcv'd data to destination port: !send from receive buffer, use its length: 
	sendBufLen = sendChannel[rcvChan]._DataSocket->writeBlock((char *)rcvChannel[rcvChan]._SocketBuf, readBufLen, sendChannel[rcvChan]._sendqHost, sendChannel[rcvChan]._port);
}

void 
QtDSP::dataReceiveSocketActivatedSlot1(int socket)	//	socket not used ... pass ? for MULTIPORT implementation
{
	static	int errcount = 0;
	static	int readBufLen, lastreadBufLen;
	static	int rcvChan = 1;	//	receive channel 
	//	temp send 
	static	int sendBufLen, lastsendBufLen;

	//	!test PNs sequential: from here ...
	int hit = 0;		//	index into N-hit packet
	int	gates;
	int	r_c;	//	return_code
	//	define p: pointer to packet data 
	float* p = ((float*)(rcvChannel[rcvChan]._SocketBuf)) + _pulseStride*hit + (sizeof(PACKETHEADER)/sizeof(float));	//	
	uint8* PNptr = (uint8*)((char *)(rcvChannel[rcvChan]._SocketBuf) + 32 + 28 + 32);	//	32 + 28 + 32 = _PNOffset in piraqx
	static	uint8	PN, lastPN;
//	... to here
	rcvChannel[rcvChan]._packetCount++; 

	readBufLen = rcvChannel[rcvChan]._DataSocket->readBlock((char *)rcvChannel[rcvChan]._SocketBuf, sizeof(short)*100000);
	if	(lastreadBufLen != readBufLen)	{	//	packet length varies: log it
		sprintf(m_statusLogBuf, "len error %d: rBL %d lrBL %d", rcvChannel[rcvChan]._packetCount, readBufLen, lastreadBufLen); statusLog->append( m_statusLogBuf ); 
	}
	else	{	//	get piraqx parameters
		if	(!rcvChannel[rcvChan]._parametersSet)	{	//	piraqx parameters need to be initialized from received data
			r_c = getParameters( rcvChannel[rcvChan]._SocketBuf, rcvChan );	//	 
			if	(r_c == 1)	{		//	success
				rcvChannel[rcvChan]._parametersSet = 1; 
				sprintf(m_statusLogBuf, "_parametersSet"); statusLog->append( m_statusLogBuf ); 
				sprintf(m_statusLogBuf, "_pulseStride  %d", _pulseStride); statusLog->append( m_statusLogBuf ); 
				//	set FIFO2 parameters: 
				rcvChannel[rcvChan].rxFIFO->set_recordNum( 1000 );	//	FIFO number of records 
				rcvChannel[rcvChan].rxFIFO->set_sequenceOffset( _PNOffset );	//	FIFO record offset to sequence number (here PN)in bytes
			}
		}
	}

	lastreadBufLen = readBufLen;
#if 1
	if (!(rcvChannel[rcvChan]._packetCount % 100))	{
		if(readBufLen > 0) {
			sprintf(m_statusLogBuf, "socket %d: rBL %d lrBL %d pkts %d", socket, readBufLen, lastreadBufLen, rcvChannel[rcvChan]._packetCount); statusLog->append( m_statusLogBuf );
		}
	}
#endif
	//	test PNs sequential; log if not:
	//	p is set to 1st pulse in packet
	const void * testReqSNReadPtr;
	if	(rcvChannel[rcvChan]._parametersSet)	{	//	operating parameters set, test PNs sequential
		for	(hit = 0; hit < _Nhits; hit++)	{	//	all hits in packet
			PN	= *PNptr;
			//	maintain p, detect full set, break
			p += _pulseStride; 
			if	(PN != lastPN + 1)	{	//	we'll always get 1 at begin
				errcount++;	
				sprintf(m_statusLogBuf, "hit%d: lastPN = %I64d PN = %I64d errors = %d", hit, lastPN, PN, errcount); statusLog->append( m_statusLogBuf );
			}	
			//	move pulse data to FIFO. _pulseStride = length of move in floats ... rebase this to length in chars
			rcvChannel[rcvChan].rxFIFO->writeRecord( _pulseStride*sizeof(float), (char *)rcvChannel[rcvChan]._SocketBuf + (hit*_pulseStride*sizeof(float))); 
			lastPN = PN; 
			rcvChannel[rcvChan]._PN = lastPN;	//	make most-recent PN visible
			PNptr = (uint8 *)((char *)PNptr + _pulseStride*sizeof(float)); 
			p = ((float*)(rcvChannel[rcvChan]._SocketBuf)) + _pulseStride*hit + (sizeof(PACKETHEADER)/sizeof(float));	//	point to data
			if	(rcvChannel[rcvChan]._pulsesToProcess > 3*rcvChannel[rcvChan]._hits)	{	//	some timeseries data has accumulated
				if	(rcvChannel[rcvChan]._radarType == SVH)	//	SVH timeseries data
					SABPgen(rcvChan); 
				//	XV, XH here
			}
		}

		rcvChannel[rcvChan]._pulsesToProcess += _Nhits;	//	maintain 
	}
	//	send rcv'd data to destination port: !send from receive buffer, use its length: 
	sendBufLen = sendChannel[rcvChan]._DataSocket->writeBlock((char *)rcvChannel[rcvChan]._SocketBuf, readBufLen, sendChannel[rcvChan]._sendqHost, sendChannel[rcvChan]._port);
}

void 
QtDSP::dataReceiveSocketActivatedSlot2(int socket)	//	socket not used ... pass ? for MULTIPORT implementation
{
	static	int errcount = 0;
	static	int readBufLen, lastreadBufLen;
	static	int rcvChan = 2;	//	receive channel 
	//	temp send 
	static	int sendBufLen, lastsendBufLen;

	//	!test PNs sequential: from here ...
	int hit = 0;		//	index into N-hit packet
	int	gates;
	int	r_c;	//	return_code
	//	define p: pointer to packet data 
	float* p = ((float*)(rcvChannel[rcvChan]._SocketBuf)) + _pulseStride*hit + (sizeof(PACKETHEADER)/sizeof(float));	//	
	uint8* PNptr = (uint8*)((char *)(rcvChannel[rcvChan]._SocketBuf) + 32 + 28 + 32);	//	32 + 28 + 32 = _PNOffset in piraqx
	static	uint8	PN, lastPN;
//	... to here
	rcvChannel[rcvChan]._packetCount++; 

	readBufLen = rcvChannel[rcvChan]._DataSocket->readBlock((char *)rcvChannel[rcvChan]._SocketBuf, sizeof(short)*100000);
	if	(lastreadBufLen != readBufLen)	{	//	packet length varies: log it
		sprintf(m_statusLogBuf, "len error %d: rBL %d lrBL %d", rcvChannel[rcvChan]._packetCount, readBufLen, lastreadBufLen); statusLog->append( m_statusLogBuf ); 
	}
	else	{	//	get piraqx parameters
		if	(!rcvChannel[rcvChan]._parametersSet)	{	//	piraqx parameters need to be initialized from received data
			r_c = getParameters( rcvChannel[rcvChan]._SocketBuf, rcvChan );	//	 
			if	(r_c == 1)	{		//	success
				rcvChannel[rcvChan]._parametersSet = 1; 
				sprintf(m_statusLogBuf, "_parametersSet"); statusLog->append( m_statusLogBuf ); 
				sprintf(m_statusLogBuf, "_pulseStride  %d", _pulseStride); statusLog->append( m_statusLogBuf ); 
				//	set FIFO3 parameters: 
				rcvChannel[rcvChan].rxFIFO->set_recordNum( 1000 );	//	FIFO number of records 
				rcvChannel[rcvChan].rxFIFO->set_sequenceOffset( _PNOffset );	//	FIFO record offset to sequence number (here PN)in bytes
			}
		}
	}

	lastreadBufLen = readBufLen;
#if 0	//	print packet count
	if (!(rcvChannel[rcvChan]._packetCount % 100))	{
		if(readBufLen > 0) {
			sprintf(m_statusLogBuf, "socket %d: pkts %d", socket, rcvChannel[rcvChan]._packetCount); statusLog->append( m_statusLogBuf );
		}
	}
#endif
	//	test PNs sequential; log if not:
	//	p is set to 1st pulse in packet
	const void * testReqSNReadPtr;
	if	(rcvChannel[rcvChan]._parametersSet)	{	//	operating parameters set, test PNs sequential
		for	(hit = 0; hit < _Nhits; hit++)	{	//	all hits in packet
			PN	= *PNptr;
			//	maintain p, detect full set, break
			p += _pulseStride; 
			if	(PN != lastPN + 1)	{	//	we'll always get 1 at begin
				errcount++;	
				sprintf(m_statusLogBuf, "hit%d: lastPN = %I64d PN = %I64d errors = %d", hit, lastPN, PN, errcount); statusLog->append( m_statusLogBuf );
			}	
			//	move pulse data to FIFO. _pulseStride = length of move in floats ... rebase this to length in chars
			rcvChannel[rcvChan].rxFIFO->writeRecord( _pulseStride*sizeof(float), (char *)rcvChannel[rcvChan]._SocketBuf + (hit*_pulseStride*sizeof(float))); 
			lastPN = PN; 
			rcvChannel[rcvChan]._PN = lastPN;	//	make most-recent PN visible
			PNptr = (uint8 *)((char *)PNptr + _pulseStride*sizeof(float)); 
			p = ((float*)(rcvChannel[rcvChan]._SocketBuf)) + _pulseStride*hit + (sizeof(PACKETHEADER)/sizeof(float));	//	point to data
			if	(rcvChannel[rcvChan]._pulsesToProcess > 3*rcvChannel[rcvChan]._hits)	{	//	some timeseries data has accumulated
				if	(rcvChannel[rcvChan]._radarType == SVH)	//	SVH timeseries data
					SABPgen(rcvChan); 
				//	XV, XH here
			}
		}

		rcvChannel[rcvChan]._pulsesToProcess += _Nhits;	//	maintain 
	}
	//	send rcv'd data to destination port: !send from receive buffer, use its length: 
	sendBufLen = sendChannel[rcvChan]._DataSocket->writeBlock((char *)rcvChannel[rcvChan]._SocketBuf, readBufLen, sendChannel[rcvChan]._sendqHost, sendChannel[rcvChan]._port);
}

//	compute S-band APBs over 1 beam 
//	global allocations: move inside QtDSP or ABPgen: 
#if 1
//	allocate 4 record buffers for 4 sequential pulses
	CP2_PIRAQ_DATA_TYPE * V1 = new CP2_PIRAQ_DATA_TYPE[10000];	 //	replace throughout w/CP2_DATA_TYPE
	float * H1 = new CP2_PIRAQ_DATA_TYPE[10000];	 
	float * V2 = new CP2_PIRAQ_DATA_TYPE[10000];	 
	float * H2 = new CP2_PIRAQ_DATA_TYPE[10000];	
	//	allocate storage for 1 beam
	float * SABP = new CP2_PIRAQ_DATA_TYPE[65536]; 
#endif
#if 1
	//	pointers to pulse buffers, by function, for calculating ABPs: 
	//	initialize ONCE; after this, lagging-leading designation of these pointers on entry depends on operating #hits
	float * VlagPtr = V1; 
	float * HlagPtr = H1;
	float * VPtr = V2;
	float * HPtr = H2; 
#endif

void 
QtDSP::SABPgen(int rcvChan) {	//	generate S-band ABPs on rcvChan data
	uint8 SABPgenPN;
	//	pointers to pulse buffers, by function, for calculating ABPs: 
//!	float * VlagPtr, * HlagPtr, * VPtr, * HPtr; 
	//	pointers to I and Q in each record, for calculating ABPs: 
	float * VlagDataI, * HlagDataI, * VDataI, * HDataI;
	float * VlagDataQ, * HlagDataQ, * VDataQ, * HDataQ;
	//	pointer to SVH ABP data
	float * SABPData; 
	const void * PulseReadPtr;
	float	ABPscale = (float)pow(2,31)*(float)pow(2,31)*(rcvChannel[rcvChan]._hits/2);	//	hits/2 = #sums performed per beam. 
	//	temp send 
	static	int sendBufLen, lastsendBufLen, SABPLen;

	int i, j;

	if	(!_SABPgenBeginPN)	{	//	first time: get a suitable PN to begin beam calculations
		_SABPgenBeginPN = rcvChannel[rcvChan].rxFIFO->leastSequenceNumber();
		sprintf(m_statusLogBuf, "leastSequenceNumber = %I64d", _SABPgenBeginPN); statusLog->append( m_statusLogBuf );
		_SABPgenBeginPN -= (_SABPgenBeginPN % rcvChannel[rcvChan]._hits); 
		_SABPgenBeginPN +=  2*rcvChannel[rcvChan]._hits; 
		sprintf(m_statusLogBuf, "_SABPgenBeginPN = %I64d", _SABPgenBeginPN); statusLog->append( m_statusLogBuf );
	}
	//	have suitable start Pn; set record pointers: 
//!	VlagPtr = V1; HlagPtr = H1; VPtr = V2; HPtr = H2; 
	//	get first 2 pulses: BeginPN-2, -1: "lags"
	//	get Vlag:
	PulseReadPtr = rcvChannel[rcvChan].rxFIFO->getRecordPtr(_SABPgenBeginPN - 2);
	if	(PulseReadPtr == NULL)	{	//	FIFO did not find requested PN
		sprintf(m_statusLogBuf, "FIFO: NULL PulseReadPtr"); statusLog->append( m_statusLogBuf ); 
		//	dump the beam altogether: 
	}
	memmove(VlagPtr, PulseReadPtr, _pulseStride*(sizeof(float)));	//	getParameters(): define a record size

	//	get Hlag:
	PulseReadPtr = rcvChannel[rcvChan].rxFIFO->getRecordPtr(_SABPgenBeginPN - 1);
	if	(PulseReadPtr == NULL)	{	
		sprintf(m_statusLogBuf, "FIFO: NULL PulseReadPtr"); statusLog->append( m_statusLogBuf ); 
		//	dump the beam altogether
	}
	memmove(HlagPtr, PulseReadPtr, _pulseStride*(sizeof(float)));	

	//	initialize the beam w/header from 1st pulse of beam: 
	memmove(SABP, PulseReadPtr,sizeof(PACKETHEADER)); 
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
		PulseReadPtr = rcvChannel[rcvChan].rxFIFO->getRecordPtr(SABPgenPN); SABPgenPN++; 
		if	(PulseReadPtr == NULL)	{	
			sprintf(m_statusLogBuf, "FIFO: NULL PulseReadPtr"); statusLog->append( m_statusLogBuf ); 
			//	dump the beam altogether
		}
		memmove(VPtr, PulseReadPtr, _pulseStride*(sizeof(float)));
		VDataI = VPtr + _IQdataOffset/sizeof(float); VDataQ = VDataI + 1;	//	set pointers to data; _IQdataOffset in bytes
		//	get H:
		PulseReadPtr = rcvChannel[rcvChan].rxFIFO->getRecordPtr(SABPgenPN); SABPgenPN++;  
		if	(PulseReadPtr == NULL)	{	
			sprintf(m_statusLogBuf, "FIFO: NULL PulseReadPtr"); statusLog->append( m_statusLogBuf ); 
			//	dump the beam altogether
		}
		memmove(HPtr, PulseReadPtr, _pulseStride*(sizeof(float)));	
		HDataI = HPtr + _IQdataOffset/sizeof(float); HDataQ = HDataI + 1;	//	set pointers to data; _IQdataOffset in bytes
		//	set Vlag, Hlag data pointers: 
		VlagDataI = VlagPtr + _IQdataOffset/sizeof(float); VlagDataQ = VlagDataI + 1;	//	point to V1 I,Q data; offset in bytes
		HlagDataI = HlagPtr + _IQdataOffset/sizeof(float); HlagDataQ = HlagDataI + 1;	//	offset in bytes
		for	(j = 0; j < rcvChannel[rcvChan]._gates; j++)	{	//	all gates, this pair of hits
			//	order calculations to produce same as SPol ABP set: 
			//	compute H-V AB plus Pv
			*SABPData += *HlagDataI * *VDataI + *HlagDataQ * *VDataQ; SABPData++; 
//			*SABPData += *HlagDataI * *VDataQ - *HlagDataQ * *VDataI; SABPData++; 
			*SABPData += -(*HlagDataI * *VDataQ - *HlagDataQ * *VDataI); SABPData++;	//	sign change gets correct velocity...
			*SABPData += *VDataI * *VDataI + *VDataQ * *VDataQ; SABPData++; 
			//	compute V-H AB plus Ph
			*SABPData += *VDataI * *HDataI + *VDataQ * *HDataQ; SABPData++; 
//			*SABPData += *VDataI * *HDataQ - *VDataQ * *HDataI; SABPData++; 
			*SABPData += -(*VDataI * *HDataQ - *VDataQ * *HDataI); SABPData++;	//	sign change gets correct velocity...
			*SABPData += *HDataI * *HDataI + *HDataQ * *HDataQ; SABPData++; 
			//	compute "ab of second lag"
			*SABPData += (*HlagDataI * *HDataI + *HlagDataQ * *HDataQ) + (*VlagDataI * *VDataI + *VlagDataQ * *VDataQ); SABPData++; 
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
#if 1
	//	print data for gate 0: 
	if	((_SABPgenBeginPN % 400) == 0) {	//	less than always -- test print beam header, data, etc.. 
		SABPData = SABP + (_IQdataOffset/sizeof(float) + 0);	//	compute pointer to a gate 
//		sprintf(m_statusLogBuf, "ABPH-V0: %+8.3e %+8.3e %+8.3e %4.3f", *(SABPData+0), *(SABPData+1), *(SABPData+2), 10.0*log10(*(SABPData+2))); statusLog->append( m_statusLogBuf ); 
		sprintf(m_statusLogBuf, "ABPV-H0: %+8.3e %+8.3e %+8.3e %4.3f", *(SABPData+3), *(SABPData+4), *(SABPData+5), 10.0*log10(*(SABPData+5))); statusLog->append( m_statusLogBuf ); 
		sprintf(m_statusLogBuf, "ABH2V20: %+8.3e %+8.3e", *(SABPData+6), *(SABPData+7)); statusLog->append( m_statusLogBuf ); 
	}
#endif	
	SABPData = SABP + _IQdataOffset/sizeof(float);	//	start again at Gate 0
	//	et voila! a beam!
	rcvChannel[rcvChan]._pulsesToProcess -= rcvChannel[rcvChan]._hits; 
	_SABPgenBeginPN =  SABPgenPN;		//	begin of next beam
	//	set record length, dataformat in ABP packet for consumers: 
	setABPParameters((char *)SABP, SABPLen, PIRAQ_CP2_SVHABP); 
	//	send ABP data to destination port -- numbered immediately after timeseries ports. send from SABP buffer, use its length: 
	sendBufLen = sendChannel[DATA_CHANNELS + rcvChan]._DataSocket->writeBlock((char *)SABP, SABPLen, sendChannel[QTDSP_SEND_SBAND_ABP]._sendqHost, sendChannel[DATA_CHANNELS + rcvChan]._port);
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

//#ifdef WINSOCK2_TEST //udpsock.cpp winsock2 stuff: TRY THIS INDEPENDENTLY
//	confirm winsock2 available: this works -- no exit
	WSADATA w;
	// Must be done at the beginning of every WinSock program
	int error = WSAStartup (0x0202, &w);   // Fill in w
	if (error) exit(0); 
	if (w.wVersion != 0x0202)  exit(0); 
//#endif
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

	rcvChannel->_DataSocketNotifier = new QSocketNotifier(rcvChannel->_DataSocket->socket(), QSocketNotifier::Read);
	sprintf(m_statusLogBuf, "init rec port %d socket %d", rcvChannel->_port, rcvChannel->_DataSocket->socket()); 
	statusLog->append( m_statusLogBuf );

	rxBufSize = rcvChannel->_DataSocket->receiveBufferSize(); 
	sprintf(m_statusLogBuf, "port %d rxBufSize %d", rcvChannel->_port, rxBufSize); 
	statusLog->append( m_statusLogBuf );

}

void
QtDSP::initializeSendSocket(transmitChannel * sendChannel)	{	//	 
	sendChannel->_DataSocket = new QSocketDevice(QSocketDevice::Datagram);

	char nameBuf[1000];
	strcpy(nameBuf,m_destinationHostname.ascii());
	if (gethostname(nameBuf, sizeof(nameBuf))) {
		statusLog->append("gethostname failed");
		//!add socket active/inactive to struct; set inactive. disallow sends at runtime to inactive socket. 
		//exit(1);
	}

	struct hostent* pHostEnt = gethostbyname(nameBuf);
	if (!pHostEnt) {
		statusLog->append("gethostbyname failed");
		//exit(1);
	}
	sprintf(m_statusLogBuf, "Destination Hostname %s", m_destinationHostname.ascii());
	statusLog->append( m_statusLogBuf );

	sendChannel->_SocketBuf = new CP2_PIRAQ_DATA_TYPE[1000000];	
	std::string destIPname = m_destinationHostname.ascii();
	std::string destIPaddress = inet_ntoa(*(struct in_addr*)pHostEnt->h_addr_list[0]);
	std::cout << "ip name: " << destIPname.c_str() << ", ip address  " << destIPaddress.c_str() << std::endl;

	_sendqHost.setAddress(destIPaddress.c_str()); // works
	sendChannel[0]._sendqHost.setAddress(destIPaddress.c_str()); //?
	std::cout << "opened datagram port:" << sendChannel->_port << std::endl;
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
QtDSP::terminateReceiveSocket( receiveChannel * rcvChannel )	{	//	?pass port#
	if (rcvChannel->_DataSocketNotifier)
		delete rcvChannel->_DataSocketNotifier;

	if (rcvChannel->_DataSocket)
		delete rcvChannel->_DataSocket;

	if (rcvChannel->_SocketBuf)
		delete [] rcvChannel->_SocketBuf;

	sprintf(m_statusLogBuf, "terminating rcv %d rec'd %d packets", rcvChannel->_rcvChannel, rcvChannel->_packetCount);	//	report final packet count
	statusLog->append(m_statusLogBuf);
}
