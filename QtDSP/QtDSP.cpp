//
#include "qtdsp.h"
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qspinbox.h>
#include <qlineedit.h>

#include <iostream>

//#define	WINSOCK2_TEST	//	testing winsock2 implementation

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
	sprintf(_postIt, "%d", QTDSP_BASE_PORT);	_destinationPortBase->setText( QString( _postIt ) ); 
	//	default Destination Hostname to this machine: 
#ifdef	STREAM_SOCKET
	m_pDataSocket = new QSocketDevice(QSocketDevice::Stream);	//	exit() below without this 
#else
	m_pDataSocket = new QSocketDevice(QSocketDevice::Datagram);	//	exit() below without this 
#endif
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
	// Start a timer to tell the display to update
	m_dataDisplayTimer = startTimer(QTDSP_DISPLAY_REFRESH_INTERVAL);

	_parametersSet = 0;		//	piraqx parameters not yet initialized from received data
}

QtDSP::~QtDSP() {

	if	(m_receiving)	{	//	currently receiving on input: generalize
		for (int i = 0; i < QTDSP_RECEIVE_CHANNELS; i++) {	//	all receive channels
		    terminateReceiveSocket(&rcvChannel[i]);
		}
	}
	//?delete timer? 
}

void 
QtDSP::timerEvent(QTimerEvent *e) {
}


int
QtDSP::getParameters( CP2_PIRAQ_DATA_TYPE[] )	// get program operating parameters from piraqx (or other) "housekeeping" header structure 
{
	_gates = *(int *)((rcvChannel[0]._SocketBuf) + 108/sizeof(int));	//	gates: 32+28+48 as expected; 
	_pulseStride = (sizeof(PACKETHEADER) + 8*_gates)/sizeof(float);	//	stride in floats; 2nd term DATASIZE: 8 bytes per gate wired for now 
	//	compute #hits combined by piraq: equal in both piraq executable (CP2_DCCS3_1.out) and host applications
	_Nhits = UDPSENDSIZE / (HEADERSIZE + (8*_gates));	//	8 bytes per gate: define
	return(1);	//	condition this 
}

void
QtDSP::connectDataRcv()	// connect receive notifiers to data-receive slot
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
	m_destinationPortNumber = _destinationPortBase->text(); 
	m_receivePortNumber = _receivePortBase->text(); 
	//	string contents do not appear in Debugger without this:
	std::cout << "->text() returned " << m_destinationHostname << std::endl;
	std::cout << "->text() returned " << m_destinationPortNumber << std::endl;
	std::cout << "->text() returned " << m_receivePortNumber << std::endl;
	//does not work:	m_destinationPort = atoi(_destinationPort->text()); 
	m_destinationPort = atoi(m_destinationPortNumber); 
	m_receivePort = atoi(m_receivePortNumber); 

	//	create the sockets that receive packets from CP2exec:   
	//	initialize the channels that receive packets from CP2exec:   
	for (int i = 0; i < QTDSP_RECEIVE_CHANNELS; i++)	{	//	MULTIPORT: all receive channels
		rcvChannel[i]._rcvChannel = i;	//	enum'd channel name
		rcvChannel[i]._port = m_receivePort + i;		//	define ports from base value 
		rcvChannel[i]._packetCount = rcvChannel[i]._packetCountErrors = 0; 
		rcvChannel[i]._sockBufSize = 1000000;	//	default buffer size; customize as necessary
		initializeReceiveSocket(&rcvChannel[i]); //	initialize receive socket
	}
	//	send sockets for timeseries, moments data: 
	for (int i = 0; i < QTDSP_SEND_CHANNELS; i++)	{	//	MULTIPORT: all send channels
		sendChannel[i]._sendChannel = i;	//	enum'd channel name
		sendChannel[i]._port = m_destinationPort + i;		//	define ports from base value 
		sendChannel[i]._packetCount = sendChannel[i]._packetCountErrors = 0; 
		sendChannel[i]._sockBufSize = 1000000;	//	default buffer size; customize as necessary
		initializeSendSocket(&sendChannel[i]); //	initialize send socket
	}
	connectDataRcv(); 
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
	int	r_c;	//	return_code
	//	define p: pointer to packet data
	float* p = ((float*)(rcvChannel[0]._SocketBuf)) + _pulseStride*hit + (sizeof(PACKETHEADER)/sizeof(float));	//	
	uint8* PNptr = (uint8*)((char *)(rcvChannel[0]._SocketBuf) + 32 + 28 + 32);	//	
	static	uint8	PN, lastPN;
