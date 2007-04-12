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

	typedef enum radar_config_t {
		/// Single polarization (NEXRAD)
		SP = 0, 
		/// Dual pol, alternating transmission, copolar receiver only
		/// (CP2 SBand)
		DP_ALT_HV_CO_ONLY = 1, 
		/// Dual pol, alternating transmission, co-polar and cross-polar
		///receivers (SPOL with Mitch Switch and receiver in 
		/// switching mode, CHILL)
		DP_ALT_HV_CO_CROSS = 2,
		/// Dual pol, alternating transmission, fixed H and V receivers (SPOL
		/// with Mitch Switch and receivers in fixed mode)
		DP_ALT_HV_FIXED_HV = 3,
		/// Dual pol, simultaneous transmission, fixed H and V receivers (NEXRAD
		/// upgrade, SPOL with T and receivers in fixed mode)
		DP_SIM_HV_FIXED_HV = 4,
		/// Dual pol, simultaneous transmission, switching H and V receivers
		/// (SPOL with T and receivers in switching mode)
		DP_SIM_HV_SWITCHED_HV = 5,
		/// Dual pol, H transmission, fixed H and V receivers (CP2 X band)
		DP_H_ONLY_FIXED_HV = 6
	} radar_config_t;

	typedef enum debug_t {
		DEBUG_OFF = 0,
		DEBUG_NORM = 1,
		DEBUG_VERBOSE = 2,
		DEBUG_EXTRA_VERBOSE = 3
	} debug_t;

	typedef enum algorithm_t {
		ALG_PP = 0,
		ALG_FFT = 1
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

	typedef enum moments_corr_t {
		R0R1 = 0,
		R1R2 = 1
	} moments_corr_t;

	// struct typedefs

	typedef struct radar_params_t {
		radar_config_t radar_config;
		double horiz_beam_width;
		double vert_beam_width;
		double pulse_width;
		double wavelength_cm;
		double xmit_peak_pwr;
	} radar_params_t;

	typedef struct receiver_t {
		double noise_dBm; // as digitized, i.e. after gain applied
		double noise_h_dBm;
		double noise_v_dBm;
		double gain;
		double radar_constant;
		double dbz0;
		double system_phidp_deg;
	} receiver_t;

	typedef struct moments_params_t {
		moments_mode_t  mode;
		int             n_samples;
		double          start_range;
		double          gate_spacing;
		algorithm_t     algorithm;
		fft_window_t    window;
		moments_corr_t  moments_corr_type;
		bool            apply_clutter_filter;
		double          azimuth_resolution;
		bool            index_beams_in_azimuth;
	} moments_params_t;

	///////////////////////////
	// Member functions
	//

	////////////////////////////////////////////
	// Default constructor
	//

	Params();

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

