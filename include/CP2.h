#ifndef CP2_H
#define CP2_H

//	CP2.h: definitions related to the CP2 Radar Processor; used by any CP2 project
#include <math.h>

//	CP2_PIRAQ_DATA_TYE defines the type, and thus the size, of the data that is sent in a block after the header.
#define CP2_PIRAQ_DATA_TYPE			float
#define	QTDSP_BASE_PORT				21010	//	QtDSP base port# for broadcast
#define	CP2EXEC_BASE_PORT			3100	//	CP2exec base port# for receive data
#define CP2_DATA_CHANNELS			4		//	two channels each for S and X band 

#define PIRAQ3D_SCALE	1.0/(unsigned int)pow(2,31)	//	normalization for PIRAQ 2^31 data to +/-1.0 full scale, using multiplication
#define	SCALE_DATA						//	scale data to +/-1.0	
#define	NOISE_FLOOR -70.0 

#define	UDPSENDSIZEMAX	(unsigned int)65536	//	max size of N-hit packets in system: PIRAQ through Qt applications

enum	CP2DataChannels	{	SBAND_H,	SBAND_V,	XBAND_H,	XBAND_V,	CP2DATA_CHANNELS	};	//	rename this
enum	CP2RadarTypes	{	SVH,		XH,			XV,			CP2RADARTYPES	};	//	CP2 Radar Types

//	S-band product types:
enum	{	SVHVP,	SVHHP,	SVEL,	SNCP,	SWIDTH,	SPHIDP,	VREFL,	HREFL,	ZDR	}; 

//	CP2 data format definitions
#define	PIRAQ_CP2_TIMESERIES	18	/* Simple timeseries: CP2 project */
#define	PIRAQ_CP2_SVHABP		19	/* S-band VH ABP: CP2 project */
#define	PIRAQ_CP2_XHABP			20	/* X-band H : CP2 project */
#define	PIRAQ_CP2_XVABP			21	/* X-band V : CP2 project */

#define RECORDSIZE_MAX	16384	//	~< 2000 gates in CP2
#define RECORDNUM_MAX	1000	//	~2 sec data in CP2 at 1KHz

#define	SVHABP_STRIDE	8		//	#data in 1 gate SVH ABP

#endif