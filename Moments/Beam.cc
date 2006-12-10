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
///////////////////////////////////////////////////////////////
// Beam.cc
//
// Beam object
//
// Mike Dixon, RAP, NCAR, P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// April 2005
//
///////////////////////////////////////////////////////////////
//
// Beam object holds time series and moment data.
//
////////////////////////////////////////////////////////////////

#include <iostream>
#include <iomanip>
#include <cmath>
#include <toolsa/DateTime.hh>
#include <toolsa/toolsa_macros.h>
#include "Beam.hh"
using namespace std;

InterestMap *Beam::_tdbzInterestMap = NULL;
InterestMap *Beam::_sqrtTdbzInterestMap = NULL;
InterestMap *Beam::_spinInterestMap = NULL;
InterestMap *Beam::_cvarDbInterestMap = NULL;
InterestMap *Beam::_cpaInterestMap = NULL;
InterestMap *Beam::_velInterestMap = NULL;
InterestMap *Beam::_widthInterestMap = NULL;
InterestMap *Beam::_velSdevInterestMap = NULL;
InterestMap *Beam::_zdrSdevInterestMap = NULL;
InterestMap *Beam::_rhohvInterestMap = NULL;
InterestMap *Beam::_rhohvSdevInterestMap = NULL;
InterestMap *Beam::_phidpSdevInterestMap = NULL;
InterestMap *Beam::_clutRatioNarrowInterestMap = NULL;
InterestMap *Beam::_clutRatioWideInterestMap = NULL;
InterestMap *Beam::_clutWxPeakSepInterestMap = NULL;

////////////////////////////////////////////////////
// Constructor

Beam::Beam(const string &prog_name,
	   const Params &params,
	   int max_trips,
	   const deque<Pulse *> pulse_queue,
	   double az,
	   MomentsMgr *momentsMgr) :
  _progName(prog_name),
  _params(params),
  _maxTrips(max_trips),
  _az(az),
  _momentsMgr(momentsMgr)

{

  _nSamples = _momentsMgr->getNSamples();

  // load up pulse vector

  int offset = 0;
  if (_momentsMgr->getDualPol()) {
    // for dual pol data, we check the HV flag to determine whether pulse 0
    // is horizontally or vertically polarized. If vertical, go back by one to
    // start with horizontal
    const Pulse *pulse = pulse_queue[_nSamples-1];
    if (!pulse->isHoriz()) {
      offset = 1;
    }
  }
    
  for (int ii = 0; ii < _nSamples; ii++) {
    int jj = _nSamples - ii - 1 + offset;
    Pulse *pulse = pulse_queue[jj];
    _pulses.push_back(pulse);
    pulse->addClient("Beam::Beam");
  }
  
  // initialize
  
  _halfNSamples = _nSamples / 2;
  _delta12 = NULL;
  _iq = NULL;
  _iqh = NULL;
  _iqv = NULL;

  // compute mean prt

  double sumPrt = 0.0;
  for (int ii = 0; ii < _nSamples; ii++) {
    sumPrt += _pulses[ii]->getPrt();
  }
  _prt = sumPrt / _nSamples;
  _prf = 1.0 / _prt;

  // set time

  _time = (time_t) (_pulses[_halfNSamples]->getTime() + 0.5);
  _dtime = _pulses[_halfNSamples]->getTime();

  // set elevation

  _el = _pulses[_halfNSamples]->getEl();
  if (_el > 180.0) {
    _el -= 360.0;
  }
  _targetEl = _el;

  // compute number of output gates

  _nGatesPulse = _pulses[_halfNSamples]->getNGates();
  if (_momentsMgr->getApplySz() && !_momentsMgr->getDualPol()) {
    _nGatesOut = _nGatesPulse * 2;
  } else {
    _nGatesOut = _nGatesPulse;
  }

  // fields at each gate

  _fields = new Fields[_nGatesOut];

  // gate spectra are saved so that they do not
  // need to be computed more than once
  
  for (int ii = 0; ii < _nGatesPulse; ii++) {
    GateSpectra *spec = new GateSpectra(_nSamples);
    _gateSpectra.push_back(spec);
  }

  // interest maps if needed

  if (_params.apply_cmd) {
    createInterestMaps(_params);
  }
  _cmdVarsReady = false;
  
}

//////////////////////////////////////////////////////////////////
// destructor

Beam::~Beam()

{

  for (int ii = 0; ii < (int) _pulses.size(); ii++) {
    if (_pulses[ii]->removeClient("Beam::~Beam") == 0) {
      delete _pulses[ii];
    }
  }
  _pulses.clear();

  if (_delta12) {
    delete[] _delta12;
  }

  if (_iq) {
    ufree2((void **) _iq);
  }

  if (_iqh) {
    ufree2((void **) _iqh);
  }

  if (_iqv) {
    ufree2((void **) _iqv);
  }

  if (_fields) {
    delete[] _fields;
  }

  for (int ii = 0; ii < _nGatesPulse; ii++) {
    delete _gateSpectra[ii];
  }

}

/////////////////////////////////////////////////////////
// create interest maps
//
// These are static on the class, so are only created once.

int Beam::createInterestMaps(const Params &params)

