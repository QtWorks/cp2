#ifndef CP2EXECH_
#define CP2EXECH_

#include "CP2PIRAQ.h"
#include "CP2ExecBase.h"
#include "CP2ExecThread.h"

/// CP2Exec is the main Qt dialog that
/// manages the CP2 Piraq data acquisition
/// subsystem. It also presents status
/// information about the piraqs and
/// data throughput.
///
/// In order for the GUI and the piraq
/// data activity to run concurrently,
/// the CP2ExecThread is used to manage
/// the piraqs in a separate thread. 
/// CP2Exec calls status functions
/// in CP2ExecThread to query the state
/// of that thread. 
class CP2Exec: public CP2ExecBase 
{
	Q_OBJECT
public:
	CP2Exec(std::string dspObjFile, std::string configFile);
	virtual ~CP2Exec();

public slots:

protected:
	/// The piraq management thread
	CP2ExecThread* _pThread;
	/// The file containing the dsp object code
	std::string _dspObjFile;
	/// The configuration file
	std::string _configFile;
	// The builtin timer will be used to display statistics.
	void timerEvent(QTimerEvent*);
	/// The update interval (s) for the status and 
	/// statistics reporting
	int _statsUpdateInterval;
	/// The timer will start out at a fast rate, but
	/// once the status is RUNNING, then it will update
	/// less frequently.
	bool _timerReset;
	/// The id of the stats timer.
	int _timerId;
	/// EOF for piraq 0 flag tracks the state of the EOF widget,
	/// so that we don't keep setting it if it is 
	/// already set.
	bool _eofLed0;
	/// EOF for piraq 1 flag tracks the state of the EOF widget,
	/// so that we don't keep setting it if it is 
	/// already set.
	bool _eofLed1;
	/// EOF for piraq 2 flag tracks the state of the EOF widget,
	/// so that we don't keep setting it if it is 
	/// already set.
	bool _eofLed2;

};

#endif
