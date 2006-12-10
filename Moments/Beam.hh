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
// Beam.hh
//
// Beam object
//
// Mike Dixon, RAP, NCAR
// P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// April 2005
//
///////////////////////////////////////////////////////////////

#ifndef Beam_hh
#define Beam_hh

#include <string>
#include <vector>
#include <cstdio>
#include "Params.hh"
#include "Complex.hh"
#include "Pulse.hh"
#include "MomentsMgr.hh"
#include "InterestMap.hh"
#include "GateSpectra.hh"
#include "Fields.hh"
using namespace std;

////////////////////////
// This class

class Beam {
  
public:

  // constructor

  Beam (const string &prog_name,
	const Params &params,
	int max_trips,
	const deque<Pulse *> pulse_queue,
	double az,
	MomentsMgr *momentsMgr);
  
  // destructor
  
  ~Beam();

  // Create interest maps.
  // These are static on the class, and should be created before any
  // beams are constructed.
 
  static int createInterestMaps(const Params &params);
  
  // compute moments
  //
  // Set combinedSpectraFile to NULL to prevent printing.
  //
  // Returns 0 on success, -1 on failure
  
  int computeMoments(int &combinedPrintCount,
		     FILE *combinedSpectraFile);

  int computeMomentsSinglePol(int &combinedPrintCount,
			      FILE *combinedSpectraFile);

  int computeMomentsDualPol(int &combinedPrintCount,
			    FILE *combinedSpectraFile);

  // compute CMD
  
  void computeCmd(const deque<Beam *> beams, int midBeamIndex);

  // filter clutter, using cmd values to decide whether to apply the filter
  
  void filterClutterUsingCmd();

  // set methods

  void setTargetEl(double el) { _targetEl = el; }

  // get methods

  int getNSamples() const { return _nSamples; }
  int get_maxTrips() const { return _maxTrips; }

  double getEl() const { return _el; }
  double getAz() const { return _az; }
  double getTargetEl() const { return _targetEl; }
  double getPrf() const { return _prf; }
  double getPrt() const { return _prt; }
  time_t getTime() const { return _time; }
  double getDoubleTime() const { return _dtime; }

  int getNGatesPulse() const { return _nGatesPulse; }
  int getNGatesOut() const { return _nGatesOut; }

  double getStartRange() const { return _momentsMgr->getStartRange(); }
  double getGateSpacing() const { return _momentsMgr->getGateSpacing(); }

  const MomentsMgr *getMomentsMgr() const { return _momentsMgr; }

  const Fields* getFields() const { return _fields; }

protected:
  
private:

  string _progName;
  const Params &_params;
  int _maxTrips;
  
  int _nSamples;
  int _halfNSamples;
  
  vector<Pulse *> _pulses;
  Complex_t *_delta12;
  Complex_t **_iq;
  Complex_t **_iqh;
  Complex_t **_iqv;
  
  double _el;
  double _az;
  double _targetEl;

  double _prf;
  double _prt;
  
  time_t _time;
  double _dtime;

  int _nGatesPulse;
  int _nGatesOut;

  // manager for computing moments

  MomentsMgr *_momentsMgr;

  // moments fields at each gate

  Fields *_fields;

  // details of spectra at each gate, saved for later use

  vector<GateSpectra *> _gateSpectra;

  // CMD

  bool _cmdVarsReady;

  // interest maps are static on the class, since they should
  // only be computed once

  static InterestMap *_tdbzInterestMap;
  static InterestMap *_sqrtTdbzInterestMap;
  static InterestMap *_spinInterestMap;
  static InterestMap *_cvarDbInterestMap;
  static InterestMap *_cpaInterestMap;
  static InterestMap *_velInterestMap;
  static InterestMap *_widthInterestMap;
  static InterestMap *_velSdevInterestMap;
  static InterestMap *_zdrSdevInterestMap;
  static InterestMap *_rhohvInterestMap;
  static InterestMap *_rhohvSdevInterestMap;
  static InterestMap *_phidpSdevInterestMap;
  static InterestMap *_clutRatioNarrowInterestMap;
  static InterestMap *_clutRatioWideInterestMap;
  static InterestMap *_clutWxPeakSepInterestMap;

  // private functions
  
  void _computeCmdBeamLimits(const deque<Beam *> beams,
			     int midBeamIndex,
			     int &minIndex,
			     int &maxIndex) const;
  
  void _computeCmdVars();

  double _computeAzDiff(double az1, double az2) const;

  double _computeMeanVelocity(double vel1, double vel2, double nyquist);
  
  double _velDiff2Angle(double vel1, double vel2, double nyquist);

  Complex_t _complexProduct(Complex_t c1, Complex_t c2);
  
  Complex_t _meanConjugateProduct(const Complex_t *c1,
				  const Complex_t *c2,
				  int len);

  double _velFromComplexAng(Complex_t cVal, double nyquist);
  
  double _computeInterest(double xx, double x0, double x1);

  void _addSpectrumToFile(FILE *specFile, int count, time_t beamTime,
			  double el, double az, int gateNum,
			  double snr, double vel, double width,
			  int nSamples, const Complex_t *iq);

  static int _convertInterestMapToVector
    (const string &label,
     const Params::interest_map_point_t *map,
     int nPoints,
     vector<InterestMap::ImPoint> &pts);

  void _applyCmdSpeckleFilter();

  void _applyNexradSpikeFilter();
  
  void _filterDregs(double nyquist,
		    double *dbzf,
		    double *velf,
		    double *widthf);
  
  void _performInfilling();

  void _computeInfillTexture();

};

#endif