{  

  // TDBZ

  if (_tdbzInterestMap == NULL) {
    vector<InterestMap::ImPoint> pts;
    if (_convertInterestMapToVector("Tdbz",
				    params._tdbz_interest_map,
				    params.tdbz_interest_map_n,
				    pts)) {
      return -1;
    }
    _tdbzInterestMap =
      new InterestMap("DbzTexture", pts,
		      params.tdbz_interest_weight);
    if (params.debug >= Params::DEBUG_VERBOSE) {
      cerr << "-->> Creating _tdbzInterestMap" << endl;
    }
  } // if (_tdbzInterestMap ...

  // sqrt TDBZ
  
  if (_sqrtTdbzInterestMap == NULL) {
    vector<InterestMap::ImPoint> pts;
    if (_convertInterestMapToVector("SqrtTdbz",
				    params._sqrt_tdbz_interest_map,
				    params.sqrt_tdbz_interest_map_n,
				    pts)) {
      return -1;
    }
    _sqrtTdbzInterestMap =
      new InterestMap("SqrtTdbz", pts,
		      params.sqrt_tdbz_interest_weight);
    if (params.debug >= Params::DEBUG_VERBOSE) {
      cerr << "-->> Creating _sqrtTdbzInterestMap" << endl;
    }
  } // if (_tdbzInterestMap ...

  // spin
  
  if (_spinInterestMap == NULL) {
    vector<InterestMap::ImPoint> pts;
    if (_convertInterestMapToVector("Spin",
				    params._spin_interest_map,
				    params.spin_interest_map_n,
				    pts)) {
      return -1;
    }
    _spinInterestMap =
      new InterestMap("Spin", pts,
		      params.spin_interest_weight);
    if (params.debug >= Params::DEBUG_VERBOSE) {
      cerr << "-->> Creating _spinInterestMap" << endl;
    }
  } // if (_spinInterestMap ...

  // CVAR of power in spectrum
  
  if (_cvarDbInterestMap == NULL) {
    vector<InterestMap::ImPoint> pts;
    if (_convertInterestMapToVector("CvarDb",
				    params._cvar_db_interest_map,
				    params.cvar_db_interest_map_n,
				    pts)) {
      return -1;
    }
    _cvarDbInterestMap =
      new InterestMap("CvarDb", pts,
		      params.cvar_db_interest_weight);
    if (params.debug >= Params::DEBUG_VERBOSE) {
      cerr << "-->> Creating _cvarDbInterestMap" << endl;
    }
  } // if (_cvarDbInterestMap ...

  // clutter phase alignment
  
  if (_cpaInterestMap == NULL) {
    vector<InterestMap::ImPoint> pts;
    if (_convertInterestMapToVector("cpa",
				    params._cpa_interest_map,
				    params.cpa_interest_map_n,
				    pts)) {
      return -1;
    }
    _cpaInterestMap =
      new InterestMap("cpa", pts,
		      params.cpa_interest_weight);
    if (params.debug >= Params::DEBUG_VERBOSE) {
      cerr << "-->> Creating _cvarDbInterestMap" << endl;
    }
  } // if (_cpaInterestMap ...

  // velocity

  if (_velInterestMap == NULL) {
    vector<InterestMap::ImPoint> pts;
    if (_convertInterestMapToVector("velocity",
				    params._vel_interest_map,
				    params.vel_interest_map_n,
				    pts)) {
      return -1;
    }
    _velInterestMap =
      new InterestMap("velocity", pts, params.vel_interest_weight);
    if (params.debug >= Params::DEBUG_VERBOSE) {
      cerr << "-->> Creating _velInterestMap" << endl;
    }
  } // if (_velInterestMap ...

  // width

  if (_widthInterestMap == NULL) {
    vector<InterestMap::ImPoint> pts;
    if (_convertInterestMapToVector("spectrum width",
				    params._width_interest_map,
				    params.width_interest_map_n,
				    pts)) {
      return -1;
    }
    _widthInterestMap =
      new InterestMap("spectrum width", pts, params.width_interest_weight);
    if (params.debug >= Params::DEBUG_VERBOSE) {
      cerr << "-->> Creating _widthInterestMap" << endl;
    }
  } // if (_widthInterestMap ...

  // sdev of velocity

  if (_velSdevInterestMap == NULL) {
    vector<InterestMap::ImPoint> pts;
    if (_convertInterestMapToVector("velocity sdev",
				    params._vel_sdev_interest_map,
				    params.vel_sdev_interest_map_n,
				    pts)) {
      return -1;
    }
    _velSdevInterestMap =
      new InterestMap("velocity sdev", pts, params.vel_sdev_interest_weight);
    if (params.debug >= Params::DEBUG_VERBOSE) {
      cerr << "-->> Creating _velSdevInterestMap" << endl;
    }
  } // if (_velSdevInterestMap ...

  // sdev of zdr

  if (_zdrSdevInterestMap == NULL) {
    vector<InterestMap::ImPoint> pts;
    if (_convertInterestMapToVector("zdr sdev",
				    params._zdr_sdev_interest_map,
				    params.zdr_sdev_interest_map_n,
				    pts)) {
      return -1;
    }
    _zdrSdevInterestMap =
      new InterestMap("zdr sdev", pts, params.zdr_sdev_interest_weight);
    if (params.debug >= Params::DEBUG_VERBOSE) {
      cerr << "-->> Creating _zdrSdevInterestMap" << endl;
    }
  } // if (_zdrSdevInterestMap ...

  // rhohv

  if (_rhohvInterestMap == NULL) {
    vector<InterestMap::ImPoint> pts;
    if (_convertInterestMapToVector("rhohv",
				    params._rhohv_interest_map,
				    params.rhohv_interest_map_n,
				    pts)) {
      return -1;
    }
    _rhohvInterestMap =
      new InterestMap("rhohv", pts, params.rhohv_interest_weight);
    if (params.debug >= Params::DEBUG_VERBOSE) {
      cerr << "-->> Creating _rhohvInterestMap" << endl;
    }
  } // if (_rhohvInterestMap ...

  // sdev of rhohv

  if (_rhohvSdevInterestMap == NULL) {
    vector<InterestMap::ImPoint> pts;
    if (_convertInterestMapToVector("rhohv sdev",
				    params._rhohv_sdev_interest_map,
				    params.rhohv_sdev_interest_map_n,
				    pts)) {
      return -1;
    }
    _rhohvSdevInterestMap =
      new InterestMap("rhohv sdev", pts, params.rhohv_sdev_interest_weight);
    if (params.debug >= Params::DEBUG_VERBOSE) {
      cerr << "-->> Creating _rhohvSdevInterestMap" << endl;
    }
  } // if (_rhohvSdevInterestMap ...

  // sdev of phidp

  if (_phidpSdevInterestMap == NULL) {
    vector<InterestMap::ImPoint> pts;
    if (_convertInterestMapToVector("phidp sdev",
				    params._phidp_sdev_interest_map,
				    params.phidp_sdev_interest_map_n,
				    pts)) {
      return -1;
    }
    _phidpSdevInterestMap =
      new InterestMap("phidp sdev", pts, params.phidp_sdev_interest_weight);
    if (params.debug >= Params::DEBUG_VERBOSE) {
      cerr << "-->> Creating _phidpSdevInterestMap" << endl;
    }
  } // if (_phidpSdevInterestMap ...

  // clutter probability fields

  if (_clutRatioNarrowInterestMap == NULL) {
    vector<InterestMap::ImPoint> pts;
    if (_convertInterestMapToVector("clutter ratio narrow",
				    params._clut_ratio_narrow_interest_map,
				    params.clut_ratio_narrow_interest_map_n,
				    pts)) {
      return -1;
    }
    _clutRatioNarrowInterestMap =
      new InterestMap("clutter ration narrow",
                      pts, params.clut_ratio_narrow_interest_weight);
    if (params.debug >= Params::DEBUG_VERBOSE) {
      cerr << "-->> Creating _clutRatioNarrowInterestMap" << endl;
    }
  } // if (_clutRatioNarrowInterestMap ...

  if (_clutRatioWideInterestMap == NULL) {
    vector<InterestMap::ImPoint> pts;
    if (_convertInterestMapToVector("clutter ratio wide",
				    params._clut_ratio_wide_interest_map,
				    params.clut_ratio_wide_interest_map_n,
				    pts)) {
      return -1;
    }
    _clutRatioWideInterestMap =
      new InterestMap("clutter ratio wide",
                      pts, params.clut_ratio_wide_interest_weight);
    if (params.debug >= Params::DEBUG_VERBOSE) {
      cerr << "-->> Creating _clutRatioWideInterestMap" << endl;
    }
  } // if (_clutRatioWideInterestMap ...

  if (_clutWxPeakSepInterestMap == NULL) {
    vector<InterestMap::ImPoint> pts;
    if (_convertInterestMapToVector("clut-wx-peak sep",
				    params._clut_wx_peak_sep_interest_map,
				    params.clut_wx_peak_sep_interest_map_n,
				    pts)) {
      return -1;
    }
    _clutWxPeakSepInterestMap =
      new InterestMap("clut-wx-peak sep",
                      pts, params.clut_wx_peak_sep_interest_weight);
    if (params.debug >= Params::DEBUG_VERBOSE) {
      cerr << "-->> Creating _clutWxPeakSepInterestMap" << endl;
    }
  } // if (_clutWxPeakSepInterestMap ...
  
  return 0;
  
}

