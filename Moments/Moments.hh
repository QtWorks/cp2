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
// Moments.hh
//
// Mike Dixon, RAP, NCAR
// P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// March 2006
//
///////////////////////////////////////////////////////////////

#ifndef Moments_hh
#define Moments_hh

#include <string>
#include <vector>
#include "Fft.hh"

using namespace std;

////////////////////////
// This class

class Moments {
  
public:

  typedef enum {
    WINDOW_NONE,
    WINDOW_VONHANN,
    WINDOW_BLACKMAN
  } window_t;

  // constructor
  
  Moments(int n_samples);
  
  // destructor
  
  ~Moments();

  // set debug printing

  void setDebugPrint(bool status = true) const;

  // set wavelength
  
  void setWavelength(double wavelength);

  // set the noise value in dBM

  void setNoiseValueDbm(double dbm);

  // apply rectangular window
  
  void applyRectWindow(const Complex_t *in, Complex_t *out) const;

  // apply vonhann window to a time series
  
  void applyVonhannWindow(const Complex_t *in, Complex_t *out) const;
  
  // apply modified vonhann window to a time series

  void applyBlackmanWindow(const Complex_t *in, Complex_t *out) const;

  // compute power from IQ

  double computePower(const Complex_t *IQ) const;
  
  // compute power from magnitudes
  
  double computePower(const double *mag) const;
    
  // compute moments using pulse-pair
  
  void computeByPp(const Complex_t *IQ,
		   double prtSecs,
		   double &power,
                   double &vel,
		   double &width) const;
  
  // compute moments using fft spectra

  void computeByFft(const Complex_t *IQ,
		    window_t windowType,
		    double prtSecs,
		    double &power,
		    double &vel,
                    double &width) const;

  /////////////////////////
  // static utility methods

  // compute complex conjugate

  static void conjugate(int nSamples, const Complex_t *in, Complex_t *conj);

  // load magnitudes from complex power spectrum

  static void loadMag(int nSamples, const Complex_t *in, double *mag);

  // load power from complex spectrum

  static void loadPower(int nSamples, const Complex_t *in, double *power);

  // normalize magnitudes to 1

  static void normalizeMag(int nSamples, const Complex_t *in, double *norm_mag);

protected:
  
private:

  static const double _missingDbl;

  int _nSamples;
  double _wavelengthMeters;
  double _noiseValueDbm;
  double _noiseValueMwatts;
  
  // windows

  double *_vonhann;
  double *_blackman;

  // FFT support

  Fft *_fft;

  // debugging

  mutable bool _debugPrint;

  // functions

  void _initVonhann(double *window);
  void _initBlackman(double *window);

  void _applyWindow(const double *window,
		    const Complex_t *in, Complex_t *out) const;

  void _computeSpectralNoise(const double *powerCentered,
			     double &noiseMean,
			     double &noiseSdev) const;
  
  void _velWidthFromTd(const Complex_t *IQ,
		       double prtSecs,
		       double &vel, double &width) const;
  
  void _velWidthFromFft(const double *magnitude,
			double prtSecs,
			double &vel, 
                        double &width) const;
  
};

#endif

