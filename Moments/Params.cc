////////////////////////////////////////////
// Params.cc
//
// Params code for MomentsCompute
//
/////////////////////////////////////////////

using namespace std;

#include "Params.hh"

////////////////////////////////////////////
// Default constructor
//

Params::Params()
  
{

  debug = DEBUG_VERBOSE;

  atmos_attenuation = 0.012; // db/km
  dbz_calib_correction = 0.0; // dB
  moments_snr_threshold = 3.0; // dB
  zdr_correction = 0.0; // dB
  ldr_correction = 0.0; // dB

  index_beams_in_azimuth = true;
  azimuth_resolution = 1.0; // for indexed beams

  // radar params

  radar.horiz_beam_width = 0.91; // deg
  radar.vert_beam_width = 0.91; // deg
  radar.pulse_width = 1.0; // us
  radar.wavelength_cm = 10.0; // cm
  radar.xmit_peak_pwr = 0.4e6; // W

  // receiver calibrations

  hc_receiver.noise_dBm = -77.0000;
  hc_receiver.gain  = 37.0000;
  hc_receiver.radar_constant = -68.3782;
  hc_receiver.dbz0 = -48.0;
  
  hx_receiver.noise_dBm = -77.0000;
  hx_receiver.gain  = 37.0000;
  hx_receiver.radar_constant = -68.3782;
  hx_receiver.dbz0 = -48.0;

  vc_receiver.noise_dBm = -77.0000;
  vc_receiver.gain  = 37.0000;
  vc_receiver.radar_constant = -68.3782;
  vc_receiver.dbz0 = -48.0;

  vx_receiver.noise_dBm = -77.0000;
  vx_receiver.gain  = 37.0000;
  vx_receiver.radar_constant = -68.3782;
  vx_receiver.dbz0 = -48.0;

  // moments manager params

  n_moments_params = 2;
  moments_params = new moments_params_t[n_moments_params];
 
  moments_params[0].n_samples = 32;
  moments_params[0].lower_prf = 0;
  moments_params[0].upper_prf = 500;
  moments_params[0].start_range = 0.075;
  moments_params[0].start_range = 0.150;
  moments_params[0].algorithm = ALG_FFT;
  moments_params[0].window = WINDOW_BLACKMAN;
  moments_params[0].mode = SINGLE_POL;

  moments_params[1].n_samples = 64;
  moments_params[1].lower_prf = 500;
  moments_params[1].upper_prf = 2000;
  moments_params[1].start_range = 0.075;
  moments_params[1].start_range = 0.150;
  moments_params[1].algorithm = ALG_FFT;
  moments_params[1].window = WINDOW_HANNING;
  moments_params[1].mode = DUAL_FAST_ALT;

}

////////////////////////////////////////////
// Destructor
//

Params::~Params()
  
{

  if (moments_params) {
    delete moments_params;
  }
  
}

