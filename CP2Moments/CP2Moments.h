#ifndef CP2MomentsH_
#define CP2MomentsH_

#include <QEvent>
#include <QButtonGroup>
#include <math.h>

#include <deque>
#include <set>
#include <map>
#include <string>

#include "ui_CP2Moments.h"
#include "MomentThread.h"
#include "CP2UdpSocket.h" 
#include "CP2Config.h"

#define PIRAQ3D_SCALE	1.0/(unsigned int)pow(2.0,31)	
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
class CP2Moments: public QDialog, public Ui::CP2Moments 
{
	Q_OBJECT
public:
	CP2Moments(QDialog* parent=0);
	virtual ~CP2Moments();

public slots:
	/// stop/start the products generation
	void startStopSlot(bool);
	/// Call when new pulse data is available on the pulse socket.
	void newPulseDataSlot();

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
	void sBeamOut(Beam* pBeam);
	/// Send out a products data gram for X band.
	/// @param pBeam The Xband beam.
	void xBeamOut(Beam* pBeam);
	void sendProduct(CP2ProductHeader& header, 
						 std::vector<double>& data,
						 CP2Packet& packet,
						 bool forceSend=false);
	/// set true if products are to be calculated; false otherwise.
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
	CP2UdpSocket*   _pPulseSocket;
	/// The port number for incoming pulses.
	int	_pulsePort;
	/// The host address for the incoming pulses. 
	QHostAddress _inIpAddr;
	/// The socket that data products are transmitted on.
	CP2UdpSocket*   _pProductSocket;
	/// The host address for the outgoing products. Probably
	/// will be a broadcast address.
	QHostAddress _outIpAddr;
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
	/// S band product data are copied here from the moments computation,
	/// before being added to the outgoing produtcs CP2Packet
	std::vector<double> _sProductData;
	/// X band product data are copied here from the moments computation,
	/// before being added to the outgoing produtcs CP2Packet
	std::vector<double> _xProductData;
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
	/// The configuration for CP2Moments
	CP2Config _config;
};

#endif
