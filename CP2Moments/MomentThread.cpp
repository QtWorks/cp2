#include "MomentThread.h"
#include <iostream>


///////////////////////////////////////////////////////////////////////////////////
MomentThread::MomentThread(Params params):
_params(params)
{
}

///////////////////////////////////////////////////////////////////////////////////
MomentThread::~MomentThread(void)
{
}

///////////////////////////////////////////////////////////////////////////////////
void
MomentThread::run()
{
	// create the moment compute engine for S band
	_momentsEngine = new MomentsEngine(_params);

	while (1) {
		_pulseQueueMutex.lock();
		_queueWait.wait(&_pulseQueueMutex);
		while (_pulseQueue.size() > 0) {
			std::pair<CP2FullPulse*, CP2FullPulse*> pp = _pulseQueue[0];
			CP2FullPulse* p1 = pp.first;
			CP2FullPulse* p2 = pp.second;
			if (p2) {
			_momentsEngine->processPulse(
				p1->data(),
				p2->data(),
				p1->header()->gates,
				1.0e-6,
				p1->header()->el,
				p1->header()->az,
				p1->header()->pulse_num,
				p1->header()->horiz);
			} else {
			_momentsEngine->processPulse(
				p1->data(),
				0,
				p1->header()->gates,
				1.0e-6,
				p1->header()->el,
				p1->header()->az,
				p1->header()->pulse_num,
				p1->header()->horiz);
			}
			_pulseQueue.erase(_pulseQueue.begin());
			delete p1;
			if (p2) {
				delete p2;
			}
			Beam* pBeam = _momentsEngine->getNewBeam();
			if (pBeam)
			{
				_beamQueueMutex.lock();
				_beamQueue.push_back(pBeam);
				_beamQueueMutex.unlock();
			}

		}
		_pulseQueueMutex.unlock();
	}
}

///////////////////////////////////////////////////////////////////////////////////
void 
MomentThread::processPulse(CP2FullPulse* pPulse1, CP2FullPulse* pPulse2)
{
	_pulseQueueMutex.lock();
	_pulseQueue.push_back(std::pair<CP2FullPulse*, CP2FullPulse*>(pPulse1, pPulse2));	
	_pulseQueueMutex.unlock();
	_queueWait.wakeAll();
}


///////////////////////////////////////////////////////////////////////////////////
Beam*
MomentThread::getNewBeam()
{	
	Beam* pBeam = 0;
	_beamQueueMutex.lock();
	if (_beamQueue.size() > 0) {
		pBeam = _beamQueue[0];
		_beamQueue.erase(_beamQueue.begin());
	}
	_beamQueueMutex.unlock();
	return pBeam;
}
///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////

