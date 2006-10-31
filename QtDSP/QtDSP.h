#ifndef QTDSP_H
#define QTDSP_H

#include <winsock2.h>		//	no redefinition errors result if before Qt includes
#include <qsocketdevice.h> 
#include <qsocketnotifier.h>
#include <qevent.h>
#include <qtextedit.h>	//	?why needed here? 

#include "QtDSPBase.h"

#include "../include/CP2.h"			//	CP2-wide Definitions
#include "../include/piraqx.h"		//	CP2-piraqx Definitions
#include "../include/dd_types.h"	//	CP2-piraqx data types

#include "Fifo.h"		//	std::vector-based implementation 

#include <vector>
#include <map>

//	QtDSP-specific definitions: 
//	3 IQ receive channels: 
enum	{	QTDSP_RECEIVE_SBAND, QTDSP_RECEIVE_XBANDH, QTDSP_RECEIVE_XBANDV, QTDSP_RECEIVE_CHANNELS	};	
//	3 IQ, 3 ABP send channels.  
enum	{	QTDSP_SEND_SBAND_IQ, QTDSP_SEND_XBANDH_IQ, QTDSP_SEND_XBANDV_IQ, QTDSP_SEND_SBAND_ABP, QTDSP_SEND_XBANDH_ABP, QTDSP_SEND_XBANDV_ABP, QTDSP_SEND_CHANNELS	};	

#define	QTDSP_MONITOR_INTERVAL		1000	//	mSec monitor interval to activate/deactivate data channels 
#define	QTDSP_MONITOR_RECEIVE_SECS	5		//	seconds before receive channel deemed "silent"
#define	DATA_CHANNELS				3		//	# data channels in system

class QtDSP : public QtDSPBase {
	Q_OBJECT		// per Qt documentation: "strongly recommended"
public:
	QtDSP();
	~QtDSP();

	int	getParameters( CP2_PIRAQ_DATA_TYPE[], int rcvChan);	//	get program operating parameters from piraqx (or other) "housekeeping" header structure
	void setABPParameters( char * ABPPacket, uint4 recordlen, uint4 dataformat );	//	set parameters in ABP "housekeeping" header structure
	void SABPgen(int rcvChan);	//	generate S-band ABPs on rcvChan data  
	void XHABPgen(int rcvChan); 
	void XVABPgen(int rcvChan); 

public slots:
	void startStopReceiveSlot();	

	//	one SIGNAL/SLOT pair for each receive channel: 
	void dataReceiveSocketActivatedSlot( int socket );

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
	QString m_IQdestinationPortNumber; 
	QString m_ABPdestinationPortNumber; 
	QString m_receivePortNumber; 
	int	m_IQdestinationPort;				//	program values from user input
	int	m_ABPdestinationPort;	
	int	m_receivePort; 

	int				_pulseStride;	//	length in bytes of 1 pulse: header + data
	int				_Nhits;			//	pulses combined into one packet by PIRAQ
	unsigned int	_PNOffset;		//	offset in bytes to pulsenumber from begin of CP2exec-generated packet
	unsigned int	_IQdataOffset;	//	offset to begin of data in (piraqx) packet

	uint8			_SABPgenBeginPN;	//	1st PN in beam to compute pulsepairs

	/// save the mapping of sockets to receive channels
	std::map<int, int> _socketToChannelMap;

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
		int _parametersSet;		//	set if operating parameters obtained from incoming data
		unsigned int _secsSilent;	//	seconds receive channel silent
		uint8 _PN;				//	most-recent received pulsenumber
		//	parameters related to this channel and its radar description: 
		//	operating parameters from piraqx (or other header) 
		//  "housekeeping" for use by program: data types per piraqx.h
		uint4			_gates;
		uint4			_hits;
		//	operating parameters derived from piraqx and N-hit implementation:	
		uint4			_radarType;		//	set by field "desc", describing the radar function
		unsigned int	_pulsesToProcess;	//	pulses to process, this channel
		Fifo*			_fifo;		//	std::vector implementation 
		uint4			_fifo_hits;	//	timeseries hits: fifo "size"
		int    lastreadBufLen;
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

	Fifo * _fifo1;	//	std::vector implementation 
	Fifo * _fifo2;	
	Fifo * _fifo3;	

	void initializeReceiveSocket(receiveChannel * rcvChannel);	//	pointer to struct containing udp receive socket parameters
	void initializeSendSocket(transmitChannel * sendChannel);	//	ditto for udp send socket 
	void terminateReceiveSocket(receiveChannel * rcvChannel);	//	
	void connectUp();
	void timerEvent(QTimerEvent *e);

};

#endif
