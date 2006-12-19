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
   setDefault();
}

Params::Params(moments_mode_t mode)
{
  setDefault();
  moments_params.mode = mode;
}

////////////////////////////////////////////
// Destructor
//

Params::~Params()
  
{

}
void
Params::setDefault()
{
  debug = Params::DEBUG_OFF;

  atmos_attenuation = 0.012; // db/km
  dbz_calib_correction = 0.0; // dB
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
  moments_params.n_samples = 64;
  moments_params.lower_prf = 500;
  moments_params.upper_prf = 2000;
  moments_params.start_range = 0.075;
  moments_params.start_range = 0.150;
  moments_params.algorithm = ALG_PP;
  moments_params.window = WINDOW_HANNING;
}

