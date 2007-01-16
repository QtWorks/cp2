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
	CP2PIRAQ* _piraq1;
	CP2PIRAQ* _piraq2;
	CP2PIRAQ* _piraq3;
	// The dsp object code file name
	std::string _dspObjFile;
	// The configuration file name
	std::string _configFile;

	bool _stop;
	int _pulses1;
	int _pulses2;
	int _pulses3;
};



#endif
