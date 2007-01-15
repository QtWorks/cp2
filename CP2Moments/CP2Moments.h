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

// request this much space for the socket receive buffer
#define CP2PPI_RCVBUF 100000000

class CP2Moments: public CP2MomentsBase 
{
	Q_OBJECT
public:
	CP2Moments();
	virtual ~CP2Moments();

public slots:
	void stopSlot(bool);
	// Call when data is available on the data socket.
	void newDataSlot(
		int socket         ///< File descriptor of the data socket
		);

protected:
	// The builtin timer will be used to display statistics.
	void timerEvent(QTimerEvent*);
	void initializeSocket(); 
	void sDatagram(Beam* pBeam);
	void xDatagram(Beam* pBeam);

	QSocketDevice*   _pSocket;
	QSocketNotifier* _pSocketNotifier;
	int				_dataGramPort;
	int				_prevPulseCount[3];			///<	prior cumulative pulse count, used for throughput calcs
	int				_pulseCount[3];				///<	cumulative pulse count
	int				_errorCount[3];				///<	cumulative error count
	bool			_eof[3];						///<    set true when fifo eof occurs. Used so that we don't 
	long long       _lastPulseNum[3];
	int _statsUpdateInterval;
	int _sBeamCount;
	int _xBeamCount;
	char*   _pSocketBuf;

		/// Moment computation parameters for S band
	Params _Sparams;

	/// Moment computation parameters for X band
	Params _Xparams;

	std::vector<double> _sProductData;
	std::vector<double> _xProductData;

	double _azSband;

	double _azXband;

	/// Process the pulse, feeding it to the moments processor
	/// @param pPulse The pulse to be processed. 
	void processPulse(CP2Pulse* pPulse);

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

	MomentThread* _pSmomentThread;
	MomentThread* _pXmomentThread;

	bool _processSband;
	bool _processXband;

};

#endif
