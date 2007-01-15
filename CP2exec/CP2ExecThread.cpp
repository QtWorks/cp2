#include "CP2ExecThread.h"
#include "CP2PIRAQ.h"
#include "CP2Net.h"

CP2ExecThread::CP2ExecThread(int piraqs, CP2PIRAQ* piraq1, CP2PIRAQ* piraq2, CP2PIRAQ* piraq3):
_piraqs(piraqs),
_piraq1(piraq1),
_piraq2(piraq2),
_piraq3(piraq3),
_pulses1(0),
_pulses2(0),
_pulses3(0),
_stop(false)
{

}

/////////////////////////////////////////////////////////////////////
CP2ExecThread::~CP2ExecThread()
{

}

/////////////////////////////////////////////////////////////////////
void
CP2ExecThread::run()
{
	while(1) { 
		if (_piraqs & 0x01) {
			_pulses1 += _piraq1->poll();
		}
		if (_piraqs & 0x02) {
			_pulses2 += _piraq2->poll();
		}
		if (_piraqs & 0x04) {
			_pulses3 += _piraq3->poll();
		}

		if (_stop)
			break;
	}
}

/////////////////////////////////////////////////////////////////////
void
CP2ExecThread::stop()
{
	_stop = true;
}
/////////////////////////////////////////////////////////////////////
void 
CP2ExecThread::pnErrors(int& errors1, int& errors2, int& errors3)
{
	errors1 = _piraq1->pnErrors();
	errors2 = _piraq2->pnErrors();
	errors3 = _piraq3->pnErrors();

}
/////////////////////////////////////////////////////////////////////
void 
CP2ExecThread::rates(double& rate1, double& rate2, double& rate3)
{
	rate1 = _piraq1->sampleRate();
	rate2 = _piraq2->sampleRate();
	rate3 = _piraq3->sampleRate();
}
/////////////////////////////////////////////////////////////////////
void
CP2ExecThread::pulses(int& pulses1, int& pulses2, int& pulses3) {
	pulses1 = _pulses1;
	pulses2 = _pulses2;
	pulses3 = _pulses3;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
