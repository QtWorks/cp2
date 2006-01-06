/****************************************************/
/* Read files to fill up the configuration structure        */
/****************************************************/

#include <Afx.h>

#include	<stdio.h>
#include <errno.h>

#include "../include/proto.h"

#include <time.h>
#include <math.h>

#define TESTING_INITIALIZATION	// put obviously incorrect data in unititialized INFOHEADER allocations 

void struct_init(INFOHEADER *h, char *fname)
   {
   RADAR		*radar;
   CONFIG	*config;
   struct synth synthinfo[CHANNELS]; 
   FILE * SynthAngleInfo; 

   int  i; 
   unsigned int channel; // radar channel for channel-specific parameters (RapidDOW) 

   config  = (CONFIG *)malloc(sizeof(CONFIG));
   radar = (RADAR *)malloc(sizeof(RADAR));

   readconfig(fname,config);   /* read in config.dsp and set up all parameters */
   readradar(fname,radar);	/* read in the config.rdr file */   

   channel = h->channel * 2; /* here h->channel = board#; = channel# in channel-separated data */ 
   printf("struct_init(): h->channel = %d h->gates = %d\n", h->channel, h->gates); // 
   strncpy(h->desc,"DWLX",4);
   h->one = 1;			/* one, per JVA request (endian flag: LITTLE_ENDIAN) */ 
   h->byte_offset_to_data = sizeof(INFOHEADER);	/*   */
   h->typeof_compression = 0; // NO_COMPRESSION;			/* none */ 
   h->ctrlflags = 0;	// later statements assume this
   h->recordlen	= 0;
   h->start_gate	= 0;	// use this sensible default, but needs attention
   h->gates 		= config->gatesa;
   h->hits			= config->hits;
   h->rcvr_pulsewidth	= (float)config->rcvr_pulsewidth * (8.0/(float)SYSTEM_CLOCK); // SYSTEM_CLOCK=48e6 gives 6MHz timebase 
   h->prt[0]			= (float)config->prt * (8.0/(float)SYSTEM_CLOCK); // SYSTEM_CLOCK=48e6 gives 6MHz timebase 
   h->prt[1]			= (float)config->prt2 * (8.0/(float)SYSTEM_CLOCK); // SYSTEM_CLOCK=48e6 gives 6MHz timebase 
   printf("struct_init(): h->rcvr_pulsewidth = %+8.3e config->rcvr_pulsewidth = %+8.3e h->prt[0] = %+8.3e h->prt[1] = %+8.3e\n", h->rcvr_pulsewidth, config->rcvr_pulsewidth, h->prt[0], h->prt[1]); 
//   if prt2 not specified in config file, set to prt:    if (h->prt[0] != h->prt[1]) 
//   h->delay				= config->delay * 100e-9; // ?corresponding allocation 

   config->clutterfilter ? SET(h->ctrlflags,CTRL_CLUTTERFILTER) : CLR(h->ctrlflags,CTRL_CLUTTERFILTER);
   config->timeseries ? SET(h->ctrlflags,CTRL_TIMESERIES) : CLR(h->ctrlflags,CTRL_TIMESERIES);

   h->ts_start_gate			= config->ts_start_gate;
   h->ts_end_gate			= config->ts_end_gate;
#ifndef TESTING_INITIALIZATION
   h->secs				= 0;
   h->nanosecs			= 0;
   h->az				= 0;
   h->el				= 0;
   h->radar_longitude	= -104;
   h->radar_latitude	= 40;
   h->radar_altitude	= 1500;
   h->ew_velocity		= 0;
   h->ns_velocity		= 0;
   h->vert_velocity	= 0;
   h->fxd_angle		= 0;
   h->scan_type		= 0;
   h->scan_num		= 0;
   h->vol_num		= 0;
#endif
   h->dataformat		= config->dataformat;
   if (h->dataformat == PIRAQ_CP2_TIMESERIES)
	   h->bytespergate = 2*sizeof(float); // CP2: 2 fp I,Q per gate
   h->clutter_type[0] = config->clutterfilter; 
   h->clutter_start[0] = config->clutter_start; 
   h->clutter_end[0] = config->clutter_end; 
   h->xmit_power	= radar->peak_power;
// put radar file piraq_saturation_power in infoheader data_sys_sat from radar struct data_sys_sat
   h->data_sys_sat	= radar->data_sys_sat;
   h->E_plane_angle	= radar->E_plane_angle;
   h->H_plane_angle	= radar->H_plane_angle;
   h->antenna_rotation_angle	= radar->antenna_rotation_angle;
#ifndef TESTING_INITIALIZATION
   h->yaw				= 0;
   h->pitch				= 0;
   h->roll				= 0;
   h->gate0mag		= 1.0;
#endif
   h->packetflag	= 0;	// clear: set to -1 by piraq on hardware EOF detect 

   h->dacv				= radar->frequency;
   h->rev				= radar->rev; //???see below
   h->year				= radar->year;
   strncpy(h->radar_name,radar->radar_name,PX_MAX_RADAR_NAME);
   strncpy(h->channel_name,radar->channel_name,PX_MAX_CHANNEL_NAME);
   strncpy(h->project_name,radar->project_name,PX_MAX_PROJECT_NAME);
   strncpy(h->operator_name,radar->operator_name,PX_MAX_OPERATOR_NAME);
   strncpy(h->site_name,radar->site_name,PX_MAX_SITE_NAME);
   strncpy(h->comment,radar->text,PX_SZ_COMMENT);
   h->polarization		= 1;
   h->test_pulse_pwr	= radar->test_pulse_pwr;

   h->test_pulse_frq	= radar->test_pulse_frq;
   h->frequency		= radar->frequency;
   h->xmit_power	= radar->peak_power;
   h->noise_figure	= radar->noise_figure;

   // initalize channel-specific parameters; 'v' parms piraq CHB.
   h->receiver_gain	= radar->receiver_gain[channel];
   h->vreceiver_gain	= radar->receiver_gain[channel + 1];
   h->noise_power	= radar->noise_power[channel];
   h->vnoise_power	= radar->noise_power[channel + 1];

   h->data_sys_sat	= radar->data_sys_sat;
   h->antenna_gain	= radar->antenna_gain;	

   h->H_beam_width = radar->horz_beam_width;
   h->V_beam_width = radar->vert_beam_width;	
   h->xmit_pulsewidth = radar->xmit_pulsewidth;
   h->rconst		= radar->rconst;
   h->phaseoffset	= radar->phaseoffset;
   h->zdr_fudge_factor = radar->zdr_fudge_factor;
   h->mismatch_loss	= radar->missmatch_loss;
   h->radar_latitude	= radar->latitude;
   h->radar_longitude	= radar->longitude;
   h->radar_altitude	= radar->altitude;
   h->meters_to_first_gate = config->meters_to_first_gate; 
   h->gate_spacing_meters[0] = config->gate_spacing_meters; 
   h->num_segments = 1;	
   h->gates_in_segment[0] = h->gates;

   h->i_offset	= radar->i_offset;		// I dc offset 
   h->q_offset	= radar->q_offset;		// Q dc offset 

// unitialized parameters: set to obviously untrue values 
   h->prt[2] =    h->prt[3]		= 9999.9; // [0],[1] set above
   for (i = 1; i < PX_MAX_SEGMENTS; i++) { // only first gate_spacing_meters array element initialized
		h->gate_spacing_meters[i] = 9999.9; h->gates_in_segment[i] = 9999;
   } 
   for (i = 1; i < PX_NUM_CLUTTER_REGIONS; i++) { // 0th initialized above: 1 clutter region 
		h->clutter_start[i] = h->clutter_end[i] = h->clutter_type[i] = 9999;
   } 
   h->secs				= 9999;
   h->nanosecs			= 9999;
   strncpy(h->gps_datum,"GPSDATM",PX_MAX_GPS_DATUM-1); 
   h->ew_velocity		= -9999.9;
   h->ns_velocity		= -9999.9;
   h->vert_velocity	= -9999.9;

   h->fxd_angle		= -9999.9;
   h->true_scan_rate	= -9999.9;
   h->scan_type		= 9999;
   h->scan_num		= 9999;
   h->vol_num		= 9999;

   h->transition	= 9999;

   h->yaw			= -9999.9;
   h->pitch			= -9999.9;
   h->roll			= -9999.9;
   h->track			= -9999.9;
   h->gate0mag		= -9999.9;

   h->julian_day	= 9999;

   h->rcvr_const	= -9999.9;

   h->test_pulse_rngs_km[0]	=	h->test_pulse_rngs_km[1]	=-9999.9;

   h->i_norm		= -9999.9;
   h->q_norm		= -9999.9;
   h->i_compand		= -9999.9;
   h->q_compand		= -9999.9;
   h->transform_matrix[0][0][0]	=	h->transform_matrix[0][0][1]	= -9999.9;
   h->transform_matrix[0][1][0]	=	h->transform_matrix[0][1][1]	= -9999.9;
   h->transform_matrix[1][0][0]	=	h->transform_matrix[1][0][1]	= -9999.9;
   h->transform_matrix[1][1][0]	=	h->transform_matrix[1][1][1]	= -9999.9;
   for (i = 0; i < 4; i++) { 
		h->stokes[i] = 9999.9; 
   } 
   for (i = 0; i < 16; i++) {   
		h->spare[i] = 9999.9; 
   } 
   free(config);
   free(radar);
   }