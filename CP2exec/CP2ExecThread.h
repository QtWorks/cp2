#ifndef CP2EXECTHREADH_
#define CP2EXECTHREADH_

#include <string>
#include <qthread.h>
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
/////////////////////////////////////////////////////////////////////
/// Initialize the windows network interface. closeNetwork() 
/// must be called the same number of times that this routine is
/// called, because WSAstartup() mantains a reference count. See
/// the windows documentation.
/// @param ipName The network name that datagrams will be sent to
/// @param port the destination port.
/// @param sockAddr A sockAddr structure will be initialized here, so that
///  it can be used later for the sendto() call.
/// @return The socke file descriptor, or -1 if failure.
int initNetwork(char* ipName, int port, struct sockaddr_in& sockAddr);
void closeNetwork();

	CP2PIRAQ* _piraq1;
	CP2PIRAQ* _piraq2;
	CP2PIRAQ* _piraq3;
	// The dsp object code file name
	std::string _dspObjFile;
	// The configuration file name
	std::string _configFile;
	unsigned int _packetsPerPciXfer; 
	int _outport;

	bool _stop;
	int _pulses1;
	int _pulses2;
	int _pulses3;
};



#endif
