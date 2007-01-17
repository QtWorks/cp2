#ifndef CP2EXECTHREADH_
#define CP2EXECTHREADH_

#include <string>
#include <qthread.h>
#include <qsocketdevice.h>
#include <qhostaddress.h>

#include "CP2PIRAQ.h"
#include "CP2Net.h"

class CP2ExecThread: public QThread {

public:
	CP2ExecThread(std::string dspObjfile, std::string configFile);
	virtual ~CP2ExecThread();
	void run();
	void stop();
	void pnErrors(int& errors1, int& errors2, int& errors3);
	void pulses(int& pulses1, int& pulses2, int& pulses);
	void rates(double& rate1, double& rate2, double& rate3);

protected:
	/// The piraq dsp's will read the antenna pointing information
	// directly across the pci bus from the PMAC. They need the 
	/// PCI physical address of the PMAC dual ported ram.
	/// @return The pci physical address of the PMAC dual ported ram.
	unsigned int findPMACdpram();
	/// The S band piraq
	CP2PIRAQ* _piraq1;
	/// The Xh piraq
	CP2PIRAQ* _piraq2;
	/// The Xv piraq
	CP2PIRAQ* _piraq3;
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
	QSocketDevice _socketDevice;
	/// The destination datagram port.
	int _outPort;
	/// The destination network.
	QHostAddress _hostAddr;

	bool _stop;
	int _pulses1;
	int _pulses2;
	int _pulses3;
};



#endif
