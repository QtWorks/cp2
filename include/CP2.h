#ifndef CP2_H
#define CP2_H

//	CP2.h: definitions related to the CP2 Radar Processor; used by any CP2 project

//	CP2_PIRAQ_DATA_TYE defines the type, and thus the size, of the data that is sent in a block after the header.
#define CP2_PIRAQ_DATA_TYPE			float
#define	QTDSP_BASE_PORT				21010	//	QtDSP base port# for broadcast
#define	CP2EXEC_BASE_PORT			3100	//	CP2exec base port# for receive data
#define CP2_DATA_CHANNELS			4		//	two channels each for S and X band 

#define PIRAQ3D_SCALE	1.0/(unsigned int)pow(2,31)	//	normalization for PIRAQ 2^31 data to +/-1.0 full scale, using multiplication
#define	SCALE_DATA						//	scale data to to +/-1.0	

#define	UDPSENDSIZE	(unsigned int)65536	//	size of N-hit packets in system: PIRAQ through Qt applications

enum	CP2DataChannels	{	SBAND_H,	SBAND_V,	XBAND_H,	XBAND_V,	CP2DATA_CHANNELS	};	//	rename this

#endif