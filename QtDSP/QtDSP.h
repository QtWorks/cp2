#ifndef QTDSP_H
#define QTDSP_H

//#include <winsock.h>		//	no redefinition errors if before Qt includes
#include <winsock2.h>		//	no redefinition errors if before Qt includes
#include <qsocketdevice.h> 
#include <qsocketnotifier.h>
#include <qevent.h>
#include <qtextedit.h>	//	?why needed here? 

#include "QtDSPBase.h"

#include "../include/CP2.h"			//	CP2-wide Definitions
#include "../include/piraqx.h"		//	CP2-piraqx Definitions
#include "../include/dd_types.h"	//	CP2-piraqx data types

#include <vector>

//	QtDSP-specific definitions:
enum	{	QTDSP_RECEIVE_CHANNEL, QRC_UNUSED1, QRC_UNUSED2, QRC_UNUSED3, QTDSP_RECEIVE_CHANNELS	};	//	4 receive channels 
//enum	{	QTDSP_RECEIVE_CHANNEL, QTDSP_RECEIVE_CHANNELS	};	//	receive all data on one channel
enum	{	QTDSP_SEND_CHANNEL, QSC_UNUSED1, QSC_UNUSED2, QSC_UNUSED3, QTDSP_SEND_CHANNELS	};	//	4 send channels 
//enum	{	QTDSP_SEND_CHANNEL, QTDSP_SEND_CHANNELS	};	//	send all data on one channel 
#define	QTDSP_DISPLAY_REFRESH_INTERVAL	1000	//	mSec display refresh 

#define	noMULTIPORT	//	switch multiple--port functions
#define	noSTREAM_SOCKET		//	socket device Stream

class QGroupBox;

class QtDSP : public QtDSPBase {
	Q_OBJECT		// per Qt documentation: "strongly recommended"
public:
	QtDSP();
	~QtDSP();

	int	getParameters( CP2_PIRAQ_DATA_TYPE[] );		//	get program operating parameters from piraqx (or other) "housekeeping" header structure

public slots:
	void startStopReceiveSlot();	

	//	one SIGNAL/SLOT pair for each receive channel: 
	void dataReceiveSocketActivatedSlot0(
		int socket         // File descriptor of the data socket
		);
	void dataReceiveSocketActivatedSlot1(
		int socket         // File descriptor of the data socket
		);
	void dataReceiveSocketActivatedSlot2(
		int socket         // File descriptor of the data socket
		);

protected:
	//	objects, etc. for 1 comm channel: 
	QSocketDevice*   m_pDataSocket;
	QSocketNotifier* m_pDataSocketNotifier;
	CP2_PIRAQ_DATA_TYPE*   m_pSocketBuf;
	QHostAddress _sendqHost; 

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

	//	generalize to all channels: 
	int				_parametersSet;	//	set when piraqx parameters successfully initialized from received data
	//	operating parameters from piraqx (or other header) for use by program: data types per piraqx.h
	uint4			_gates;
	//	operating parameters derived from piraqx and N-hit implementation:	
	int				_pulseStride;	//	length in bytes of 1 pulse: header + data
	int				_Nhits;			//	

	//	define structure array of receive channels: 
	struct	receiveChannel {
		int	_rcvChannel;	//	index
		int	_port;
		QSocketDevice*   _DataSocket;
		QSocketNotifier* _DataSocketNotifier;
		int	_sockBufSize;	//	in bytes
		CP2_PIRAQ_DATA_TYPE*   _SocketBuf;
		int	_packetCount;
		int	_packetCountErrors;
	} rcvChannel[QTDSP_RECEIVE_CHANNELS];

	//	define structure array of send channels: 
	struct	transmitChannel {
		int	_sendChannel;	//	index
		int	_port;
		QSocketDevice*   _DataSocket;
		QSocketNotifier* _DataSocketNotifier;
		QHostAddress	_sendqHost;
		int	_sockBufSize;	//	in bytes
		CP2_PIRAQ_DATA_TYPE*   _SocketBuf;
		int	_packetCount;
		int	_packetCountErrors;
	} sendChannel[QTDSP_SEND_CHANNELS];

	/// The IQ data packet: rename for clarity 
	std::vector<double> IQ;	//	single-pulse timeseries arrays; names need decoration throughout
	void timerEvent(QTimerEvent *e);

	void initializeReceiveSocket(receiveChannel * rcvChannel);	//	pointer to struct containing udp receive socket parameters
	void initializeSendSocket(transmitChannel * sendChannel);	//	ditto for udp send socket 
	void terminateReceiveSocket(receiveChannel * rcvChannel);	//	!generalize 
	void connectDataRcv();
};

#endif
