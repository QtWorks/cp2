#ifndef CP2SIMTHREADH_
#define CP2SIMTHREADH_

#include <qthread.h>
#include <qmutex.h>
#include <qwaitcondition.h>
#include <vector>

// CP2 timeseries network transfer protocol.
#include "CP2Net.h"
using namespace CP2Net;

// Configuration management
#include "CP2Config.h"

// UDP socket support
#include "CP2UdpSocket.h"

// Angle simulator
#include "SimAngles.h"

class CP2SimThread :
	public QThread
{
public:
	CP2SimThread();
	virtual ~CP2SimThread(void);
	virtual void run();
	void setRunState(bool runState);

protected:
	/// initialize the socket for outgoing pulses.
	void initializeSocket(); 
	/// Send out the next set of pulses
	void nextPulses();
	// send the data on the pulse socket
	int sendData(int size, void* data);
	// create a SimAngles from the configuration
	SimAngles createSimAngles();
	/// The configuration
	CP2Config _config;
	/// set true if pulses are to be calculated; false otherwise.
	bool _run;
	/// The time series raw data socket.
	CP2UdpSocket*   _pPulseSocket;
	/// The port number for outgoing pulses.
	int	_pulsePort;
	/// The maximum message size that we can send
	/// on UDP.
	int _soMaxMsgSize;
	/// data fill area
	std::vector<float> _data;
	/// number of gates
	int _gates;
	/// Number of pulses to put in each datagram
	int _pulsesPerDatagram;
	/// The number of resends on the datagram
	int _resendCount;

	CP2Packet _sPacket;
	CP2Packet _xhPacket;
	CP2Packet _xvPacket;

	SimAngles _simAngles;


};

#endif

