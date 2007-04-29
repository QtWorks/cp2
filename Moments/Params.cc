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
	radar.xmit_rcv_mode    = DP_ALT_HV_CO_CROSS;
	radar.horiz_beam_width = 0.91;   // deg
	radar.vert_beam_width  = 0.91;   // deg
	radar.pulse_width      = 1.0;    // us
	radar.wavelength_cm    = 10.68;  // cm
	radar.xmit_peak_pwr    = 0.4e6;  // W

	// receiver calibrations
	hc_receiver.noise_dBm        = -77.0310;
	hc_receiver.noise_h_dBm      = hc_receiver.noise_dBm;
	hc_receiver.noise_v_dBm      = hc_receiver.noise_dBm;
	hc_receiver.gain             = 37.2704;
	hc_receiver.radar_constant   = -68.3782;

	hx_receiver.noise_dBm        = -77.1704;
	hx_receiver.noise_h_dBm      = hx_receiver.noise_dBm;
	hx_receiver.noise_v_dBm      = hx_receiver.noise_dBm;
	hx_receiver.gain             = 37.1916;
	hx_receiver.radar_constant   = -68.3782;

	vc_receiver.noise_dBm        = -77.4886;
	vc_receiver.noise_h_dBm      = vc_receiver.noise_dBm;
	vc_receiver.noise_v_dBm      = vc_receiver.noise_dBm;
	vc_receiver.gain             = 35.6743;
	vc_receiver.radar_constant   = -68.3782;

	vx_receiver.noise_dBm        = -77.7405;
	vx_receiver.noise_h_dBm      = vx_receiver.noise_dBm;
	vx_receiver.noise_v_dBm      = vx_receiver.noise_dBm;
	vx_receiver.gain             = 35.4526;
	vx_receiver.radar_constant   = -68.3782;

	// moments manager params

        moments_params.mode                   = DUAL_CP2_SBAND;
	moments_params.n_samples              = 64;
	moments_params.start_range            = 0.0;
	moments_params.gate_spacing           = 0.150;
	moments_params.algorithm              = ALG_PP;
        moments_params.window                 = WINDOW_NONE;
	moments_params.apply_clutter_filter   = false;
	moments_params.index_beams_in_azimuth = true;
	moments_params.azimuth_resolution     = 1.0; // for indexed beams

        correct_for_system_phidp = true;
        system_phidp = 45;

}

