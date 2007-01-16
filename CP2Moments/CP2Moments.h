#ifndef CP2MomentsH_
#define CP2MomentsH_

#include <winsock2.h>		//	no redefinition errors if before Qt includes?
#include <qsocketdevice.h> 
#include <qsocketnotifier.h>
#include <qevent.h>
#include <qbuttongroup.h>

#include <deque>
#include <set>
#include <map>
#include <string>

#include "CP2MomentsBase.h"
#include "MomentThread.h"

/// request this much space for the pulse socket receive buffer
#define CP2MOMENTS_PULSE_RCVBUF 100000000
/// request this much space for the product socket send buffer
#define CP2MOMENTS_PROD_SNDBUF  10000000
// the max number of ethernet interfaces a machine might have,
/// used when determininga broadcast address.
#define MAX_NBR_OF_ETHERNET_CARDS 10

/// CP2Moments is a Qt based application which reads S band and X band
/// pulses from a datagram sockets, feeds these pulses into moments
/// calculation engines, and then transmits the resulting Beam products
/// back to the network.
///
/// A pulse is a vector of I/Q data along the gates,
/// from one radar channel (S, Xh or Xv).
///
/// A beam is a vector of multiple radar parameters along
/// the gates, for either the S band radar or the X band radar. 
/// The S beams are derived from the S band pulses.
/// The X beams are derived from the Xh and Xv pulses. 
/// Since the Xh and Xv pulses arrive as independent
/// datagrams, a collator is used to align Xh and Xv
/// pulses with identical timestamps, before delivering
/// them to the moments computation.
class CP2Moments: public CP2MomentsBase 
{
	Q_OBJECT
public:
	CP2Moments();
	virtual ~CP2Moments();

public slots:
	/// stop/start the products generation
	void startStopSlot(bool);
	/// Call when new pulse data is available on the data socket.
	/// @param socket File descriptor of the data socket
	void newPulseDataSlot(int socket);

protected:
	/// The builtin timer will be used to display statistics.
	void timerEvent(QTimerEvent*);
	/// initialize the time series socket for incoming raw data and
	/// the products socket for outgoing products.
	void initializeSockets(); 
	/// Process the pulse, feeding it to the moments processing threads.
	/// @param pPulse The pulse to be processed. 
	void processPulse(CP2Pulse* pPulse);
	/// Send out a products data gram for S band.
	/// @param pBeam The Sband beam.
	void sDatagram(Beam* pBeam);
	/// Send out a products data gram for X band.
	/// @param pBeam The Xband beam.
	void xDatagram(Beam* pBeam);
	void sendProduct(CP2ProductHeader& header, 
						 std::vector<double>& data,
						 CP2Packet& packet,
						 bool forceSend=false);
	/// Calculates a suitable broadcast address to use with
	/// the supplied IP number.
	/// @param dwIp The IP number in net byte order
	/// @return A broadcast address for that IP (255.255.255.255 if 
	/// that IP wasn't found)
	unsigned long CP2Moments::GetBroadcastAddress(char* IPname);
	/// set true if products are to be caclualed; false otherwise.
	bool _run;
	/// The thread which will compute S band moments
	MomentThread* _pSmomentThread;
	/// The thread which will compute X band moments.
	MomentThread* _pXmomentThread;
	/// Set true if S band pulses are to be processed.
	bool _processSband;
	/// Set true if the X band pulses are to be processed.
	bool _processXband;
	/// The time series raw data socket.
	QSocketDevice*   _pPulseSocket;
	/// A notifier for incoming time series data.
	QSocketNotifier* _pPulseSocketNotifier;
	/// The socket that data products are transmitted on.
	QSocketDevice*   _pProductSocket;
	/// The port number for incoming time series data.
	int	_timeSeriesPort;
	/// The host address for the outgoing products. Probably
	/// will be a broadcast address.
	QHostAddress _outHostAddr;
	/// The IP name for the outgoing products
	QString _outHostIP;
	/// The port number for outgoing products.
	int	_productsPort;
	/// The maximum message size that we can send
	/// on UDP.
	int _soMaxMsgSize;
	///	prior cumulative pulse counts, used for throughput calcs
	int	_prevPulseCount[3];	
	///	cumulative pulse counts 
	int	_pulseCount[3];	
	///	cumulative error counts
	int	_errorCount[3];		
	/// A flag that is set true as soon as one EOF is detected on a channel.
	bool _eof[3];		
	/// The pulse number of the preceeding pules, used to detect dropped pulses.
	long long _lastPulseNum[3];
	/// The interval, in seconds, that the statistics displays will be updated.
	int _statsUpdateInterval;
	/// A running count of produced S band beams.
	int _sBeamCount;
	/// A running count of X band beams.
	int _xBeamCount;
	/// A buffer which will receive the time series data from the socket.
	char*   _pPulseSocketBuf;
	/// Moment computation parameters for S band
	Params _Sparams;
	/// Moment computation parameters for X band
	Params _Xparams;
	/// S band product data are copied here from the moments computation,
	/// before being added to the outgoing produtcs CP2Packet
	std::vector<double> _sProductData;
	/// X band product data are copied here from the moments computation,
	/// before being added to the outgoing produtcs CP2Packet
	std::vector<double> _xProductData;
	/// The current S band azimuth
	double _azSband;
	/// The current X band azimuth
	double _azXband;
	/// The collator collects and matches time tags
	/// from H and V Xband channels
	CP2PulseCollator _collator;
	/// The incoming pulse datagrams will be used to fill
	/// in this packet, and then the individual pulses can
	/// be pulled out of it.
	CP2Packet _pulsePacket;
	/// The Sband beam products will be filled into this packet, and 
	/// then it will be writen to the network.
	CP2Packet _sProductPacket;
	/// The band beam products will be filled into this packet, and 
	/// then it will be writen to the network.
	CP2Packet _xProductPacket;
};

#endif