/////////////////////////////////////////////////
// compute moments
//
// Returns 0 on success, -1 on failure
    
int Beam::computeMoments(int &combinedPrintCount,
			 FILE *combinedSpectraFile)
  
{

  int iret = 0;

  if (_momentsMgr->getDualPol()) {
    
    if (computeMomentsDualPol(combinedPrintCount, combinedSpectraFile)) {
      iret = -1;
    }

  } else {

    if (computeMomentsSinglePol(combinedPrintCount, combinedSpectraFile)) {
      iret = -1;
    }

  }

  _cmdVarsReady = false;

  return iret;

}

/////////////////////////////////////////////////
// compute moments - single pol
//
// Returns 0 on success, -1 on failure
    
int Beam::computeMomentsSinglePol(int &combinedPrintCount,
				  FILE *combinedSpectraFile)
  
{

  // compute delta phases for SZ, if required
  
  if (_momentsMgr->getApplySz()) {
    _delta12 = new Complex_t[_nSamples];
    for (int i = 0; i < _nSamples; i++) {
      _delta12[i].re = cos(_pulses[i]->getPhaseDiff0() * DEG_TO_RAD);
      _delta12[i].im = -1.0 * sin(_pulses[i]->getPhaseDiff0() * DEG_TO_RAD);
    }
  } // if (_applySz) 
  
  // get unpacked pulses
  
  const fl32* iqData[_nSamples];
  for (int ii = 0; ii < _nSamples; ii++) {
    iqData[ii] = _pulses[ii]->getIq();
  }
  
  // load up IQ data array
  
  if (_iq) {
    ufree2((void **) _iq);
  }
  _iq = (Complex_t **) ucalloc2(_nGatesPulse, _nSamples, sizeof(Complex_t));
  int posn = 0;
  for (int igate = 0; igate < _nGatesPulse; igate++, posn += 2) {
    Complex_t *iq = _iq[igate];
    for (int isamp = 0; isamp < _nSamples; isamp++, iq++) {
      iq->re = iqData[isamp][posn];
      iq->im = iqData[isamp][posn + 1];
    } // isamp
  } // igate
  
  if (_momentsMgr->getApplySz()) {
    _momentsMgr->computeMomentsSz(_time, _el, _az, _prt,
				  _nGatesPulse, _iq, _delta12,
				  combinedPrintCount, combinedSpectraFile,
				  _fields,
				  _gateSpectra);
    _performInfilling();
  } else {
    _momentsMgr->computeMoments(_time, _el, _az, _prt,
				_nGatesPulse, _iq,
				combinedPrintCount, combinedSpectraFile,
                                _fields,
				_gateSpectra);
  }
  
  return 0;

}

/////////////////////////////////////////////////
// compute moments - dual pol case
//
// Returns 0 on success, -1 on failure
    
int Beam::computeMomentsDualPol(int &combinedPrintCount,
				FILE *combinedSpectraFile)
  
{

  // set up data pointer array
  
  const fl32* iqData[_nSamples];
  for (int ii = 0; ii < _nSamples; ii++) {
    iqData[ii] = _pulses[ii]->getIq();
  }
  
  // load up IQ data array
  
  if (_iq) {
    ufree2((void **) _iq);
  }
  _iq = (Complex_t **) ucalloc2(_nGatesPulse, _nSamples, sizeof(Complex_t));
  for (int igate = 0, posn = 0; igate < _nGatesPulse; igate++, posn += 2) {
    Complex_t *iq = _iq[igate];
    for (int isamp = 0; isamp < _nSamples; isamp++, iq++) {
      iq->re = iqData[isamp][posn];
      iq->im = iqData[isamp][posn + 1];
    } // isamp
  } // igate
  
  // load up IQH and IQV data arrays
  
  int nDual = _nSamples / 2;
  if (_iqh) {
    ufree2((void **) _iqh);
  }
  if (_iqv) {
    ufree2((void **) _iqv);
  }
  _iqh = (Complex_t **) ucalloc2(_nGatesPulse, nDual, sizeof(Complex_t));
  _iqv = (Complex_t **) ucalloc2(_nGatesPulse, nDual, sizeof(Complex_t));
  for (int igate = 0, posn = 0; igate < _nGatesPulse; igate++, posn += 2) {
    Complex_t *iqh = _iqh[igate];
    Complex_t *iqv = _iqv[igate];
    for (int isamp = 0; isamp < _nSamples; iqh++, iqv++) {
      iqh->re = iqData[isamp][posn];
      iqh->im = iqData[isamp][posn + 1];
      isamp++;
      iqv->re = iqData[isamp][posn];
      iqv->im = iqData[isamp][posn + 1];
      isamp++;
    } // isamp
  } // igate

  _momentsMgr->computeDualAlt(_time, _el, _az, _prt,
                              _nGatesPulse, _iq, _iqh, _iqv,
                              combinedPrintCount, combinedSpectraFile,
                              _fields,
                              _gateSpectra);
  
  return 0;

}

/////////////////////////////////////////////////
// compute CMD

void Beam::computeCmd(const deque<Beam *> beams,
		      int midBeamIndex)
  
