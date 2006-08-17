#include <Afx.h>

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <time.h>
#include        <math.h>

#include        "../include/proto.h"

RADAR   *initradar(char *filename)
   {
   RADAR        *r;

   if((r = (RADAR *)malloc(sizeof(RADAR))) == NULL)
	{printf("INITRADAR: Cannot allocate memory for radar structure\n"); return(NULL);}
   
   readradar(filename,r);

   return(r);
   }

/* RapidDOW-specific config.rdr keywords for channel gain and noise power. receiver_gain, 
   receiver_gain replaced by receiver0_gain ... receiver5_gain, etc. this confines the 
   "branch" to here ???  */ 
static char *parms[] = {
				"radar_name","polarization","test_pulse_power",
				"test_pulse_frequency","frequency","transmit_power",
				"noise_figure","noise_power0","noise_power1",
				"noise_power2","noise_power3","noise_power4","noise_power5",
				"receiver_gain0","receiver_gain1","receiver_gain2",
				"receiver_gain3","receiver_gain4","receiver_gain5",
				"piraq_saturation_power","antenna_gain",
				"horizontal_beamwidth","vertical_beamwidth",
				"xmit_pulsewidth","debug","text","phase_offset",
				"vert_receiver_gain","vert_antenna_gain",
				"vnoise_power","zdr_fudge_factor","missmatch_loss",
				"e_plane_angle","h_plane_angle","antenna_rotation_angle",
				"latitude","longitude","altitude",
				"i_offset","q_offset","zdr_bias","noise_phase_offset","channel_name",
				"project_name",	"operator_name","site_name",
				"description",""};

