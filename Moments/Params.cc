////////////////////////////////////////////
// Params.cc
//
// Params code for MomentsEngine
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
////////////////////////////////////////////
// Destructor
//
Params::~Params()
{

}
////////////////////////////////////////////
void
Params::setDefault()
{
	debug = Params::DEBUG_OFF;

	atmos_attenuation    = 0.012; // db/km
	dbz_calib_correction = 0.0;   // dB
	zdr_correction       = 0.0;   // dB
	ldr_correction       = 0.0;   // dB


	// radar params
	radar.radar_config     = DP_ALT_HV_CO_CROSS;
	radar.horiz_beam_width = 0.91;   // deg
	radar.vert_beam_width  = 0.91;   // deg
	radar.pulse_width      = 1.0;    // us
	radar.wavelength_cm    = 10.0;   // cm
	radar.xmit_peak_pwr    = 0.4e6;  // W

	// receiver calibrations
	hc_receiver.noise_dBm        = -77.0000;
	hc_receiver.noise_h_dBm      = hc_receiver.noise_dBm;
	hc_receiver.noise_v_dBm      = hc_receiver.noise_dBm;
	hc_receiver.gain             = 37.0000;
	hc_receiver.radar_constant   = -68.3782;
	hc_receiver.dbz0             = -48.0;
	hc_receiver.system_phidp_deg = 0.0;

	hx_receiver.noise_dBm        = -77.0000;
	hx_receiver.noise_h_dBm      = hx_receiver.noise_dBm;
	hx_receiver.noise_v_dBm      = hx_receiver.noise_dBm;
	hx_receiver.gain             = 37.0000;
	hx_receiver.radar_constant   = -68.3782;
	hx_receiver.dbz0             = -48.0;
	hx_receiver.system_phidp_deg = 0.0;

	vc_receiver.noise_dBm        = -77.0000;
	vc_receiver.noise_h_dBm      = vc_receiver.noise_dBm;
	vc_receiver.noise_v_dBm      = vc_receiver.noise_dBm;
	vc_receiver.gain             = 37.0000;
	vc_receiver.radar_constant   = -68.3782;
	vc_receiver.dbz0             = -48.0;
	vc_receiver.system_phidp_deg = 0.0;

	vx_receiver.noise_dBm        = -77.0000;
	vx_receiver.noise_h_dBm      = vx_receiver.noise_dBm;
	vx_receiver.noise_v_dBm      = vx_receiver.noise_dBm;
	vx_receiver.gain             = 37.0000;
	vx_receiver.radar_constant   = -68.3782;
	vx_receiver.dbz0             = -48.0;
	vx_receiver.system_phidp_deg = 0.0;

	// moments manager params

	moments_params.mode                   = DUAL_FAST_ALT;
	moments_params.n_samples              = 64;
	moments_params.start_range            = 0.075;
	moments_params.start_range            = 0.150;
	moments_params.algorithm              = ALG_PP;
	moments_params.window                 = WINDOW_HANNING;
	moments_params.moments_corr_type      = R1R2;
	moments_params.apply_clutter_filter   = false;
	moments_params.index_beams_in_azimuth = true;
	moments_params.azimuth_resolution     = 1.0; // for indexed beams
}

