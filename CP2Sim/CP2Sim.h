#ifndef CP2SIMINC_
#define CP2SIMINC_

#include "ui_CP2Sim.h"
#include "CP2UdpSocket.h" 
#include "CP2Config.h"
#include "CP2Net.h"
#include "CP2SimThread.h"

using namespace CP2Net;

/*!
CP2Sim is a Qt based application which creates a simulated
stream of CP2 pulses.

A pulse is a vector of I/Q data along the gates,
from one radar channel (S, Xh or Xv).

*/
class CP2Sim: public QDialog, public Ui::CP2Sim 
{
	Q_OBJECT
public:
	/// Constructor
	CP2Sim(QDialog* parent=0);
	/// Destructor
	virtual ~CP2Sim();

public slots:
	/// stop/start the pulse generation
	void startStopSlot();

protected:
	/// The builtin timer will be used to display statistics.
	virtual void timerEvent(QTimerEvent*);
	/// The configuration
	CP2Config _config;
	/// set true if pulses are to be calculated; false otherwise.
	bool _run;
	/// The thread which will cerate the pulses.
	CP2SimThread* _pCP2SimThread;
	/// The host address for the outgoing pulses. 
	QHostAddress _outIpAddr;
	/// The interval, in seconds, that the statistics displays will be updated.
	int _statsUpdateInterval;
	/// A running count of produced pulses.
	int _sPulseCount;
	/// The number of S gates. 
	int _sGates;
	/// The number of X gates. 
	int _xGates;
};

#endif