//	... to here
	rcvChannel[rcvChan]._packetCount++; 

	readBufLen = rcvChannel[rcvChan]._DataSocket->readBlock((char *)rcvChannel[rcvChan]._SocketBuf, sizeof(short)*1000000);
//	readBufLen = rcvChannel[rcvChan]._DataSocket->readBlock((char *)rcvChannel[rcvChan]._SocketBuf, sizeof(short)*2000000);
	if	(lastreadBufLen != readBufLen)	{	//	packet length varies: log it
		sprintf(m_statusLogBuf, "len error %d: rBL %d lrBL %d", rcvChannel[rcvChan]._packetCount, readBufLen, lastreadBufLen); statusLog->append( m_statusLogBuf ); 
	}
	else	{	//	get piraqx parameters
		if	(!_parametersSet)	{	//	piraqx parameters need to be initialized from received data
			r_c = getParameters( rcvChannel[0]._SocketBuf );	//	 
			if	(r_c == 1)	{		//	success
				_parametersSet = 1; 
			}
		}
	}

	lastreadBufLen = readBufLen;
	if (!(rcvChannel[rcvChan]._packetCount % 100))	{
		if(readBufLen > 0) {
			sprintf(m_statusLogBuf, "socket %d: rBL %d lrBL %d pkts %d", socket, readBufLen, lastreadBufLen, rcvChannel[rcvChan]._packetCount); statusLog->append( m_statusLogBuf );
		}
	}
//	!test PNs sequential; log if not
		//	p is set to 1st pulse in packet
#ifndef WINSOCK2_TEST // turn OFF for winsock2 test until rx works
	if	(_parametersSet)	{	//	operating parameters set, test PNs sequential
		for	(hit = 0; hit < _Nhits; hit++)	{	//	all hits in packet
			PN	= *PNptr;
			//	maintain p, detect full set, break
			p += _pulseStride; 
			if	(PN != lastPN + 1)	{	
				errcount++;	
				sprintf(m_statusLogBuf, "hit%d: lastPN = %I64d PN = %I64d errors = %d", hit, lastPN, PN, errcount); statusLog->append( m_statusLogBuf );
			}	//	we'll always get 1 at begin
			lastPN = PN; 
			PNptr = (uint8 *)((char *)PNptr + _pulseStride*sizeof(float)); 
		}
	}
#endif
	//	send rcv'd data to destination port: !send from receive buffer, use its length: 
#ifndef WINSOCK2_TEST // turn OFF for winsock2 test until rx works
	sendBufLen = sendChannel[rcvChan]._DataSocket->writeBlock((char *)rcvChannel[rcvChan]._SocketBuf, readBufLen, sendChannel[rcvChan]._sendqHost, sendChannel[rcvChan]._port);
#endif
}


void 
QtDSP::dataReceiveSocketActivatedSlot1(int socket)	//	socket not used ... pass ? for MULTIPORT implementation
{
	static	int errcount = 0;
	static	int readBufLen, lastreadBufLen;
	static	int rcvChan = 1;	//	receive channel 
	//	temp send 
	static	int sendBufLen, lastsendBufLen;

	rcvChannel[rcvChan]._packetCount++;
	readBufLen = rcvChannel[rcvChan]._DataSocket->readBlock((char *)rcvChannel[rcvChan]._SocketBuf, sizeof(short)*1000000);
	if	(lastreadBufLen != readBufLen)	{	//	packet length varies: log it
		sprintf(m_statusLogBuf, "%d: rBL %d lrBL %d", rcvChannel[rcvChan]._packetCount, readBufLen, lastreadBufLen); statusLog->append( m_statusLogBuf ); 
	}

	lastreadBufLen = readBufLen;
	if (!(rcvChannel[rcvChan]._packetCount % 100))	{
		if(readBufLen > 0) {
			sprintf(m_statusLogBuf, "socket %d: rBL %d lrBL %d pkts %d", socket, readBufLen, lastreadBufLen, rcvChannel[rcvChan]._packetCount); statusLog->append( m_statusLogBuf );
		}
	}
	//	send rcv'd data to destination port: !send from receive buffer, use its length: 
	sendBufLen = sendChannel[rcvChan]._DataSocket->writeBlock((char *)rcvChannel[rcvChan]._SocketBuf, readBufLen, sendChannel[0]._sendqHost, sendChannel[rcvChan]._port);

}