{
  
  const Beam *midBeam = beams[midBeamIndex];
  
  // vel and width

  for (int igate = 0; igate < _nGatesOut; igate++) {
    _fields[igate].cmd_vel = beams[midBeamIndex]->_fields[igate].vel;
    _fields[igate].cmd_width = beams[midBeamIndex]->_fields[igate].width;
  }

  // set limits for CMD computations by checking the surrounding beams
  
  int minBeamIndex, maxBeamIndex;
  _computeCmdBeamLimits(beams, midBeamIndex, minBeamIndex, maxBeamIndex);
  
  // make sure all beams have computed the cmd variables

  for (int ii = minBeamIndex; ii <= maxBeamIndex; ii++) {
    beams[ii]->_computeCmdVars();
  }

  // compute number of gates in kernel, making sure there is an odd number

  int nGatesKernel = _params.cmd_kernel_ngates;
  int nGatesHalf = nGatesKernel / 2;

  // set up gate limits

  vector<int> startGate;
  vector<int> endGate;
  for (int igate = 0; igate < _nGatesOut; igate++) {
    int start = igate - nGatesHalf;
    if (start < 0) {
      start = 0;
    }
    startGate.push_back(start);
    int end = igate + nGatesHalf;
    if (end > _nGatesOut - 1) {
      end = _nGatesOut - 1;
    }
    endGate.push_back(end);
  } // igate
  
  // tdbz and spin
  // are based on spatial variation in range and azimuth

  for (int igate = 0; igate < _nGatesOut; igate++) {
    
    // compute sums etc. for stats over the kernel space
    
    double nTexture = 0.0;
    double sumDbzDiffSq = 0.0;
    
    double nSpinChange = 0.0;
    double nSpinTotal = 0.0;
    double sumSpinDbz = 0.0;
    
    for (int ibeam = minBeamIndex; ibeam <= maxBeamIndex; ibeam++) {
      
      const Fields *fields = beams[ibeam]->_fields + startGate[igate];
      for (int jj = startGate[igate]; jj <= endGate[igate]; jj++, fields++) {
        
	// dbz texture

	double dds = fields->cmd_dbz_diff_sq;
	if (dds != Fields::missingDouble) {
	  sumDbzDiffSq += dds;
	  nTexture++;
	}

	// spin
        
	double dsc = fields->cmd_spin_change;
        double dbz = fields->dbz;
	if (dsc != Fields::missingDouble &&
            dbz != Fields::missingDouble) {
	  nSpinChange += dsc;
          sumSpinDbz += dbz;
	  nSpinTotal++;
	}

      } // jj
      
    } // ibeam
    
    // texture
    
    if (nTexture > 0) {
      double texture = sumDbzDiffSq / nTexture;
      _fields[igate].cmd_tdbz = texture;
      _fields[igate].cmd_sqrt_tdbz = sqrt(texture);
    }

    // spin
    
    if (nSpinTotal > 0) {
      _fields[igate].cmd_spin = (nSpinChange / nSpinTotal) * 100.0;
    }

  } // igate

  // vel sdev computed in range only

  for (int igate = 0; igate < _nGatesOut; igate++) {

    // compute sums etc. for stats over the kernel space

    double nVel = 0.0;
    double sumVel = 0.0;
    double sumVelSq = 0.0;
    
    const Fields *fields = midBeam->_fields + startGate[igate];
    for (int jj = startGate[igate]; jj <= endGate[igate]; jj++, fields++) {

      double vv = fields->vel;
      if (vv != Fields::missingDouble) {
        sumVel += vv;
        sumVelSq += (vv * vv);
        nVel++;
      }
      
    } // jj
    
    // vel sdev
    
    if (nVel > 0) {
      double meanVel = sumVel / nVel;
      if (nVel > 2) {
	double term1 = sumVelSq / nVel;
	double term2 = meanVel * meanVel;
	if (term1 >= term2) {
	  _fields[igate].cmd_vel_sdev = sqrt(term1 - term2);
	}
      }
    }

  } // igate

#ifdef JUNK

  // test computed in range only
  
  for (int igate = 0; igate < _nGatesOut; igate++) {

    // compute sums etc. for stats over the kernel space

    double maxDbz = -100.0;
    double minDbz = 100.0;
    double sumDbz = 0.0;
    double nn = 0.0;
    vector<double> vals;
    
    const Fields *fields = midBeam->_fields + startGate[igate];
    for (int jj = startGate[igate]; jj <= endGate[igate]; jj++, fields++) {
      double dbz = fields->dbz;
      if (dbz != Fields::missingDouble) {
        maxDbz = MAX(maxDbz, dbz);
        minDbz = MIN(minDbz, dbz);
        sumDbz += dbz;
        nn++;
        vals.push_back(dbz);
      }
    } // jj
    
    double meanDbz = 0.0;
    if (nn > 0) {
      meanDbz = sumDbz / nn;
    }

    sort(vals.begin(), vals.end());
    double diff = 0.0;
    if (vals.size() > 2) {
      diff = vals[1] - vals[0];
    }

    double test = 0.0;
    if (maxDbz != minDbz) {
      // test = ((meanDbz - minDbz) / (maxDbz - minDbz)) * 100.0;
      test = (diff / (maxDbz - minDbz)) * 100.0;
    }
    _fields[igate].test = test;

  } // igate

#endif

  // dual pol fields

  if (_momentsMgr->getDualPol()) {

    for (int igate = 0; igate < _nGatesOut; igate++) {

      // compute sums etc. for stats over the kernel space
      
      double nZdr = 0.0;
      double sumZdr = 0.0;
      double sumZdrSq = 0.0;
      
      double nRhohv = 0.0;
      double sumRhohv = 0.0;
      double sumRhohvSq = 0.0;
      
      double nPhidp = 0.0;
      double sumPhidp = 0.0;
      double sumPhidpSq = 0.0;
      
      const Beam *beam = beams[midBeamIndex];
      const Fields *fields = beam->_fields;
      
      for (int jj = startGate[igate]; jj <= endGate[igate]; jj++, fields++) {
	
	// zdr
	
	double zz = fields->zdr;
	if (zz != Fields::missingDouble) {
	  sumZdr += zz;
	  sumZdrSq += (zz * zz);
	  nZdr++;
	}

	// rhohv
	
	double rr = fields->rhohv;
	if (rr != Fields::missingDouble) {
	  sumRhohv += rr;
	  sumRhohvSq += (rr * rr);
	  nRhohv++;
	}
	
	// phidp
	
	double ph = fields->phidp;
	if (ph != Fields::missingDouble) {
	  sumPhidp += ph;
	  sumPhidpSq += (ph * ph);
	  nPhidp++;
	}
	
      } // jj
      
      // zdr
      
      if (nZdr > 0) {
	double meanZdr = sumZdr / nZdr;
	if (nZdr > 2) {
	  double term1 = sumZdrSq / nZdr;
	  double term2 = meanZdr * meanZdr;
	  if (term1 >= term2) {
	    _fields[igate].cmd_zdr_sdev = sqrt(term1 - term2);
	  }
	}
      }
      
      // rhohv
      
      if (nRhohv > 0) {
	double meanRhohv = sumRhohv / nRhohv;
	if (nRhohv > 2) {
	  double term1 = sumRhohvSq / nRhohv;
	  double term2 = meanRhohv * meanRhohv;
	  if (term1 >= term2) {
	    _fields[igate].cmd_rhohv_sdev = sqrt(term1 - term2);
	  }
	}
      }
      
      // phidp
      
      if (nPhidp > 0) {
	double meanPhidp = sumPhidp / nPhidp;
	if (nPhidp > 2) {
	  double term1 = sumPhidpSq / nPhidp;
	  double term2 = meanPhidp * meanPhidp;
	  if (term1 >= term2) {
	    _fields[igate].cmd_phidp_sdev = sqrt(term1 - term2);
	  }
	}
      }
      
    } // igate
    
  } // if (dualPol ...
  
  // compute cmd clutter field

  Fields *fields = _fields;
  double probThreshold = _params.cmd_threshold_for_clutter;
  double ratioNarrowThreshold = _params.min_clutter_ratio_narrow;
  double cmdSnrThreshold = _params.cmd_snr_threshold;
  
  for (int igate = 0; igate < _nGatesOut; igate++, fields++) {
    
    if (fields->dbz == Fields::missingDouble) {
      continue;
    }
    if (fields->snr == Fields::missingDouble ||
        fields->snr < cmdSnrThreshold) {
      continue;
    }

    double sumInterest = 0.0;
    double sumWeights = 0.0;
    
    _tdbzInterestMap->accumInterest(fields->cmd_tdbz,
                                    sumInterest, sumWeights);
    
    _sqrtTdbzInterestMap->accumInterest(fields->cmd_sqrt_tdbz,
                                        sumInterest, sumWeights);
    
    _spinInterestMap->accumInterest(fields->cmd_spin,
                                    sumInterest, sumWeights);
    
    _cvarDbInterestMap->accumInterest(fields->cmd_cvar_db,
                                      sumInterest, sumWeights);
    
    _cpaInterestMap->accumInterest(fields->cpa,
                                   sumInterest, sumWeights);
    
    _velInterestMap->accumInterest(fields->cmd_vel,
                                   sumInterest, sumWeights);
    
    _widthInterestMap->accumInterest(fields->cmd_width,
                                     sumInterest, sumWeights);
    
    _velSdevInterestMap->accumInterest(fields->cmd_vel_sdev,
                                       sumInterest, sumWeights);

    _zdrSdevInterestMap->accumInterest(fields->cmd_zdr_sdev,
                                       sumInterest, sumWeights);

    _rhohvInterestMap->accumInterest(fields->rhohv,
                                     sumInterest, sumWeights);
    
    _rhohvSdevInterestMap->accumInterest(fields->cmd_rhohv_sdev,
                                         sumInterest, sumWeights);

    _phidpSdevInterestMap->accumInterest(fields->cmd_phidp_sdev,
                                         sumInterest, sumWeights);
    
    _clutRatioNarrowInterestMap->accumInterest(fields->cmd_ratio_narrow,
                                               sumInterest, sumWeights);

    _clutRatioWideInterestMap->accumInterest(fields->cmd_ratio_wide,
                                             sumInterest, sumWeights);

    _clutWxPeakSepInterestMap->accumInterest(fields->cmd_wx2peak_sep,
                                             sumInterest, sumWeights);

    fields->cmd = sumInterest / sumWeights;

    if (fields->cmd >= probThreshold &&
        fields->cmd_ratio_narrow >= ratioNarrowThreshold) {
      fields->cmd_flag = 1;
    } else {
      fields->cmd_flag = 0;
    }

  } // igate

  if (_params.apply_cmd_speckle_filter) {
    _applyCmdSpeckleFilter();
  }

}

