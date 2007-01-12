////////////////////////////////////////////
// Params.hh
//
// Header file for 'Params' class.
//
// Code for program TsArchive2Dsr
//
/////////////////////////////////////////////

#ifndef Params_hh
#define Params_hh

#include <vector>
using namespace std;

#include <iostream>
#include <cstdio>
#include <climits>
#include <cfloat>

// Class definition

class Params {

public:

  // enum typedefs

  typedef enum debug_t {
    DEBUG_OFF = 0,
    DEBUG_NORM = 1,
    DEBUG_VERBOSE = 2,
    DEBUG_EXTRA_VERBOSE = 3
  } debug_t;

  typedef enum algorithm_t {
    ALG_PP = 0,
    ALG_ABP = 1,
    ALG_FFT = 2
  } algorithm_t;

  typedef enum fft_window_t {
    WINDOW_HANNING = 0,
    WINDOW_BLACKMAN = 1,
    WINDOW_NONE = 2
  } fft_window_t;

  typedef enum moments_mode_t {
    SINGLE_POL = 0,
    DUAL_FAST_ALT = 1,
    DUAL_CP2_XBAND = 2
  } moments_mode_t;

  // struct typedefs

  typedef struct radar_params_t {
    double horiz_beam_width;
    double vert_beam_width;
    double pulse_width;
    double wavelength_cm;
    double xmit_peak_pwr;
  } radar_params_t;

  typedef struct receiver_t {
    double noise_dBm; // as digitized, i.e. after gain applied
    double gain;
    double radar_constant;
    double dbz0;
  } receiver_t;

  typedef struct moments_params_t {
    int n_samples;
    double start_range;
    double gate_spacing;
    algorithm_t algorithm;
    fft_window_t window;
    moments_mode_t mode;
  } moments_params_t;

  ///////////////////////////
  // Member functions
  //

  ////////////////////////////////////////////
  // Default constructor
  //

  Params();

  Params(moments_mode_t mode, int samples);

  ////////////////////////////////////////////
  // Destructor
  //

  ~Params ();

  ///////////////////////////
  // Data Members
  //

  debug_t debug;

  double atmos_attenuation;
  double dbz_calib_correction;
  double zdr_correction;
  double ldr_correction;

  bool index_beams_in_azimuth;
  double azimuth_resolution;

  radar_params_t radar;

  receiver_t hc_receiver;
  receiver_t hx_receiver;
  receiver_t vc_receiver;
  receiver_t vx_receiver;

  moments_params_t moments_params;  

protected:
	void setDefault();

private:

};

#endif

