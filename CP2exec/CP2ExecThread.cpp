#include "CP2ExecThread.h"
#include "CP2PIRAQ.h"
#include "CP2Net.h"

CP2ExecThread::CP2ExecThread(int piraqs, CP2PIRAQ* piraq1, CP2PIRAQ* piraq2, CP2PIRAQ* piraq3):
_piraqs(piraqs),
_piraq1(piraq1),
_piraq2(piraq2),
_piraq3(piraq3),
_Sbeams(0),
_Xbeams(0),
_collator(6000),
_pulses1(0),
_pulses2(0),
_pulses3(0),
_collatorErrors(0),
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
	std::vector<CP2FullPulse*> SPulses;
	std::vector<CP2FullPulse*> XhPulses;
	std::vector<CP2FullPulse*> XvPulses;

	_pSmomentThread = new MomentThread(Params::DUAL_FAST_ALT, 100);
	_pXmomentThread = new MomentThread(Params::DUAL_CP2_XBAND, 100);

	_pSmomentThread->start();

	_pXmomentThread->start();

	while(1) { 
		if (_piraqs & 0x01) {
			_piraq1->poll(SPulses);
			_pulses1 += SPulses.size();
		}
		if (_piraqs & 0x02) {
			_piraq2->poll(XhPulses);
			_pulses2 += XhPulses.size();
			// send the pulse to the collator. The collator finds matching 
			// pulses. If orphan pulses are detected, they are deleted
			// by the collator. Otherwise, matching pulses returned from
			// the collator can be deleted here.
			for (int i = 0; i < XhPulses.size(); i++) {
				_collator.addPulse(XhPulses[i], 0);
			}
			XhPulses.clear();
		}
		if (_piraqs & 0x04) {
			_piraq3->poll(XvPulses);
			_pulses3 += XvPulses.size();
			// send the pulse to the collator. The collator finds matching 
			// pulses. If orphan pulses are detected, they are deleted
			// by the collator. Otherwise, matching pulses returned from
			// the collator can be deleted here.
			for (int i = 0; i < XvPulses.size(); i++) {
				_collator.addPulse(XvPulses[i], 1);
			}
			XvPulses.clear();
		}

		// now process S band pulses
		for (int i = 0; i < SPulses.size(); i++) {
			_azSband += 1.0/100;
			if (_azSband > 360.0)
				_azSband = 0.0;
			SPulses[i]->header()->antAz = _azSband;
			_pSmomentThread->processPulse(SPulses[i], 0);
			// ask for a completed S band beam. The return value will be
			// 0 if nothing is ready yet.
			Beam* pBeam = _pSmomentThread->getNewBeam();
			if (pBeam) {
				_Sbeams++;
				delete pBeam;
			}
		}
		SPulses.clear();
		// now see if we have some Xband matching pulses
		CP2FullPulse* pHPulse;
		CP2FullPulse* pVPulse;
		while (_collator.gotMatch(&pHPulse, &pVPulse)) {
			_azXband += 1.0/100;
			if (_azXband > 360.0)
				_azXband = 0.0;
			pHPulse->header()->antAz = _azXband;
			pVPulse->header()->antAz = _azXband;

			// a matching pair was found. Send them to the X band
			// moments compute engine.
			_pXmomentThread->processPulse(pHPulse, pVPulse);
			// ask for a completed beam. The return value will be
			// 0 if nothing is ready yet.
			Beam* pBeam = _pXmomentThread->getNewBeam();
			if (pBeam) {
				_Xbeams++;
				delete pBeam;
			}
		}
		_collatorErrors += _collator.discards();

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
CP2ExecThread::beams(int& Sbeams, int&Xbeams)
{

	Sbeams = _Sbeams;
	Xbeams = _Xbeams;

}
/////////////////////////////////////////////////////////////////////
void
CP2ExecThread::collatorErrors(int& errors)
{
	errors = _collatorErrors;
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
