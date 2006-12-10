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
// TsArchive2Dsr.hh
//
// Mike Dixon, RAP, NCAR
// P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// May 2006
//
///////////////////////////////////////////////////////////////

#ifndef TsArchive2Dsr_hh
#define TsArchive2Dsr_hh

#include <string>
#include <vector>
#include <deque>
#include <cstdio>
#include <didss/DsInputPath.hh>
#include <rapformats/DsRadarSweep.hh>
#include "Args.hh"
#include "Params.hh"
#include "OpsInfo.hh"
#include "Pulse.hh"
#include "MomentsMgr.hh"
#include "OutputFmq.hh"
#include "Beam.hh"
using namespace std;

////////////////////////
// This class

class TsArchive2Dsr {
  
public:

  // constructor

  TsArchive2Dsr (int argc, char **argv);

  // destructor
  
  ~TsArchive2Dsr();

  // run 

  int Run();

  // data members

  bool isOK;

protected:
  
private:

  // missing data value

  static const double _missingDbl = -9999.0;

  // basic

  string _progName;
  char *_paramsPath;
  Args _args;
  Params _params;

  // input data

  DsInputPath *_input;

  // output fmq

  OutputFmq *_fmq;

  // pulse info

  OpsInfo *_opsInfo;

  // pulse queue

  deque<Pulse *> _pulseQueue;
  int _maxPulseQueueSize;
  long _pulseSeqNum;
  
  // moments computation management

  vector<MomentsMgr *> _momentsMgrArray;
  MomentsMgr *_momentsMgr;
  double _prevPrfForMoments;
  
  // SZ
  
  static const int _maxTrips = 5;
  static const int _maxBins = 4096;
  static const int _maxGates = _maxBins * _maxTrips;
  static const int _nSamplesSz = 64;
  int _nSamples;

  // beam identification

  int _midIndex1;
  int _midIndex2;
  int _countSinceBeam;
  
  // beam time and location
  
  time_t _time;
  double _az;
  double _el;
  double _prevAz;
  double _prevEl;
  
  int _nGatesPulse;
  int _nGatesOut;

  // send params to fmq
  
  double _prevPrfForParams;
  int _prevNGatesForParams;
  bool _paramsSentThisFile;

  // volume identification
  
  int _volNum;
  int _tiltNum;
  double _volMinEl;
  double _volMaxEl;
  int _nBeamsThisVol;

  // sweep info from SPDB

  bool _guessingSweepInfo;
  bool _currentSweepValid;
  DsRadarSweep _currentSweep;
  double _prevSweepEndTime;

  // beam queue
  
  deque<Beam *> _beamQueue;
  int _maxBeamQueueSize;
  int _midBeamIndex;
  
  // debug printing 
  
  int _specPrintCount;
  string _spectraFilePath;
  FILE *_spectraFile;
  
  // CMD
  
  bool _applyCmd;
  int _cmdKernelWidth;
  
  // private functions
  
  int _processFile(const char *input_path);

  void _prepareForMoments(Pulse *pulse);

  bool _beamReady();

  int _computeBeamMomentsBasic(Beam *beam);

  void _addPulseToQueue(Pulse *pulse);

  void _addBeamToQueue(Beam *beam);

  void _setSweepInfo(Beam *beam, bool &endOfVol);

  int _readSweepInfo(Beam *beam, int &volNum, int &tiltNum);
  
  bool _beamWithinSweep(const Beam *beam, const DsRadarSweep &sweep);

};

#endif

