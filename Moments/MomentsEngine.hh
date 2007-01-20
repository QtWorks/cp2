/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
** Copyright UCAR (c) 1992 - 1999
** University Corporation for Atmospheric Research(UCAR)
** National Center for Atmospheric Research(NCAR)
** Research Applications Program(RAP)
** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA
** All rights reserved. Licenced use only.
** Do not copy or distribute without authorization
** 1999/03/14 14:18:54
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
/////////////////////////////////////////////////////////////
// MomentsEngine.hh
//
// Mike Dixon, RAP, NCAR
// P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// May 2006
//
///////////////////////////////////////////////////////////////

#ifndef MomentsEngine_hh
#define MomentsEngine_hh

#include <string>
#include <vector>
#include <deque>
#include <cstdio>
#include "Params.hh"
#include "Pulse.hh"
#include "Beam.hh"
#include "MomentsMgr.hh"
using namespace std;

////////////////////////
// This class

class MomentsEngine {

public:

	// constructor

	MomentsEngine(Params params);

	// destructor

	~MomentsEngine();

        // process a pulse

	int processPulse(
		float* data, 
		float* crossdata,
		int gates, 
		double prt, 
		double el, 
		double az, 
		long long pulseNum,
		bool horizontal);

	// If a new beam is available, return it.
	// @return A pointer to the available beam. If
	// no beam is avaiable, a null is returned. A beam 
	// may be fetched only once. The client must 
	// delete the Beam when finished with it.
	Beam* getNewBeam();

	// data members

	bool isOK;

  // set debugging

  void setDebug();
  void setDebugVerbose();
  void setDebugExtraVerbose();

  // get params

  const Params &getParams() { return _params; }

protected:

	bool _beamReady(double& beamAz);

	int _computeBeamMoments(Beam *beam);

	void _addPulseToQueue(Pulse *pulse);

	// missing data value

	static const double _missingDbl;

	// parameters

	Params _params;

	// pulse queue

	deque<Pulse *> _pulseQueue;
	int _maxPulseQueueSize;
	long long _pulseSeqNum;

	// moments computation management

	MomentsMgr *_momentsMgr;

	double _prevPrfForMoments;

	static const int _maxGates = 4096;
	int _nSamples;

	// beam identification

	int _midIndex1;
	int _midIndex2;
	int _countSinceBeam;

	// beam time and location

	double _time;
	double _az;
	double _el;
	double _prevAz;
	double _prevEl;

	int _nGatesPulse;
	int _nGatesOut;
 
	// The currently computed beam, ready to 
	// to be fetched via getNextBeam(). Once fetched,
	// this will be set to null.
	Beam* _currentBeam;


};

#endif