/////////////////////////////////////////////////////////////////////////
// filter clutter, using cmd values to decide whether to apply the filter
    
void Beam::filterClutterUsingCmd()
  
{

  // compute filtered moments
  
  if (_momentsMgr->getDualPol()) {
    
    _momentsMgr->filterClutterDualAlt(_prt, _nGatesPulse,
                                      _iq, _iqh, _iqv,
                                      _fields, _gateSpectra);
    
  } else {
    
    if (_momentsMgr->getApplySz()) {
      _momentsMgr->filterClutterSz(_prt, _nGatesPulse, _delta12,
                                   _fields, _gateSpectra);
    } else {
      _momentsMgr->filterClutter(_prt, _nGatesPulse, _iq,
                                 _fields, _gateSpectra);
    }
    
  }

  if (_params.apply_nexrad_spike_filter_after_cmd) {
    _applyNexradSpikeFilter();
  }


}

/////////////////////////////////////////////////////////
// compute CMD beam limits by checking surrounding beams

void Beam::_computeCmdBeamLimits(const deque<Beam *> beams,
				 int midBeamIndex,
				 int &minBeamIndex,
				 int &maxBeamIndex) const
  
{
  
  minBeamIndex = midBeamIndex;
  maxBeamIndex = midBeamIndex;
  
  double midEl = getEl();
  double maxElDiff = 0.2;
  
  double prevAz = getAz();
  double maxAzDiff = _params.azimuth_resolution * 2.0;
  for (int ii = midBeamIndex - 1; ii >= 0; ii--) {
    const Beam *bb = beams[ii];
    if (bb->getMomentsMgr() != getMomentsMgr() ||
	bb->getNGatesOut() != getNGatesOut()) {
      break;
    }
    double az = bb->getAz();
    if (fabs(_computeAzDiff(az, prevAz)) > maxAzDiff) {
      break;
    }
    double el = bb->getEl();
    if (fabs(el - midEl) > maxElDiff) {
      break;
    }
    minBeamIndex = ii;
    prevAz = az;
  }
  
  prevAz = getAz();
  for (int ii = midBeamIndex + 1; ii < (int) beams.size(); ii++) {
    const Beam *bb = beams[ii];
    if (bb->getMomentsMgr() != getMomentsMgr() ||
	bb->getNGatesOut() != getNGatesOut()) {
      break;
    }
    double az = bb->getAz();
    if (fabs(_computeAzDiff(az, prevAz)) > maxAzDiff) {
      break;
    }
    double el = bb->getEl();
    if (fabs(el - midEl) > maxElDiff) {
      break;
    }
    maxBeamIndex = ii;
    prevAz = az;
  }

  if (_params.debug >= Params::DEBUG_VERBOSE) {
    cerr << "CMD, el, az, minBeamIndex, maxBeamIndex: "
	 << getEl() << ", "
	 << getAz() << ", "
	 << minBeamIndex << ", " << maxBeamIndex << endl;
  }
    
}

