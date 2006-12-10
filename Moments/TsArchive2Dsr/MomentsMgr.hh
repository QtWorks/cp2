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
// MomentsMgr.hh
//
// Mike Dixon, RAP, NCAR
// P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// April 2005
//
///////////////////////////////////////////////////////////////
//
// MomentsMgr manages the use of the Moments objects, handling 
// the specific parameters for each case.
//
///////////////////////////////////////////////////////////////

#ifndef MomentsMgr_hh
#define MomentsMgr_hh

#include <string>
#include <vector>
#include <cstdio>
#include "Params.hh"
#include "Pulse.hh"
#include "Moments.hh"
#include "GateSpectra.hh"
#include "OpsInfo.hh"
#include "Fields.hh"
using namespace std;

////////////////////////
// This class

class MomentsMgr {
  
public:
  
  // constructor
  
  MomentsMgr (const string &prog_name,
	      const Params &params,
              const OpsInfo &opsInfo,
	      Params::moments_params_t &moments_params,
	      int n_samples,
	      int max_gates);
  
  // destructor
  
  ~MomentsMgr();
  
  // compute moments, either normal or dual pol
  // Set combinedSpectraFile to NULL to prevent printing.
  
  void computeMoments(time_t beamTime,
		      double el, double az, double prt,
		      int nGatesPulse,
		      Complex_t **IQ,
		      int &combinedPrintCount,
		      FILE *combinedSpectraFile,
                      Fields *fields,
		      vector<GateSpectra *> gateSpectra);
  
  void computeMomentsSz(time_t beamTime,
			double el, double az, double prt,
			int nGatesPulse,
			Complex_t **IQ,
			const Complex_t *delta12,
			int &combinedPrintCount,
			FILE *combinedSpectraFile,
                        Fields *fields,
			vector<GateSpectra *> gateSpectra);
  
  void computeDualAlt(time_t beamTime,
                      double el, double az, double prt,
                      int nGatesPulse,
                      Complex_t **IQ,
                      Complex_t **IQH,
                      Complex_t **IQV,
                      int &combinedPrintCount,
                      FILE *combinedSpectraFile,
                      Fields *fields,
                      vector<GateSpectra *> gateSpectra);
  
  void computeRefract(int nGatesPulse, int nSamples,
                      Complex_t **IQ,
                      Fields *fields);

  void filterClutter(double prt,
		     int nGatesPulse,
		     Complex_t **IQ,
                     Fields *fields,
		     vector<GateSpectra *> gateSpectra);
  
  void filterClutterSz(double prt,
		       int nGatesPulse,
		       const Complex_t *delta12,
                       Fields *fields,
		       vector<GateSpectra *> gateSpectra);
  
  void filterClutterDualAlt(double prt,
			    int nGatesPulse,
			    Complex_t **IQ,
			    Complex_t **IQH,
			    Complex_t **IQV,
                            Fields *fields,
			    vector<GateSpectra *> gateSpectra);

  // get methods
  
  double getLowerPrf() const { return _lowerPrf; }
  double getUpperPrf() const { return _upperPrf; }
  double getPulseWidth() const { return _pulseWidth; }

  double getStartRange() const { return _startRange; }
  double getGateSpacing() const { return _gateSpacing; }
  int getMaxGates() const { return _maxGates; }

  bool getUseFft() const { return _useFft; }
  bool getApplySz() const { return _applySz; }
  bool getDualPol() const { return _dualPol; }

  Moments::window_t getFftWindow() const { return _fftWindow; }

  int getNSamples() const { return _nSamples; }

  const double *getRangeCorr() const { return _rangeCorr; }

  double getDbz0Chan0() const { return _dbz0Chan0; }
  double getDbz0Chan1() const { return _dbz0Chan1; }
  double getAtmosAtten() const { return _atmosAtten; }

  const Moments &getMoments() const { return _moments; }

protected:
  
private:

  static const double _missingDbl;

  string _progName;
  const Params &_params;
  const OpsInfo &_opsInfo;
  
  // PRF range
  
  double _lowerPrf;
  double _upperPrf;
  double _pulseWidth;
  
  // geometry
  
  int _maxGates;

  // flags

  bool _useFft;
  bool _applySz;
  bool _dualPol;
  Moments::window_t _fftWindow;

  // number of pulse samples

  int _nSamples;
  int _nSamplesHalf;
  
  // range correction

  double *_rangeCorr;
  double _startRange;
  double _gateSpacing;

  // calibration
  
  double _atmosAtten;  // db/km
  double _noiseFixedChan0;
  double _noiseFixedChan1;
  double _dbz0Chan0;
  double _dbz0Chan1;

  // compute refractivity fields

  bool _computeRefract;

  // Moments object

  Moments _moments;
  Moments _momentsHalf; // for alternating mode dual pol

  // functions

  void _init();

  void _computeRangeCorrection();

  double _computeMeanVelocity(double vel1, double vel2, double nyquist) const;
  
  double _velDiff2Angle(double vel1, double vel2, double nyquist) const;
  
  Complex_t _complexSum(Complex_t c1, Complex_t c2) const;
  
  Complex_t _complexMean(Complex_t c1, Complex_t c2) const;
  
  Complex_t _complexProduct(Complex_t c1, Complex_t c2) const;
  
  Complex_t _conjugateProduct(Complex_t c1, Complex_t c2) const;
  
  Complex_t _meanComplexProduct(const Complex_t *c1,
				const Complex_t *c2,
				int len) const;

  Complex_t _meanConjugateProduct(const Complex_t *c1,
				  const Complex_t *c2,
				  int len) const;

  double _velFromComplex(Complex_t cc, double nyquist) const;
  
  double _velFromArg(double arg, double nyquist) const;
  
  Complex_t _computeComplexDiff(const Complex_t &c1,
				const Complex_t &c2) const;

  double _computeArgDiff(const Complex_t &c1,
			 const Complex_t &c2) const;

  double _computeArg(const Complex_t &cc) const;

  double _computeMag(const Complex_t &cc) const;

  double _computePower(const Complex_t &cc) const;

  double _meanPower(const Complex_t *c1, int len) const;

  void _computeKdp(int nGates, Fields *fields) const;
  
  double _computePhidpSlope(int index,
                            int nGatesForSlope,
                            int nGatesHalf,
                            const Fields *fields) const;

  double _computeInterest(double xx, double x0, double x1) const;

  void _addSpectrumToFile(FILE *specFile, int count, time_t beamTime,
			  double el, double az, int gateNum,
			  double snr, double vel, double width,
			  int nSamples, const Complex_t *iq) const;

  bool _setMomentsDebugPrint(const Moments &moments, double el,
			     double az, double range) const;
  
  void _applyTimeDomainFilter(const Complex_t *iq,
			      Complex_t *filtered) const;

  void _computeFoldingAlt(const double *snr,
                          const double *vel,
                          int *fold,
                          int nGatesPulse,
                          double nyquist) const;
  
  void _applyMedianFilterToVcarDb(int nGatesPulse,
                                  Fields *fields);

  void _applyMedianFilterToCPA(int nGatesPulse,
                               Fields *fields);

  void _applyMedianFilter(double *field,
                          int fieldLen,
                          int filterLen);

  static int _doubleCompare(const void *i, const void *j);

};

#endif