/* if fname is NULL string, default file will be loaded. */
/* all characters following text keyword are read-in as ascii */
void readradar(char *fname, RADAR *radar)
   {
   int  linenum,debug,text,i,c,err=0;
   char filename[180],keyword[180],value[180],line[180],name[180];
   time_t       tod;
   struct       tm      *tmbuf;
   double       logstuff;
   FILE *fp;

   i = 0; /* force initialization for compiler! */
   /* initialize all parameters in the radar structure */
   *radar->radar_name = 0;
   *radar->text = 0;
   radar->polarization = radar->test_pulse_pwr = radar->test_pulse_frq 
	= radar->frequency = radar->peak_power = radar->noise_figure 
	/*= radar->noise_power = radar->receiver_gain[0]*/ = radar->data_sys_sat 
	= radar->antenna_gain = radar->horz_beam_width = radar->phaseoffset
	= radar->xmit_pulsewidth = radar->vert_beam_width = 0.0;
	radar->vantenna_gain = radar->missmatch_loss = /*radar->vnoise_power = */0.0;
	radar->latitude = 40.0; radar->longitude = -105.0; radar->altitude = 1700.0; 
	for (i = 0; i++; i < CHANNELS) { // all system channels
		radar->receiver_gain[i] = 0.0; 
		radar->noise_power[i] = 0.0; 
	} 
	if(strcmp(fname,""))        
      {
      // config file in .exe parent directory: 
	  strcpy(&filename[0],"../"); 
	  for(i=0; fname[i] && fname[i] != '.'; i++)  
		  filename[i+3] = fname[i];
	  strcpy(&filename[i+3],".rdr");
      fp = fopen(filename,"r");
      if(!fp)
	 {
	 strcpy(filename,"../config.rdr");
	 fp = fopen(filename,"r");
	 }
      }
   else 
      {
      strcpy(filename,"config.rdr");
      fp = fopen(filename,"r");
      }
   
   if(!fp) {printf("READRADAR: cannot open configuration file %s\n",filename); exit(0);}

   text = linenum = 0;
   while(!text && fgets(line,170,fp) != NULL)  /* while not end of file */
    {
    linenum++;
    getkeyval(line,keyword,value);  /* get the keyword and the value */
    if(strlen(keyword) != 0 && keyword[0] != ';')       /* if line is not a comment line or blank */  
      {
      switch(parse(keyword,parms))
	 {
	 case 0:        /* radar_name */
			strncpy(radar->radar_name,value,PX_MAX_RADAR_NAME);
			break;
	 case 1:        /* polarization */
			set(value,"%s",&radar->polarization,keyword,linenum,filename);
			break;
	 case 2:        /* test_pulse_pwr */
			set(value,"%f",&radar->test_pulse_pwr,keyword,linenum,filename);
			break;
	 case 3:        /* test_pulse_frq */
			set(value,"%f",&radar->test_pulse_frq,keyword,linenum,filename);
			break;
	 case 4:        /* frequency */
			set(value,"%f",&radar->frequency,keyword,linenum,filename);
			break;
	 case 5:        /* peak_power */
			set(value,"%f",&radar->peak_power,keyword,linenum,filename);
			break;
	 case 6:        /* noise_figure */
			set(value,"%f",&radar->noise_figure,keyword,linenum,filename);
			break;
	 case 7:        /* noise_power: ch0 */
			set(value,"%f",&radar->noise_power[0],keyword,linenum,filename);
			break;
	 case 8:        /* noise_power: ch1 */
			set(value,"%f",&radar->noise_power[1],keyword,linenum,filename);
			break;
	 case 9:        /* noise_power: ch2 */
			set(value,"%f",&radar->noise_power[2],keyword,linenum,filename);
			break;
	 case 10:        /* noise_power: ch3 */
			set(value,"%f",&radar->noise_power[3],keyword,linenum,filename);
			break;
	 case 11:        /* noise_power: ch4 */
			set(value,"%f",&radar->noise_power[4],keyword,linenum,filename);
			break;
	 case 12:        /* noise_power: ch5 */
			set(value,"%f",&radar->noise_power[5],keyword,linenum,filename);
			break;
	 case 13:        /* receiver_gain: ch0 */
			set(value,"%f",&radar->receiver_gain[0],keyword,linenum,filename);
			break;
	 case 14:        /* receiver_gain: ch1 */
			set(value,"%f",&radar->receiver_gain[1],keyword,linenum,filename);
			break;
	 case 15:        /* receiver_gain: ch2 */
			set(value,"%f",&radar->receiver_gain[2],keyword,linenum,filename);
			break;
	 case 16:        /* receiver_gain: ch3 */
			set(value,"%f",&radar->receiver_gain[3],keyword,linenum,filename);
			break;
	 case 17:        /* receiver_gain: ch4 */
			set(value,"%f",&radar->receiver_gain[4],keyword,linenum,filename);
			break;
	 case 18:        /* receiver_gain: ch5 */
			set(value,"%f",&radar->receiver_gain[5],keyword,linenum,filename);
			break;
	 case 19:        /* data_sys_sat */
			set(value,"%f",&radar->data_sys_sat,keyword,linenum,filename);
			break;
	 case 20:       /* antenna_gain */
			set(value,"%f",&radar->antenna_gain,keyword,linenum,filename);
			break;
	 case 21:        /* horizontal beamwidth */
			set(value,"%f",&radar->horz_beam_width,keyword,linenum,filename);
			break;
	 case 22:        /* vertical beamwidth */
			set(value,"%f",&radar->vert_beam_width,keyword,linenum,filename);
			break;
	 case 23:        /* transmitted pulsewidth */
			set(value,"%f",&radar->xmit_pulsewidth,keyword,linenum,filename);
			break;
	 case 24:        /* debug */
			set(value,"%q",&debug,keyword,linenum,filename);
			break;
	 case 25:        /* text */
			set(value,"%q",&text,keyword,linenum,filename);
			break;
	 case 26:        /* differential phase offset */
			set(value,"%f",&radar->phaseoffset,keyword,linenum,filename);
			radar->phaseoffset *= M_PI / 180.0;
			while(radar->phaseoffset >= M_PI) radar->phaseoffset -= 2 * M_PI;
			while(radar->phaseoffset <  -M_PI) radar->phaseoffset += 2 * M_PI;
			break;
	 case 27:       /* vert_receiver_gain */
//			set(value,"%f",&radar->vreceiver_gain,keyword,linenum,filename);
			break;
	 case 28:       /* vert_antenna_gain */
			set(value,"%f",&radar->vantenna_gain,keyword,linenum,filename);
			break;
	 case 29:       /* vnoise_power */
//			set(value,"%f",&radar->vnoise_power,keyword,linenum,filename);
			break;
	 case 30:       /* zdr_fudge_factor */
			set(value,"%f",&radar->zdr_fudge_factor,keyword,linenum,filename);
			break;
	 case 31:       /* receiver filter missmatch loss (positive number) */
			set(value,"%f",&radar->missmatch_loss,keyword,linenum,filename);
			break;
	 case 32:       /* E field plane angle */
			set(value,"%f",&radar->E_plane_angle,keyword,linenum,filename);
			break;
	 case 33:       /* H field plane angle */
			set(value,"%f",&radar->H_plane_angle,keyword,linenum,filename);
			break;
	 case 34:       /* antenna rotation angle: used in S-Pol 2nd frequency */
			set(value,"%f",&radar->antenna_rotation_angle,keyword,linenum,filename);
			break;
	 case 35:        /* latitude */
			set(value,"%f",&radar->latitude,keyword,linenum,filename);
			break;
	 case 36:        /* longitude */
			set(value,"%f",&radar->longitude,keyword,linenum,filename);
			break;
	 case 37:        /* altitude */
			set(value,"%f",&radar->altitude,keyword,linenum,filename);
			break;
	 case 38:        /* I dc offset */
			set(value,"%f",&radar->i_offset,keyword,linenum,filename);
			break;
	 case 39:        /* Q dc offset */
			set(value,"%f",&radar->q_offset,keyword,linenum,filename);
			break;
	 case 40:        /* zdr bias */
			set(value,"%f",&radar->zdr_bias,keyword,linenum,filename);
			break;
	 case 41:        /* noise phase offset: cushion for noise immunity in velocity estimation */
			set(value,"%f",&radar->noise_phase_offset,keyword,linenum,filename);
			break;
			// !note: keep these strncpy initializations LAST; add new parameters imm. above
	 case 42:       /* channel name */
			strncpy(radar->channel_name,value,PX_MAX_CHANNEL_NAME);
			break;
	 case 43:       /* project name */
			strncpy(radar->project_name,value,PX_MAX_PROJECT_NAME);
			break;
	 case 44:       /* operator name */
			strncpy(radar->operator_name,value,PX_MAX_OPERATOR_NAME);
			break;
	 case 45:       /* site name */
			strncpy(radar->site_name,value,PX_MAX_SITE_NAME);
			break;
	 case 46:       /* description: SVH, XH, XV */
			strncpy(radar->desc,value,PX_MAX_RADAR_DESC);
			break;
	 default:
		printf("unrecognized keyword \"%s\" in line %d of %s\n",keyword,linenum,filename);
		err++;
		break;
	 }
      }
     }
   
	 if(text) {    /* if there is text in the file */
      for(i=0; i<PX_SZ_COMMENT && (EOF != (c = fgetc(fp))); i++)
		  radar->text[i] = c;
	   radar->text[i] = '\0'; // !note: PX_SZ_COMMENT = 64
	 }
   fclose(fp);

   radar->recordlen = sizeof(RADAR);   

   radar->rev = 2;  /* rev is 1: text starts after byte 40 */

   tod = time(NULL);
   tmbuf = gmtime(&tod);
   radar->year = 1900 + tmbuf->tm_year;

   logstuff = radar->peak_power * 1000.0                /* transmit power (mW) */
	    * radar->horz_beam_width * M_PI / 180.0     /* beamwidth (rads) */
	    * radar->vert_beam_width * M_PI / 180.0     /* beamwidth (rads) */
	    * radar->xmit_pulsewidth                    /* transmitted pulsewidth (Sec) */
	    * M_PI * M_PI * M_PI                        /* dimensionless factor */
	    * K2                                        /* water phase  */
	    * radar->frequency * radar->frequency       /* frequency (Hz^2) */
	    / (C * 512.0 * 2 * log(2.0) * 1000.0 * 1000.0);  /* denom */

   radar->rconst = -10.0 * log10(logstuff)      /* from above */
		 - 2.0 * radar->antenna_gain   /* antenna gain */
		 + radar->missmatch_loss       /* filter missmatch loss */
		 + 180.0;                      /* scale for mm6/m3 */
   
   if(debug)
      {
      printf("\nDEBUG: %s\n",filename);
      strncpy(line,radar->radar_name,PX_MAX_RADAR_NAME);  line[PX_MAX_RADAR_NAME] = 0;
      printf("%-28s %s\n",parms[0],line);
      printf("%-28s %c\n",parms[1],radar->polarization);
      printf("%-28s %f\n",parms[2],radar->test_pulse_pwr);
      printf("%-28s %f\n",parms[3],radar->test_pulse_frq);
      printf("%-28s %f\n",parms[4],radar->frequency);
      printf("%-28s %f\n",parms[5],radar->peak_power);
      printf("%-28s %f\n",parms[6],radar->noise_figure);
      printf("%-28s %f\n",parms[7],radar->noise_power[0]);
      printf("%-28s %f\n",parms[8],radar->noise_power[1]);
      printf("%-28s %f\n",parms[9],radar->noise_power[2]);
      printf("%-28s %f\n",parms[10],radar->noise_power[3]);
      printf("%-28s %f\n",parms[11],radar->noise_power[4]);
      printf("%-28s %f\n",parms[12],radar->noise_power[5]);
//     printf("%-28s %f\n",parms[19],radar->vnoise_power);
      printf("%-28s %f\n",parms[13],radar->receiver_gain[0]);
      printf("%-28s %f\n",parms[14],radar->receiver_gain[1]);
      printf("%-28s %f\n",parms[15],radar->receiver_gain[2]);
      printf("%-28s %f\n",parms[16],radar->receiver_gain[3]);
      printf("%-28s %f\n",parms[17],radar->receiver_gain[4]);
      printf("%-28s %f\n",parms[18],radar->receiver_gain[5]);
//     printf("%-28s %f\n",parms[17],radar->vreceiver_gain);
      printf("%-28s %f\n",parms[19],radar->data_sys_sat);
      printf("%-28s %f\n",parms[20],radar->antenna_gain);
      printf("%-28s %f\n",parms[28],radar->vantenna_gain);
      printf("%-28s %f\n",parms[21],radar->horz_beam_width);
      printf("%-28s %f\n",parms[22],radar->vert_beam_width);
      printf("%-28s %f\n",parms[23],radar->xmit_pulsewidth);
      printf("%-28s %f\n","radar constant",radar->rconst);
      printf("%-28s %d\n","year",radar->year);
      printf("%-28s %f\n",parms[32],radar->E_plane_angle);
      printf("%-28s %f\n",parms[33],radar->H_plane_angle);
      printf("%-28s %f\n",parms[34],radar->antenna_rotation_angle);
      printf("%-28s %f\n",parms[35],radar->latitude);
      printf("%-28s %f\n",parms[36],radar->longitude);
      printf("%-28s %f\n",parms[37],radar->altitude);
      printf("%-28s %f\n",parms[38],radar->i_offset);
      printf("%-28s %f\n",parms[39],radar->q_offset);
      printf("%-28s %f\n",parms[40],radar->zdr_bias);
	  printf("%-28s %f\n",parms[41],radar->noise_phase_offset);

//      printf("text:\n%s\n",radar->text);
      strncpy(line,radar->channel_name,PX_MAX_CHANNEL_NAME);  line[PX_MAX_CHANNEL_NAME] = 0;
      printf("%-28s %s\n",parms[42],line);
      strncpy(line,radar->project_name,PX_MAX_PROJECT_NAME);  line[PX_MAX_PROJECT_NAME] = 0;
      printf("%-28s %s\n",parms[43],line);
      strncpy(line,radar->operator_name,PX_MAX_OPERATOR_NAME);  line[PX_MAX_OPERATOR_NAME] = 0;
      printf("%-28s %s\n",parms[44],line);
      strncpy(line,radar->site_name,PX_MAX_SITE_NAME);  line[PX_MAX_SITE_NAME] = 0;
      printf("%-28s %s\n",parms[45],line);
      strncpy(line,radar->desc,PX_MAX_SITE_NAME);  line[PX_MAX_RADAR_DESC] = 0;
      printf("%-28s %s\n",parms[46],line);
      printf("text:\n%s\n",radar->text);
      exit(0);
      }
   if(err)      exit(-1);
   }    
   