/////////////////////////////////////////////////
// compute CMD variables
//
// Assumes moments have been computed
    
void Beam::_computeCmdVars()
  
{

  if (_cmdVarsReady) {
    return;
  }

  double spinThresh = _params.cmd_spin_dbz_threshold;
  bool useDbzNarrow = _params.use_dbz_narrow_for_tdbz_and_spin;
  int spinSense = 0;
  
  for (int ii = 1; ii < _nGatesOut; ii++) {

    double dbz0 = 0, dbz1 = 0;

    if (useDbzNarrow) {
      dbz0 = _fields[ii-1].cmd_dbz_narrow;
      dbz1 = _fields[ii].cmd_dbz_narrow;
    } else {
      dbz0 = _fields[ii-1].dbz;
      dbz1 = _fields[ii].dbz;
    }
    
    if (dbz0 != Fields::missingDouble &&
        dbz1 != Fields::missingDouble) {
      double dbzDiff = dbz1 - dbz0;
      _fields[ii].cmd_dbz_diff_sq = dbzDiff * dbzDiff;
      _fields[ii].cmd_spin_change = 0;
      if (dbzDiff >= spinThresh) {
        if (spinSense != 1) {
          spinSense = 1;
          _fields[ii].cmd_spin_change = 1;
        }
      } else if (dbzDiff <= -spinThresh) {
        if (spinSense != -1) {
          spinSense = -1;
          _fields[ii].cmd_spin_change = 1;
        }
      }
    }

    _fields[ii].test = _fields[ii].cmd_spin_change;

  } // ii

  _fields[0].cmd_dbz_diff_sq = _fields[1].cmd_dbz_diff_sq;
  _fields[0].cmd_spin_change = _fields[1].cmd_spin_change;

  _cmdVarsReady = true;

}

////////////////////////////////////////////////////////
// compute azimuth difference, taking account of wrapping
    
double Beam::_computeAzDiff(double az1, double az2) const
  
{

  double diff = az1 - az2;
  if (diff > 180.0) {
    diff -= 360.0;
  } else if (diff < -180.0) {
    diff += 360.0;
  }
  return diff;

}
  
///////////////////////////////////////
// compute mean velocity

double Beam::_computeMeanVelocity(double vel1, double vel2, double nyquist)

{

  double diff = fabs(vel1 - vel2);
  if (diff < nyquist) {
    return (vel1 + vel2) / 2.0;
  } else {
    if (vel1 > vel2) {
      vel2 += 2.0 * nyquist;
    } else {
      vel1 += 2.0 * nyquist;
    }
    double vel = (vel1 + vel2) / 2.0;
    if (vel > nyquist) {
      vel -= 2.0 * nyquist;
    } else if (vel < -nyquist) {
      vel += 2.0 * nyquist;
    }
    return vel;
  }

}

/////////////////////////////////////////
// compute velocity difference as an angle

double Beam::_velDiff2Angle(double vel1, double vel2, double nyquist)

{
  
  double ang1 = (vel1 / nyquist) * 180.0;
  double ang2 = (vel2 / nyquist) * 180.0;
  double adiff = (ang1 - ang2) - _params.H2V_phase_differential;
  if (adiff > 180.0) {
    adiff -= 360.0;
  } else if (adiff < -180.0) {
    adiff += 360.0;
  }
  return adiff;
}

/////////////////////////////////////////
// multiply complex values

Complex_t Beam::_complexProduct(Complex_t c1, Complex_t c2)

{
  Complex_t product;
  product.re = (c1.re * c2.re) - (c1.im - c2.im);
  product.im = (c1.im * c2.re) + (c1.re * c2.im);
  return product;
}

/////////////////////////////////////////////
// compute mean conjugate product of series

Complex_t Beam::_meanConjugateProduct(const Complex_t *c1,
				      const Complex_t *c2,
				      int len)

{
  
  double sumRe = 0.0;
  double sumIm = 0.0;

  for (int ipos = 0; ipos < len; ipos++, c1++, c2++) {
    sumRe = (c1->re * c2->re) + (c1->im * c2->im);
    sumIm = (c1->im * c2->re) - (c1->re * c2->im);
  }

  Complex_t product;
  product.re = sumRe / len;
  product.im = sumIm / len;

  return product;

}

/////////////////////////////////////////
// get velocity from angle of complex val

double Beam::_velFromComplexAng(Complex_t cVal,
				double nyquist)
  
{
  double angle = 0.0;
  if (cVal.re != 0.0 || cVal.im != 0.0) {
    angle = atan2(cVal.im, cVal.re);
  }
  double vel = (angle / M_PI) * nyquist;
  return vel;
}

////////////////////////////////////////////////////////////////////////
// Compute interest value

double Beam::_computeInterest(double xx,
			      double x0, double x1)
  
{

  if (xx <= x0) {
    return 0.01;
  }
  
  if (xx >= x1) {
    return 0.99;
  }
  
  double xbar = (x0 + x1) / 2.0;
  
  if (xx <= xbar) {
    double yy = (xx - x0) / (x1 - x0);
    double yy2 = yy * yy;
    double interest = 2.0 * yy2;
    return interest;
  } else {
    double yy = (x1 - xx) / (x1 - x0);
    double yy2 = yy * yy;
    double interest = 1.0 - 2.0 * yy2;
    return interest;
  }

}

////////////////////////////////////////////////////////////////////////
// Add spectrum to combined spectra output file

void Beam::_addSpectrumToFile(FILE *specFile, int count, time_t beamTime,
			      double el, double az, int gateNum,
			      double snr, double vel, double width,
			      int nSamples, const Complex_t *iq)
  
{

  date_time_t btime;
  btime.unix_time = beamTime;
  uconvert_from_utime(&btime);
  
  fprintf(specFile,
	  "%d %d %d %d %d %d %d %g %g %d %g %g %g %d",
	  count,
	  btime.year, btime.month, btime.day,
	  btime.hour, btime.min, btime.sec,
	  el, az, gateNum,
	  snr, vel, width, nSamples);

  for (int ii = 0; ii < nSamples; ii++) {
    fprintf(specFile, " %g %g", iq[ii].re, iq[ii].im);
  }
  
  fprintf(specFile, "\n");

}

////////////////////////////////////////////////////////////////////////
// Convert interest map points to vector
//
// Returns 0 on success, -1 on failure

int Beam::_convertInterestMapToVector(const string &label,
				      const Params::interest_map_point_t *map,
				      int nPoints,
				      vector<InterestMap::ImPoint> &pts)

