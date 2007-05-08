/// @page CP2Exec
/// CP2Exec is the primary CP2 radar data acquisition program.
/// It manages operation of the Piraq III digital receivers,
/// collecting pulse data from each card, pakaging these into a
/// CP2 defined network structure, and broadcasting them to the 
/// network. Configuration is managed by CP2Exec.ini.
///
///- [General]
/// - Title=CP2Exec
///
///- [Network]
/// - PulseNetwork=192.168.1
/// - PulsePort=3100
///
///- [PciTimer]
/// - SystemClock=48000000
/// - timerMode=1
///
///- [Piraq]
/// - DspObjectFile=c:/Program Files/NCAR/CP2Soft/cp2piraq.out
/// - Gates=950
/// - PrtCounts=6000
/// - XmitWidthCounts=6
///
///- [SimulatedAngles]
/// - Enabled=false

#ifndef CP2EXECH_
#define CP2EXECH_

#include "ui_CP2Exec.h"
#include "CP2PIRAQ.h"
#include "CP2ExecThread.h"
#include <QPalette>
#include <QDateTime>

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
class CP2Exec: public QDialog, public Ui::CP2Exec 
{
	Q_OBJECT
public:
	CP2Exec(QDialog* parent);
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
	/// Palette for making the leds green
	QPalette _greenPalette;
	/// Platette for making the leds red
	QPalette _redPalette;
	/// Preserve the time tht we started, so that we can display an uptime
	QDateTime _startTime;

};

#endif
