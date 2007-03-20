#ifndef CP2EXECTHREADH_
#define CP2EXECTHREADH_

#include <string>
#include <qthread.h>
#include <QUdpSocket>

#include "CP2PIRAQ.h"
#include "CP2Net.h"
#include "CP2UdpSocket.h"
#include "CP2Config.h"
#include "SimAngles.h"

class CP2ExecThread: public QThread {

public:
	enum STATUS {STARTUP, PIRAQINIT, RUNNING};
	CP2ExecThread(std::string dspObjfile, std::string configFile);
	virtual ~CP2ExecThread();
	void run();
	void stop();
	STATUS status();
	void pnErrors(int& errors1, int& errors2, int& errors3);
	void pulses(int& pulses1, int& pulses2, int& pulses);
	void rates(double& rate1, double& rate2, double& rate3);
	void eof(bool eof[3]);
	void antennaInfo(double* az, 
		double* el, 
		unsigned int* sweep, 
		unsigned int* volume);

protected:
	/// Initialize the pulse output socket
	void initSocket();
	/// Get the simulated angles information from the configuration.
	/// Will also set the _doSimAngles flag.
	SimAngles getSimAngles();
	/// The configuration for CP2Exec
	CP2Config _config;
	/// The piraq dsp's will read the antenna pointing information
	// directly across the pci bus from the PMAC. They need the 
	/// PCI physical address of the PMAC dual ported ram.
	/// @return The pci physical address of the PMAC dual ported ram.
	unsigned int findPMACdpram();
	/// The S band piraq
	CP2PIRAQ* _piraq0;
	/// The Xh piraq
	CP2PIRAQ* _piraq1;
	/// The Xv piraq
	CP2PIRAQ* _piraq2;
	/// The dsp object code file name
	std::string _dspObjFile;
	/// The configuration file name
	std::string _configFile;
	/// The number of pulses per PCI transfer. 
	/// This sized so that each PCI transfer is less than 64KB, 
	///which is the size of the burst FIFO on the piraq which
	/// feeds the PCI transfer. 
	unsigned int _pulsesPerPciXfer;
	/// The output socket
	CP2UdpSocket* _pPulseSocket;
	/// The destination datagram port.
	int _pulsePort;
	/// The destination network.
	QHostAddress _hostAddr;
	/// The current status
	STATUS _status;

	bool _stop;
	/// The number of pulses from piraq1
	int _pulses1;
	/// The number of pulses from piraq2
	int _pulses2;
	/// The number of pulses from piraq3
	int _pulses3;
	// will be set true if an EOF is detected by the
	// CP2PIRAQ since the last time that CP2PIRAQ was
	// queried. (when queried, CP2PIRAQ returns the
	// current state, and clears it's internal flag)
	bool _eofFlags[3];
	/// Set true if angles are to be simulated
	bool _doSimAngles;

};



#endif
