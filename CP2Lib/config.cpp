/* config.cpp: reads config.dsp and config.gph parameters into global structures CONFIG, DISPLAY */ 

//#include <Afx.h>

#include        <stdio.h>
#include	<stdlib.h>
#include        <string.h>
#include        <errno.h>

#include "config.h"
#include "filesub.h"

/* define GLOBAL to define status register 1 global variable */
#define         GLOBAL

#define STOPDELAY 30

//#define COUNTFREQ 10000000ll

#define REF     100e3

int _create_physical_mapping(unsigned long *linear, int *segment, unsigned long physaddr, int size);

char    *parms[]   = {"gates"        ,"hits"         ,"prt"           ,
		      "rcvr_pulsewidth"   ,"gate0mode"    ,"clutterfilter" ,
		      "phasecorrect"      ,"timeseries"   ,"ts_start_gate" ,
		      "timingmode"        ,"delay"        ,"debug"         ,

		      "outfilename"       ,"outpath"      ,"afcgain"       ,
		      "afchigh"           ,"afclow"       ,"locktime"      ,
		      "xmit_pulsewidth"   ,"velsign"      ,"prt2"          ,
		      "trigger"           ,"tpwidth"      ,"tpdelay"       ,

		      "intenable"         ,"freq"         ,"sync"          ,
		      "ethernet"          ,"dataformat"   ,"boardnum"      ,
		      "testpulse"         ,"ts_end_gate"  ,"startgate"	  ,
			  "clutter_start"     ,"clutter_end"  ,"meters_to_first_gate",
			  "gate_spacing_meters",	"pcitimermode",           ""};

