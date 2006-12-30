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
#include "Fields.hh"
using namespace std;

////////////////////////
// This class

class MomentsMgr {

public:

	// constructor

	MomentsMgr (const Params &params,
		Params::moments_params_t &moments_params);

	// destructor

	~MomentsMgr();

	// compute moments - single pol
	// IQC data is for a single-channel copolar
	//   IQC[nGates][nPulses]

	void computeSingle(double beamTime,
		double el,
		double az,
		double prt,
		int nGates,
		Complex_t **IQC,
		Fields *fields);

	void computeDualFastAlt(double beamTime,
		double el,
		double az,
		double prt,
		int nGates,
		Complex_t **IQHC,
		Complex_t **IQVC,
		Fields *fields);

	void computeDualCp2Xband(double beamTime,
		double el,
		double az,
		double prt,
		int nGates,
		Complex_t **IQHC,
		Complex_t **IQVX,
		Fields *fields);

	// get methods

	int getNSamples() const { return _nSamples; }
	Params::moments_mode_t getMode() const { return _momentsParams.mode; }
	double getStartRange() const { return _startRange; }
	double getGateSpacing() const { return _gateSpacing; }  
	const double *getRangeCorr() const { return _rangeCorr; }

	Moments::window_t getFftWindow() const { return _fftWindow; }

	const Moments &getMoments() const { return _moments; }

protected:

private:

	static const double _missingDbl;

	const Params &_params;
	const Params::moments_params_t _momentsParams;

	// fft window

	Moments::window_t _fftWindow;

	// number of pulse samples

	int _nSamples;
	int _nSamplesHalf;

	// Moments objects

	Moments _moments;
	Moments _momentsHalf; // for alternating mode dual pol

	// range correction

	int _nGates;
	double _startRange;
	double _gateSpacing;
	double *_rangeCorr;

	// calibration

	double _noiseFixedHc;
	double _noiseFixedHx;
	double _noiseFixedVc;
	double _noiseFixedVx;

	double _dbz0Hc;
	double _dbz0Hx;
	double _dbz0Vc;
	double _dbz0Vx;

	// functions

	void _checkRangeCorrection(int nGates);

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

	// Kdp calculations disabled until a better algorithm is 
	// implemented.
	//  void _computeKdp(int nGates, Fields *fields) const;

	//  double _computePhidpSlope(int index,
	//                            int nGatesForSlope,
	//                            int nGatesHalf,
	//                            const Fields *fields) const;

	double _computeInterest(double xx, double x0, double x1) const;

	void _computeFoldingAlt(const double *snr,
		const double *vel,
		int *fold,
		int nGatesPulse,
		double nyquist) const;

	static int _doubleCompare(const void *i, const void *j);

};

#endif