void 
QtDSP::dataReceiveSocketActivatedSlot2(int socket)	
{
	static	int errcount = 0;
	static	int readBufLen, lastreadBufLen;
	static	int rcvChan = 2;	//	receive channel 
	//	temp send 
	static	int sendBufLen, lastsendBufLen;

	rcvChannel[rcvChan]._packetCount++;
	readBufLen = rcvChannel[rcvChan]._DataSocket->readBlock((char *)rcvChannel[rcvChan]._SocketBuf, sizeof(short)*1000000);
	if	(lastreadBufLen != readBufLen)	{	//	packet length varies: log it
		sprintf(m_statusLogBuf, "%d: rBL %d lrBL %d", rcvChannel[rcvChan]._packetCount, readBufLen, lastreadBufLen); statusLog->append( m_statusLogBuf ); 
	}

	lastreadBufLen = readBufLen;
	if (!(rcvChannel[rcvChan]._packetCount % 100))	{
		if(readBufLen > 0) {
			sprintf(m_statusLogBuf, "socket %d: rBL %d lrBL %d pkts %d", socket, readBufLen, lastreadBufLen, rcvChannel[rcvChan]._packetCount); statusLog->append( m_statusLogBuf );
		}
	}
	//	send rcv'd data to destination port: !send from receive buffer, use its length: 
//	sendBufLen = sendChannel[rcvChan]._DataSocket->writeBlock((char *)rcvChannel[rcvChan]._SocketBuf, readBufLen, sendChannel[rcvChan]._sendqHost, sendChannel[rcvChan]._port);

}


void
QtDSP::initializeReceiveSocket(receiveChannel * rcvChannel)	{	//	receiveChannel struct w/pointers to new objects, etc. 
	int	rxBufSize = 0; 

#ifdef	STREAM_SOCKET
	rcvChannel->_DataSocket = new QSocketDevice(QSocketDevice::Stream);
#else
	rcvChannel->_DataSocket = new QSocketDevice(QSocketDevice::Datagram);
#endif
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
	std::cout << "ip name: " << myIPname.c_str() << ", id address  " << myIPaddress.c_str() << std::endl;

	qHost.setAddress(myIPaddress.c_str());
	m_receivePacketCount = 0; 

	std::cout << "qHost:" << qHost.toString() << std::endl;
	std::cout << "opened datagram port:" << rcvChannel->_port << std::endl;
	if (!rcvChannel->_DataSocket->bind(qHost, rcvChannel->_port)) {
		sprintf(m_statusLogBuf, "Unable to bind to %s:%d", qHost.toString().ascii(), rcvChannel->_port);
		statusLog->append( m_statusLogBuf );
		exit(1); 
	}
//	int sockbufsize = 1000000;
	int sockbufsize = 20000000;
//	int sockbufsize = UDPSENDSIZE;

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
QtDSP::initializeSendSocket(transmitChannel * sendChannel)	{	//	?pass port#, socket struct w/pointers to new objects? 
	sendChannel->_DataSocket = new QSocketDevice(QSocketDevice::Datagram);
	QHostAddress qHost;

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
	sendChannel->_SocketBuf = new CP2_PIRAQ_DATA_TYPE[1000000];

	std::string myIPname = nameBuf;
	std::string myIPaddress = inet_ntoa(*(struct in_addr*)pHostEnt->h_addr_list[0]);
	std::cout << "ip name: " << myIPname.c_str() << ", id address  " << myIPaddress.c_str() << std::endl;

	qHost.setAddress(myIPaddress.c_str());
	_sendqHost.setAddress(myIPaddress.c_str()); // works
	sendChannel[0]._sendqHost.setAddress(myIPaddress.c_str()); //?
	std::cout << "qHost:" << qHost.toString() << std::endl;
	std::cout << "opened datagram port:" << sendChannel->_port << std::endl;
#if 0	//?if bind CP2Scope does not receive
	if (!sendChannel->_DataSocket->bind(qHost, sendChannel->_port)) {
		sprintf(m_statusLogBuf, "Unable to bind to %s:%d", qHost.toString().ascii(), sendChannel->_port);
		statusLog->append( m_statusLogBuf );
		exit(1); 
	}
#endif
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
//?need this?
#if 0	//?
	sendChannel->_DataSocketNotifier = new QSocketNotifier(sendChannel->_DataSocket->socket(), QSocketNotifier::Write);
	sprintf(m_statusLogBuf, "init send port %d", sendChannel->_port); 
	statusLog->append( m_statusLogBuf );
#endif

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
