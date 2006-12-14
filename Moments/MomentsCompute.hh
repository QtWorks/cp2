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
// MomentsCompute.hh
//
// Mike Dixon, RAP, NCAR
// P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// May 2006
//
///////////////////////////////////////////////////////////////

#ifndef MomentsCompute_hh
#define MomentsCompute_hh

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

class MomentsCompute {
  
public:

  // constructor

  MomentsCompute ();

  // destructor
  
  ~MomentsCompute();

  // run 

  int Run();

  // data members

  bool isOK;

protected:
  
private:

  // missing data value

  static const double _missingDbl;

  // basic

  string _progName;
  Params _params;

  // pulse queue

  deque<Pulse *> _pulseQueue;
  int _maxPulseQueueSize;
  long _pulseSeqNum;
  
  // moments computation management

  vector<MomentsMgr *> _momentsMgrArray;
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

  // private functions
  
  void _prepareForMoments(Pulse *pulse);

  bool _beamReady();

  int _computeBeamMoments(Beam *beam);

  void _addPulseToQueue(Pulse *pulse);

  void _addBeamToQueue(Beam *beam);

};

#endif

