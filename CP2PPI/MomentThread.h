#ifndef MOMENTTHREADH_
#define MOMENTTHREADH_

#include <qthread.h>
#include <qmutex.h>
#include <qwaitcondition.h>
#include <vector>

// CP2 timeseries network transfer protocol.
#include "CP2Net.h"

// Clases used in the moments computatons:
#include "MomentsCompute.hh"
#include "MomentsMgr.hh"
#include "Params.hh"
#include "Pulse.hh"
#include "Params.hh"

class MomentThread :
	public QThread
{
public:
	MomentThread(Params::moments_mode_t mode, int nSamples);
	virtual ~MomentThread(void);
	virtual void run();
	void processPulse(CP2FullPulse* pHPulse, CP2FullPulse* pVPulse);

protected:
	std::vector<std::pair<CP2FullPulse*, CP2FullPulse*> > _pulseQueue;
	QMutex _queueMutex;
	QWaitCondition _queueWait;
	MomentsCompute* _momentsCompute;
	Params _params;

};

#endif

