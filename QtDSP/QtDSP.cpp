
#include "qtdsp.h"
//#include <ScopePlot/ScopePlot.h>
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qspinbox.h>
#include <qlineedit.h>

#include <winsock.h>
#include <iostream>

QtDSP::QtDSP():
m_pDataSocket(0),    
m_pDataSocketNotifier(0),
m_pSocketBuf(0)
{
	m_receiving		= 0;		//	receive-data state
	m_receivePacketCount	= 0; 
	//	set default destination, notify ports:
	char	_postIt[10];		//	too small to be a scratchpad
	sprintf(_postIt, "%d", CP2EXEC_BASE_PORT);	_receivePort->setText( QString( _postIt ) ); 
	sprintf(_postIt, "%d", QTDSP_BASE_PORT);	_destinationPort->setText( QString( _postIt ) ); 
	//	set Destination Hostname to this machine: 
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
	// Start a timer to tell the display to update
	m_dataDisplayTimer = startTimer(DISPLAY_REFRESH_INTERVAL);

}

QtDSP::~QtDSP() {

	if	(m_receiving)		//	currently receiving on input: generalize
		terminateSocket();
//		terminateSocket(sock);
	//?delete timer? 
}

void 
QtDSP::timerEvent(QTimerEvent *e) {
}


void
QtDSP::connectDataRcv()	// connect notifier to data-receive slot 
{
	bool r_c;
	r_c = connect(m_pDataSocketNotifier, SIGNAL(activated(int)), this, SLOT(dataSocketActivatedSlot(int)));
}


void
QtDSP::startStopReceiveSlot()	//	activated on either lostFocus or returnPressed
{
	if	(m_receiving)	{		//	currently receiving data
		terminateSocket();
		m_receiving = !m_receiving; 
		statusLog->append( "Not Receiving" ); 
		return;
	}
	//	toggle start/stop
	m_receiving = !m_receiving; 
	//	pick up user input
	m_destinationHostname = _destinationHostname->text(); 
	m_destinationPortNumber = _destinationPort->text(); 
	m_receivePortNumber = _receivePort->text(); 
	//	string contents do not appear in Debugger without this:
	std::cout << "->text() returned " << m_destinationHostname << std::endl;
	std::cout << "->text() returned " << m_destinationPortNumber << std::endl;
	std::cout << "->text() returned " << m_receivePortNumber << std::endl;
	//does not work:	m_destinationPort = atoi(_destinationPort->text()); 
	m_destinationPort = atoi(m_destinationPortNumber); 
	m_receivePort = atoi(m_receivePortNumber); 

	// create the socket that receives packets from CP2exec:   
#ifdef	MULTIPORT
	initializeSocket(m_receivePort); //	port to open socket on: receive only 1st test
#else
	initializeSocket();	
#endif
	connectDataRcv(); 
	statusLog->append( "Receiving" ); 
}

void 
QtDSP::dataSocketActivatedSlot(int socket)
{
	static int errcount = 0;
	int readBufLen, lastreadBufLen;
	
	m_receivePacketCount++; 
	readBufLen = m_pDataSocket->readBlock((char *)m_pSocketBuf, sizeof(short)*1000000);
	lastreadBufLen = readBufLen;
	if	(lastreadBufLen != readBufLen)	{	sprintf(m_statusLogBuf, "rbL %d lrbL %d", readBufLen, lastreadBufLen); statusLog->append( m_statusLogBuf ); }
	if (!(m_receivePacketCount % 100))	{
		if(readBufLen > 0) {
			sprintf(m_statusLogBuf, "rbL %d lrbL %d pkts %d", readBufLen, lastreadBufLen, m_receivePacketCount); statusLog->append( m_statusLogBuf );
		}
	}
}


void
#ifdef	MULTIPORT
QtDSP::initializeSocket(int port)	{	//	?pass port#, socket struct w/pointers to new objects? 
#else
QtDSP::initializeSocket()	{	//	?pass port#, socket struct w/pointers to new objects? 
#endif
	m_pDataSocket = new QSocketDevice(QSocketDevice::Datagram);

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
	m_pSocketBuf = new CP2_PIRAQ_DATA_TYPE[1000000];

	std::string myIPname = nameBuf;
	std::string myIPaddress = inet_ntoa(*(struct in_addr*)pHostEnt->h_addr_list[0]);
	std::cout << "ip name: " << myIPname.c_str() << ", id address  " << myIPaddress.c_str() << std::endl;

	qHost.setAddress(myIPaddress.c_str());
	m_receivePacketCount = 0; 

	std::cout << "qHost:" << qHost.toString() << std::endl;
#ifdef	MULTIPORT
	std::cout << "opened datagram port:" << port << std::endl;
#else
	std::cout << "receive datagram port:" << m_receivePort << std::endl;
#endif
#ifdef	MULTIPORT
	if (!m_pDataSocket->bind(qHost, port)) {
		sprintf(m_statusLogBuf, "Unable to bind to %s:%d", qHost.toString().ascii(), port);
#else
	if (!m_pDataSocket->bind(qHost, m_receivePort)) {
		sprintf(m_statusLogBuf, "Unable to bind to %s:%d", qHost.toString().ascii(), m_receivePort);
#endif
		statusLog->append( m_statusLogBuf );
		exit(1); 
	}
	int sockbufsize = 1000000;

	int result = setsockopt (m_pDataSocket->socket(),
		SOL_SOCKET,
		SO_RCVBUF,
		(char *) &sockbufsize,
		sizeof sockbufsize);
	if (result) {
		statusLog->append("Set receive buffer size for socket failed");
		exit(1); 
	}

	m_pDataSocketNotifier = new QSocketNotifier(m_pDataSocket->socket(), QSocketNotifier::Read);
}


void 
QtDSP::terminateSocket( void )	{	//	?pass port#
	if (m_pDataSocketNotifier)
		delete m_pDataSocketNotifier;

	if (m_pDataSocket)
		delete m_pDataSocket;

	if (m_pSocketBuf)
		delete [] m_pSocketBuf;

	sprintf(m_statusLogBuf, "rec'd %d packets", m_receivePacketCount);	//	report final packet count
	statusLog->append(m_statusLogBuf);
}
