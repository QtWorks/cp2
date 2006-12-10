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
// OpsInfo.hh
//
// Mike Dixon, RAP, NCAR
// P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// April 2006
//
///////////////////////////////////////////////////////////////
//
// Stores current ops info
//
////////////////////////////////////////////////////////////////

#ifndef OpsInfo_hh
#define OpsInfo_hh

#include <string>
#include <vector>
#include <cstdio>
#include <iostream>
#include "Params.hh"
using namespace std;

////////////////////////
// This class

class OpsInfo {
  
public:

  // constructor

  OpsInfo (const Params &params);

  // destructor
  
  ~OpsInfo();

  // read in info

  int read(FILE *in);
  
  // printing
  
  void print(ostream &out) const;

  // get methods

  double getStartRange() const { return _startRange; }
  double getGateSpacing() const { return _gateSpacing; }

  int getSweepNum() const { return _iSweep; }
  const string &getSiteName() const { return _sSiteName; }
  double getPulseWidthUs() const { return _fPWidthUSec; }
  double getClockMhz() const { return _fSyClkMHz; }
  double getWavelengthCm() const { return _fWavelengthCM; }
  double getDbz0() const { return _fDBzCalib; }
  double getNoiseDbm0() const { return _fNoiseDBm[0]; }
  double getNoiseDbm1() const { return _fNoiseDBm[1]; }
  double getNoiseSdev0() const { return _fNoiseStdvDB[0]; }
  double getNoiseSdev1() const { return _fNoiseStdvDB[1]; }
  double getSaturationDbm() const { return _fSaturationDBM; }
  double getSaturationMult() const { return _fSaturationMult; }
  double getNoiseDbzAt1km() const { return _fDBzCalib; }

  // polarization type: H, V, Dual_alt, Dual_simul

  const string &getPolarizationType() const
  { return _polarizationType; }

protected:
  
private:

  const Params &_params;

  // RVP8 pulse info fields

  int _iVersion;
  int _iMajorMode;
  int _iPolarization;
  int _iPhaseModSeq;
  int _iSweep;
  int _iAuxNum;
  string _sTaskName;
  string _sSiteName;
  int _iAqMode;
  int _iUnfoldMode;
  int _iPWidthCode;
  double _fPWidthUSec;
  double _fDBzCalib;
  int _iSampleSize;
  int _iMeanAngleSync;
  int _iFlags;
  int _iPlaybackVersion;
  double _fSyClkMHz;
  double _fWavelengthCM;
  double _fSaturationDBM;
  double _fSaturationMult;
  double _fRangeMaskRes;
  int _iRangeMask[512];
  double _fNoiseDBm[2];
  double _fNoiseStdvDB[2];
  double _fNoiseRangeKM;
  double _fNoisePRFHz;
  int _iGparmLatchSts[2];
  int _iGparmImmedSts[6];
  int _iGparmDiagBits[4];
  string _sVersionString;

  // derived from info

  double _startRange;  // km
  double _maxRange;  // km
  double _gateSpacing; // km
  string _polarizationType; // H, V, HV_alternating, HV_simultaneous

  // functions

  int _readPulseInfo(FILE *in);
  void _deriveFromPulseInfo();
  void _printPulseInfo(ostream &out) const;
  void _printDerived(ostream &out) const;

};

#endif