{

  pts.clear();

  double prevVal = -1.0e99;
  for (int ii = 0; ii < nPoints; ii++) {
    if (map[ii].value <= prevVal) {
      cerr << "ERROR - Beam::_convertInterestMapToVector" << endl;
      cerr << "  Map label: " << label << endl;
      cerr << "  Map values must increase monotonically" << endl;
      return -1;
    }
    InterestMap::ImPoint pt(map[ii].value, map[ii].interest);
    pts.push_back(pt);
  } // ii
  
  return 0;

}

//////////////////////////////
// apply speckle filter to CMD

void Beam::_applyCmdSpeckleFilter()

{

  int *countSet = new int[_nGatesOut];
  int *countNot = new int[_nGatesOut];

  // compute the running count of gates which have the flag set and
  // those which do not

  // Go forward through the gates, counting up the number of gates set
  // or not set and assigning that number to the arrays as we go.

  int nSet = 0;
  int nNot = 0;
  for (int igate = 0; igate < _nGatesOut; igate++) {
    if (_fields[igate].cmd_flag) {
      nSet++;
      nNot = 0;
    } else {
      nSet = 0;
      nNot++;
    }
    countSet[igate] = nSet;
    countNot[igate] = nNot;
  }

  // Go in reverse through the gates, taking the max non-zero
  // values and copying them across the set or not-set regions.
  // This makes all the counts equal in the gaps and set areas.

  for (int igate = _nGatesOut - 2; igate >= 0; igate--) {
    if (countSet[igate] != 0 &&
        countSet[igate] < countSet[igate+1]) {
      countSet[igate] = countSet[igate+1];
    }
    if (countNot[igate] != 0 &&
        countNot[igate] < countNot[igate+1]) {
      countNot[igate] = countNot[igate+1];
    }
  }

  // fill in gaps

  for (int igate = 1; igate < _nGatesOut - 1; igate++) {

    // is the gap small enough?

    nNot = countNot[igate];
    if (nNot > 0 && nNot <= _params.cmd_speckle_max_ngates_infilled) {

      // is it surrounded by regions at least as large as the gap?

      int minGateCheck = igate - nNot;
      if (minGateCheck < 0) {
        minGateCheck = 0;
      }
      int maxGateCheck = igate + nNot;
      if (maxGateCheck > _nGatesOut - 1) {
        maxGateCheck = _nGatesOut - 1;
      }

      int nAdjacentBelow = 0;
      for (int jgate = igate - 1; jgate >= minGateCheck; jgate--) {
        nSet = countSet[jgate];
        if (nSet != 0) {
          nAdjacentBelow = nSet;
          break;
        }
      } // jgate

      int nAdjacentAbove = 0;
      for (int jgate = igate + 1; jgate <= maxGateCheck; jgate++) {
        nSet = countSet[jgate];
        if (nSet != 0) {
          nAdjacentAbove = nSet;
          break;
        }
      } // jgate

      int minAdjacent = nAdjacentBelow;
      minAdjacent = MIN(minAdjacent, nAdjacentAbove);
      
      if (minAdjacent >= nNot) {
        _fields[igate].cmd_flag = 1;
      }
    }
  } // igate

  delete[] countSet;
  delete[] countNot;

}

////////////////////////////////////////////////////////////////////////
// Filter fields for spikes in dbz
//
// This routine filters the reflectivity data according to the
// NEXRAD specification DV1208621F, section 3.2.1.2.2, page 3-15.
//
// The algorithm is stated as follows:
//
// Clutter detection:
//
// The nth bin is declared to be a point clutter cell if its power value
// exceeds those of both its second nearest neighbors by a threshold
// value TCN. In other words:
//
//    if   P(n) exceeds TCN * P(n-2)
//    and  P(n) exceeds TCN * p(n+2)
//
//  where
//
//   TCN is the point clutter threshold factor, which is always
//       greater than 1, and typically has a value of 8 (9 dB)
//
//   P(n) if the poiwer sum value for the nth range cell
//
//   n is the range gate number
//
// Clutter censoring:
//
// The formulas for censoring detected strong point clutter in an
// arbitrary array A via data substitution are as follows. If the nth
// range cell is an isolated clutter cell (i.e., it si a clutter cell but
// neither of its immediate neighboring cells is a clutter cell) then the 
// replacement scheme is as follows:
//
//   Replace A(n-1) with  A(n-2)
//   Replace A(n)   with  0.5 * A(n-2) * A(n+2)
//   Replace A(n+1) with  A(n+2)
//
// If the nth and (n+1)th range bins constitute an isolated clutter pair,
// the bin replacement scheme is as follows:
//
//   Replace A(n-1) with  A(n-2)
//   Replace A(n)   with  A(n+2)
//   Replace A(n+1) with  A(n+3)
//   Replace A(n+2) with  A(n+3)
//
// Note that runs of more than 2 successive clutter cells cannot occur
// because of the nature of the algorithm.
 
void Beam::_applyNexradSpikeFilter()
  
{
  
  // set clutter threshold

  double tcn = 9.0;

  // loop through gates

  for (int ii = 2; ii < _nGatesOut - 3; ii++) {
    
    // check for clutter at ii and ii + 1

    bool this_gate = false, next_gate = false;
    
    if ((_fields[ii].dbzf - _fields[ii - 2].dbzf) > tcn &&
	(_fields[ii].dbzf - _fields[ii + 2].dbzf) > tcn) {
      this_gate = true;
    }
    if ((_fields[ii + 1].dbzf - _fields[ii - 1].dbzf) > tcn &&
	(_fields[ii + 1].dbzf - _fields[ii + 3].dbzf) > tcn) {
      next_gate = true;
    }

    if (this_gate) {

      if (!next_gate) {

	// only gate ii has clutter, substitute accordingly
	
	_fields[ii - 1].dbzf = _fields[ii - 2].dbzf;
	_fields[ii + 1].dbzf = _fields[ii + 2].dbzf;
	if (_fields[ii - 2].dbzf == Fields::missingDouble ||
            _fields[ii + 2].dbzf == Fields::missingDouble) {
	  _fields[ii].dbzf = Fields::missingDouble;
	  _fields[ii].velf = Fields::missingDouble;
	  _fields[ii].widthf = Fields::missingDouble;
	} else {
	  _fields[ii].dbzf = _fields[ii - 2].dbzf;
	  _fields[ii].velf = _fields[ii - 2].velf;
	  _fields[ii].widthf = _fields[ii - 2].widthf;
	}
	
      } else {

	// both gate ii and ii+1 has clutter, substitute accordingly

	_fields[ii - 1].dbzf = _fields[ii - 2].dbzf;
	_fields[ii].dbzf     = _fields[ii - 2].dbzf;
	_fields[ii + 1].dbzf = _fields[ii + 3].dbzf;
	_fields[ii + 2].dbzf = _fields[ii + 3].dbzf;

	_fields[ii - 1].velf = _fields[ii - 2].velf;
	_fields[ii].velf     = _fields[ii - 2].velf;
	_fields[ii + 1].velf = _fields[ii + 3].velf;
	_fields[ii + 2].velf = _fields[ii + 3].velf;

	_fields[ii - 1].widthf = _fields[ii - 2].widthf;
	_fields[ii].widthf     = _fields[ii - 2].widthf;
	_fields[ii + 1].widthf = _fields[ii + 3].widthf;
	_fields[ii + 2].widthf = _fields[ii + 3].widthf;

      }

    }
    
  } // ii

}

