////////////////////////////////////////////
// Params.cc
//
// Params code for MomentsCompute
//
/////////////////////////////////////////////

#include <vector>
using namespace std;

#include "Params.hh"

////////////////////////////////////////////
// Default constructor
//

Params::Params()
  
{

  debug = Params::DEBUG_OFF;

  atmos_attenuation = 0.012; // db/km
  dbz_calib_correction = 0.0; // dB
  moments_snr_threshold = 3.0; // dB
  zdr_correction = 0.0; // dB
  ldr_correction = 0.0; // dB

  index_beams_in_azimuth = false;
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

  moments_params_t mparams;

  mparams.n_samples = 10;
  mparams.lower_prf = 0;
  mparams.upper_prf = 500;
  mparams.start_range = 0.075;
  mparams.start_range = 0.150;
  mparams.algorithm = ALG_PP;
  mparams.window = WINDOW_BLACKMAN;
  mparams.mode = SINGLE_POL;

  moments_params.push_back(mparams);

  mparams.n_samples = 64;
  mparams.lower_prf = 500;
  mparams.upper_prf = 2000;
  mparams.start_range = 0.075;
  mparams.start_range = 0.150;
  mparams.algorithm = ALG_PP;
  mparams.window = WINDOW_HANNING;
  mparams.mode = DUAL_FAST_ALT;

  moments_params.push_back(mparams);

}

////////////////////////////////////////////
// Destructor
//

Params::~Params()
  
{

}

