#ifndef QTDSP_H
#define QTDSP_H

#include <qsocketdevice.h> 
#include <qsocketnotifier.h>
#include <qevent.h>
#include <qtextedit.h>	//	?why needed here? 

#include "QtDSPBase.h"

#include <vector>

//	CP2 Definitions: 
//	CP2_PIRAQ_DATA_TYE defines the type, and thus the size, of the data that is sent in a block after the header.
#define CP2_PIRAQ_DATA_TYPE			float
#define	DISPLAY_REFRESH_INTERVAL	1000	//	mSec display refresh 
#define	QTDSP_BASE_PORT			21010	//	QtDSP base port# for broadcast
#define	CP2EXEC_BASE_PORT			3100	//	CP2exec base port# for receive data
#define CP2_DATA_CHANNELS			4		//	two channels each for S and X band 

#define	MULTIPORT
//#define	noMULTIPORT

class QGroupBox;

class QtDSP : public QtDSPBase {
	Q_OBJECT		// per Qt documentation: "strongly recommended"
public:
	QtDSP();
	~QtDSP();

#ifdef	MULTIPORT
	void initializeSocket(int port); //	port to open socket on
#else
	void initializeSocket(); 
#endif
	void terminateSocket(); 
	void connectDataRcv();

public slots:
	void startStopReceiveSlot();	

	// Call when data is available on the data socket.
	void dataSocketActivatedSlot(
		int socket         // File descriptor of the data socket
		);

protected:
	//	objects, etc. for 1 comm channel: 
	QSocketDevice*   m_pDataSocket;
	QSocketNotifier* m_pDataSocketNotifier;
	CP2_PIRAQ_DATA_TYPE*   m_pSocketBuf;

	char			m_statusLogBuf[1024];		//	global status string for Status Log
	int				m_receiving;		//	receive-data state
	int				m_receivePacketCount;		//	cumulative receive packet count  
	int				m_dataDisplayTimer;	//	say 1/sec for a smapp data-rate display 
	double			m_yScaleMin, m_yScaleMax;	//	y-scale min, max

	QString m_destinationHostname;		//	lineedit input strings 
	QString m_destinationPortNumber; 
	QString m_receivePortNumber; 
	int	m_destinationPort;				//	program values from user input
	int	m_receivePort; 

	/// The IQ data packet: rename for clarity 
	std::vector<double> IQ;	//	single-pulse timeseries arrays; names need decoration throughout
	void timerEvent(QTimerEvent *e);

};

#endif