////////////////////////////////////////////////////////////////////////
// Filter dregs remaining after other filters
 
void Beam::_filterDregs(double nyquist,
			double *dbzf,
			double *velf,
			double *widthf)
  
{
  
  // compute vel diff array
  
  double veld[_nGatesOut];
  veld[0] = 0.0;
  
  for (int ii = 1; ii < _nGatesOut; ii++) {
    double diff;
    if (velf[ii] == Fields::missingDouble ||
        velf[ii -1] == Fields::missingDouble) {
      diff = nyquist / 2.0;
    } else {
      diff = velf[ii] - velf[ii -1];
      if (diff > nyquist) {
	diff -= 2.0 * nyquist;
      } else if (diff < -nyquist) {
	diff += 2.0 * nyquist;
      }
    }
    veld[ii] = diff;
  } // ii
  
  for (int len = 20; len >= 6; len--) {

    int nHalf = len / 2;

    for (int ii = nHalf; ii < _nGatesOut - nHalf; ii++) {
      
      int istart = ii - nHalf;
      int iend = istart + len;
      
      if (dbzf[istart] != Fields::missingDouble ||
          dbzf[iend] != Fields::missingDouble) {
	continue;
      }
      
      double sum = 0.0;
      double count = 0.0;
      int ndata = 0;
      for (int jj = istart; jj <= iend; jj++) {
	sum += veld[jj] * veld[jj];
	count++;
	if (dbzf[jj] != Fields::missingDouble) {
	  ndata++;
	}
      } // jj
      
      if (ndata == 0) {
	continue;
      }

      double texture = sqrt(sum / count) / nyquist;
      double interest = _computeInterest(texture, 0.0, 0.75);

      if (interest > 0.5) {
	for (int jj = istart; jj <= iend; jj++) {
	  dbzf[jj] = Fields::missingDouble;
	  velf[jj] = Fields::missingDouble;
	  widthf[jj] = Fields::missingDouble;
	} // jj
      }

    } // ii
    
  } // len
  
}

////////////////////////////////////////////////////////////////////////
// Perform infilling

void Beam::_performInfilling()
  
{

  // compute the infill texture

  _computeInfillTexture();

  // set the infill flag

  for (int ii = 0; ii < _nGatesOut; ii++) {
    if (_fields[ii].dbzf == Fields::missingDouble &&
	_fields[ii].sz_itexture != Fields::missingDouble &&
	_fields[ii].sz_itexture < 0.1) {
      _fields[ii].sz_zinfill = 1;
    } else {
      _fields[ii].sz_zinfill = 0;
    }
    // }
  }

  // perform infilling

  for (int ii = 0; ii < _nGatesOut; ii++) {
    _fields[ii].sz_dbzi = _fields[ii].dbzf;
    _fields[ii].sz_veli = _fields[ii].velf;
    _fields[ii].sz_widthi = _fields[ii].widthf;
  }
  
  int nInfill = 0;
  for (int ii = 0; ii < _nGatesOut; ii++) {
    if (ii == _nGatesOut - 1 || _fields[ii].sz_zinfill == 0) {
      if (nInfill > 0) {
	int nHalf = nInfill / 2;
	int iLowStart = ii - nInfill;
	int iLowEnd = iLowStart + nHalf - 1;
	int iHighStart = iLowEnd + 1;
	int iHighEnd = ii - 1;
	for (int jj = iLowStart; jj <= iLowEnd; jj++) {
	  _fields[jj].sz_dbzi = _fields[iLowStart - 1].dbzf;
	  _fields[jj].sz_veli = _fields[iLowStart - 1].velf;
	  _fields[jj].sz_widthi = _fields[iLowStart - 1].widthf;
	}
	for (int jj = iHighStart; jj <= iHighEnd; jj++) {
	  _fields[jj].sz_dbzi = _fields[ii].dbzf;
	  _fields[jj].sz_veli = _fields[ii].velf;
	  _fields[jj].sz_widthi = _fields[ii].widthf;
	}
      }
      nInfill = 0;
    } else {
      nInfill++;
    }
  }  

}

////////////////////////////////////////////////////////////////////////
// Compute Z texture

void Beam::_computeInfillTexture()
  
{
  
  // compute dbz diff array
  
  double * dbzd = new double[_nGatesOut];
  dbzd[0] = 0.0;
  int dbzBase = -40;
  
  for (int ii = 1; ii < _nGatesOut; ii++) {
    if (_fields[ii-1].dbz != Fields::missingDouble &&
        _fields[ii].dbz != Fields::missingDouble) {
      double dbz0 = _fields[ii-1].dbz - dbzBase;
      double dbz1 = _fields[ii].dbz - dbzBase;
      double dbzMean = (dbz0 + dbz1) / 2.0;
      double diff = (dbz1 - dbz0) / dbzMean;
      dbzd[ii] = diff;
    } else {
      dbzd[ii] = Fields::missingDouble;
    }
  } // ii
  
  // compute the z texture

  int nGates = 64;
  int nGatesHalf = nGates / 2;

  for (int ii = 0; ii < nGatesHalf - 1; ii++) {
    _fields[ii].sz_itexture = Fields::missingDouble;
  }
  for (int ii = _nGatesOut - nGatesHalf; ii < _nGatesOut; ii++) {
    _fields[ii].sz_itexture = Fields::missingDouble;
  }
  
  for (int ii = nGatesHalf; ii < _nGatesOut - nGatesHalf; ii++) {
    double sum = 0.0;
    double count = 0.0;
    for (int jj = ii - nGatesHalf; jj <= ii + nGatesHalf; jj++) {
      if (dbzd[jj] != Fields::missingDouble) {
	sum += (dbzd[jj] * dbzd[jj]);
	count++;
      } // jj
    }
    double texture;
    if (count < nGatesHalf) {
      texture = Fields::missingDouble;
    } else {
      texture = sqrt(sum / count);
    }
    // double interest = _computeInterest(texture, 40.0, 70.0);
    double interest = texture;
    _fields[ii].sz_itexture = interest;
  } // ii

  delete[] dbzd;

}