/* read in config.dsp and set the parameters in the config structure */
/* if fname is NULL string, default file will be loaded. */
void readconfig(char *fname, CONFIG *config)
   {
   int  linenum,debug,i,err=0;
   double       temp;
   char filename[180],keyword[180],value[180],line[180];
   FILE *fp;

   /* initialize all parameters in the operation state structure */
   config->prt = config->timingmode
	= config->delay = config->hitcount = 0;
   config->gatesa = config->gatesb = config->hits = config->rcvr_pulsewidth 
	= config->timeseries = config->gate0mode = config->ts_start_gate
	= config->pcorrect = config->xmit_pulsewidth = config->prt2 = 0;
   config->clutterfilter = -1; // default disable clutter filter
   config->clutter_start = config->clutter_end = 0; 
   config->ts_end_gate = 0; 
   config->startgate = 0; 
   config->sync = 1;
   config->stalofreq = 0;   /* default stalo frequency */
   config->trigger = 3;
   config->testpulse = 0;
   config->delay = config->tpdelay = config->tpwidth = config->intenable = 0;
   config->afcgain = config->afchi = config->afclo = 0.0;
   config->locktime = 10.0;
   config->outfilename[0] = config->outpath[0] = 0;
   config->velsign = 1;   
   config->ethernet = 0;
   config->boardnum = 0;
   config->pcitimermode = 0;

   fp = 0;
   filename[0] = 0;
   strcat(filename, fname);
   if(strcmp(fname,""))        
      {
      fp = fopen(filename,"r");
      }

   if(!fp) {printf("cannot open configuration file %s\n",filename); exit(0);}

   linenum = 0;
   while(fgets(line,170,fp) != NULL)  /* while not end of file */
    {
    linenum++;
    getkeyval(line,keyword,value);  /* get keyword and value */
    if(strlen(keyword) != 0 && keyword[0] != ';')       /* if line is not a comment line or blank */  
      {
      switch(parse(keyword,parms))
	 {
	 case 0:        /* gates */
			set(value,"%d",&config->gatesa,keyword,linenum,filename);
			break;
	 case 1:        /* hits */
			set(value,"%d",&config->hits,keyword,linenum,filename);
			break;
	 case 2:        /* prt */
			set(value,"%d",&config->prt,keyword,linenum,filename);
			printf("readconfig(): config->prt = %d\n", config->prt); 
			config->prt = config->prt; // ENTER value in 6/8MHz timebase: for IF 48/64mMHz
			break;
	 case 3:        /* receiver pulsewidth: entered in config.dsp in timebase counts */
			set(value,"%d",&config->rcvr_pulsewidth,keyword,linenum,filename);
			break;
	 case 4:        /* GATE0MODE */
			set(value,"%q",&config->gate0mode,keyword,linenum,filename);
			break;
	 case 5:        /* clutter filter */
			set(value,"%q",&config->clutterfilter,keyword,linenum,filename);
			break;
	 case 6:        /* phase correction for magnetron */
			set(value,"%q",&config->pcorrect,keyword,linenum,filename);
			break;
	 case 7:       /* time series */
			set(value,"%q",&config->timeseries,keyword,linenum,filename);
			break;
	 case 8:        /* ts_start_gate: time series first gate */
			set(value,"%d",&config->ts_start_gate,keyword,linenum,filename);
			break;
	 case 9:        /* timingmode */
			set(value,"%d",&config->timingmode,keyword,linenum,filename);
			break;
	 case 10:        /* delay */
			set(value,"%d",&config->delay,keyword,linenum,filename);
			break;
	 case 11:        /* debug */
			set(value,"%q",&debug,keyword,linenum,filename);
			break;
	 case 12:        /* output filename */
			set(value,"%s",(int *)config->outfilename,keyword,linenum,filename);
			break;
	 case 13:        /* output filename path */
			set(value,"%s",(int *)config->outpath,keyword,linenum,filename);
			break;
	 case 14:        /* afc integrater gain */
			set(value,"%f",(int *)&config->afcgain,keyword,linenum,filename);
			break;
	 case 15:        /* afc highest voltage */
			set(value,"%f",(int *)&config->afchi,keyword,linenum,filename);
			break;
	 case 16:        /* afc lowest voltage */
			set(value,"%f",(int *)&config->afclo,keyword,linenum,filename);
			break;
	 case 17:        /* afc lowest voltage */
			set(value,"%f",(int *)&config->locktime,keyword,linenum,filename);
			break;
	 case 18:        /* transmitter pulsewidth */
			set(value,"%d",(int *)&config->xmit_pulsewidth,keyword,linenum,filename);
			break;
	 case 19:        /* velocity sign */
			set(value,"%d",(int *)&config->velsign,keyword,linenum,filename);
			if(config->velsign > -1) config->velsign =  1;
			if(config->velsign < -1) config->velsign = -1;
			break;
	 case 20:        /* prt2: second prt for dual prt */
			set(value,"%d",(int *)&config->prt2,keyword,linenum,filename);
			config->prt2 = config->prt2; // ENTER value in 6/8MHz timebase: for IF 48/64mMHz
			break;
	 case 21:        /* trigger */
			set(value,"%q",&config->trigger,keyword,linenum,filename);
			if(config->trigger == 1) config->trigger = 6;  /* both */                      
			config->trigger >>= 1;
			if(config->trigger == 1 || config->trigger == 2) config->trigger ^= 3;
			break;
	 case 22:        /* tpwidth */
			set(value,"%d",&config->tpwidth,keyword,linenum,filename);
			config->tpwidth = config->tpwidth; // ENTER value in 6/8MHz timebase: for IF 48/64mMHz
			break;
	 case 23:       /* tpdelay  */
			set(value,"%d",&config->tpdelay,keyword,linenum,filename);
			config->tpdelay = config->tpdelay; // ENTER value in 6/8MHz timebase: for IF 48/64mMHz
			break;
	 case 24:       /* intenable */
			set(value,"%d",&config->intenable,keyword,linenum,filename);
			break;
	 case 25:        /* freq (IF frequency) */
			set(value,"%lf",&temp,keyword,linenum,filename);
			config->stalofreq = temp;
			break;
	 case 26:       /* sync  */
			set(value,"%d",&config->sync,keyword,linenum,filename);
			break;
	 case 27:       /* ethernet  */
			set(value,"%x",&config->ethernet,keyword,linenum,filename);
			break;
	 case 28:       /* dataformat */
			set(value,"%d",&config->dataformat,keyword,linenum,filename);
			break;
	 case 29:       /* boardnum */
			set(value,"%d",&config->boardnum,keyword,linenum,filename);
			break;
	 case 30:       /* testpulse */
			set(value,"%q",&config->testpulse,keyword,linenum,filename);
			if(config->testpulse == 1) config->testpulse = 6;  /* both */                      
			config->testpulse >>= 1;
			if(config->testpulse == 1 || config->testpulse == 2) config->testpulse ^= 3;
			break;
	 case 31:        /* ts_end_gate: time series last gate */
			set(value,"%d",&config->ts_end_gate,keyword,linenum,filename);
			break;
	 case 32:        /* startgate: first gate w/valid data */
			set(value,"%d",&config->startgate,keyword,linenum,filename);
			break;
	 case 33:        /* clutter_start: first gate to apply clutter filter */
			set(value,"%d",&config->clutter_start,keyword,linenum,filename);
			break;
	 case 34:        /* clutter_end: last gate to apply clutter filter */
			set(value,"%d",&config->clutter_end,keyword,linenum,filename);
			break;
	 case 35:        /* meters to first gate */
			set(value,"%f",(int *)&config->meters_to_first_gate,keyword,linenum,filename);
			break;
	 case 36:        /* gate spacing meters */
			set(value,"%f",(int *)&config->gate_spacing_meters,keyword,linenum,filename);
			break;
	 case 37:        /* pci timer card timing mode */
			set(value,"%d",(int *)&config->pcitimermode,keyword,linenum,filename);
			break;

	 default:
		printf("unrecognized keyword \"%s\" in line %d of %s\n",keyword,linenum,filename);
		err++;
		break;
	 }
      }
     }
   
   fclose(fp);
   config->gatesb = config->gatesa;

   /* check to see if the dataformat is supported */
   if(config->dataformat > DATA_TYPE_MAX)
      {
      printf("unrecognized dataformat %d DATA_TYPE_MAX = %d\n",config->dataformat, DATA_TYPE_MAX);
      err++;
   }
   if(config->gatesa % 2)
      {
      printf("\ngates = %d: even # gates required.\n", config->gatesa);
//      exit(0);
   }
   if((!config->prt2) || (config->dataformat != PIRAQ_ABPDATA_STAGGER_PRT)) {
	   // prt2 not specified, or not used 
      config->prt2 = config->prt; 
   } 
   if(config->dataformat == PIRAQ_ABPDATA_STAGGER_PRT) { // Staggered PRT: no clutterfilter
      config->clutterfilter = 0;
   } 

   /* compute afclo and afchi if none are given */
   if(config->afclo == 0.0 && config->afchi == 0.0)
      {
      config->afchi = config->stalofreq + config->velsign * 145.0E6;
      config->afclo = config->stalofreq + config->velsign * 120.0E6;
      }

   /* put the two afc frequencies in order */
   if(config->afclo > config->afchi)     /* arrange hi and low so lo < hi */
      {
      temp = config->afclo;
      config->afclo = config->afchi;
      config->afchi = temp;
      config->afcgain = -config->afcgain;  /* maintain sign on gain */
      }
   
   /* check specified afc bounds */
//  if(config->afclo < config->stalofreq + config->velsign * 145.001E6 &&
//      config->afclo < config->stalofreq + config->velsign * 119.999E6)
//      {printf("AFCLOW is out of bounds\n"); err++;}
//   if(config->afchi > config->stalofreq + config->velsign * 145.001E6 &&
//      config->afchi > config->stalofreq + config->velsign * 119.999E6)
//      {printf("AFCHIGH is out of bounds\n"); err++;}

   /* keep "tsgate" in bounds */
   if(config->ts_start_gate >= config->gatesa)  config->ts_start_gate = config->gatesa - 1;
   
   /* fill in the blanks for the user */
   if(config->rcvr_pulsewidth && !config->xmit_pulsewidth)
      config->xmit_pulsewidth = config->rcvr_pulsewidth;

   if(debug)
      {
      printf("\nDEBUG: %s\n",filename);
      printf("%-18s %d\n",parms[0],config->gatesa);
      printf("%-18s %d\n",parms[1],config->hits);
      printf("%-18s %d\n",parms[2],config->prt);
      printf("%-18s %d\n",parms[3],config->rcvr_pulsewidth);
      printf("%-18s %d\n",parms[4],config->gate0mode);
      printf("%-18s %d\n",parms[5],config->clutterfilter);
      printf("%-18s %d\n",parms[6],config->pcorrect);
      printf("%-18s %d\n",parms[7],config->timeseries);
      printf("%-18s %d\n",parms[8],config->ts_start_gate);
      printf("%-18s %d\n",parms[9],config->timingmode);
      printf("%-18s %d\n",parms[10],config->delay);
      printf("%-18s %s\n",parms[12],config->outfilename);
      printf("%-18s %s\n",parms[13],config->outpath);
      printf("%-18s %f\n",parms[14],config->afcgain);
      printf("%-18s %f\n",parms[15],config->afchi);
      printf("%-18s %f\n",parms[16],config->afclo);
      printf("%-18s %f\n",parms[17],config->locktime);
      printf("%-18s %d\n",parms[18],config->xmit_pulsewidth);
      printf("%-18s %d\n",parms[19],config->velsign);
      printf("%-18s %d\n",parms[20],config->prt2);
      printf("%-18s %d\n",parms[21],config->trigger);
      printf("%-18s %d\n",parms[22],config->tpwidth);
      printf("%-18s %d\n",parms[23],config->tpdelay);
      printf("%-18s %d\n",parms[24],config->intenable);
      printf("%-18s %f\n",parms[25],config->stalofreq);
      printf("%-18s %d\n",parms[26],config->sync);
      printf("%-18s %08X\n",parms[27],config->ethernet);
      printf("%-18s %d\n",parms[28],config->dataformat);
      printf("%-18s %d\n",parms[29],config->boardnum);
      printf("%-18s %d\n",parms[30],config->testpulse);
      printf("%-18s %d\n",parms[31],config->ts_end_gate);
      printf("%-18s %d\n",parms[32],config->startgate);
      printf("%-18s %d\n",parms[33],config->clutter_start);
      printf("%-18s %d\n",parms[34],config->clutter_end);
      printf("%-18s %f\n",parms[35],config->meters_to_first_gate);
      printf("%-18s %f\n",parms[36],config->gate_spacing_meters);
      }
   
   if(err) exit(-1);
   }    




