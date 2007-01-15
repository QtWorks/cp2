#ifndef CP2EXECTHREADH_
#define CP2EXECTHREADH_

#include <qthread.h>
#include "CP2PIRAQ.h"
#include "MomentThread.h"
#include "CP2Net.h"

class CP2ExecThread: public QThread {

public:
	CP2ExecThread(int piraqs, CP2PIRAQ* piraq1, CP2PIRAQ* piraq2, CP2PIRAQ* piraq3);
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
	int _piraqs;
	bool _stop;
	int _pulses1;
	int _pulses2;
	int _pulses3;
};



#endif
