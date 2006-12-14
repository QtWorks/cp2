#ifndef CP2SCOPE_H
#define CP2SCOPE_H

#include <winsock2.h>		//	no redefinition errors if before Qt includes?
#include <qsocketdevice.h> 
#include <qsocketnotifier.h>
#include <qevent.h>

#include <fftw3.h>

#include "MomentsCompute.hh"
#include "MomentsMgr.hh"
#include "Params.hh"
#include "Pulse.hh"
#include <deque>


#include "../include/CP2.h"			//	CP2-wide Definitions
#include "../include/piraqx.h"		//	CP2-piraqx Definitions
#include "../include/dd_types.h"	//	CP2-piraqx data types
#include "CP2ScopeBase.h"
#include <ScopePlot/ScopePlot.h>

#define	CP2SCOPE_DISPLAY_REFRESH_INTERVAL	50		//	mSec display refresh 
#define CP2_DATA_CHANNELS			4		//	two channels each for S and X band 
#define	PIRAQ_NORMALIZED_FULL_SCALE	1.0		//	PIRAQ data normalized to +/-1.0
#define	PIRAQ_HALF_SCALE	pow(2,30)		//	PIRAQ data unnormalized: /2 keep int positive 
#define	ySCALEMIN					0.0
#define	ySCALEMAX					PIRAQ_NORMALIZED_FULL_SCALE
#define	DISPLAY_DECIMATION			20		//	#updates/sec
#ifdef SCALE_DATA
#define	ySCALEMAX					PIRAQ_NORMALIZED_FULL_SCALE
#else
#define	ySCALEMAX					PIRAQ_HALF_SCALE
#endif

enum	{	DATA_SET_PULSE,	DATA_SET_GATE,	DATA_SETS	}; 
//	product types:
enum	{	SVHVP,	SVHHP,	SVEL,	SNCP,	SWIDTH,	SPHIDP,	VREFL,	HREFL,	ZDR	}; 


class CP2Scope : public CP2ScopeBase {
	Q_OBJECT		// per Qt documentation: "strongly recommended"
public:
	CP2Scope();
	~CP2Scope();
	void initializeSocket(); 
	void terminateSocket(); 
	void connectDataRcv();
	void ChangeDatagramPort(int);
	double powerSpectrum(); 
	int	getParameters( CP2_PIRAQ_DATA_TYPE[] );		//	get program operating parameters from piraqx (or other) "housekeeping" header structure
	void displayData(); 
	void resizeDataVectors(); 

public slots:
    virtual void plotTypeSlot(bool);

	// Call when data is available on the data socket.
	void dataSocketActivatedSlot(
		int socket         // File descriptor of the data socket
		);
    virtual void yScaleKnob_valueChanged( double );	
//	virtual void DatagramPortSpinBox_valueChanged( int ); 
    virtual void dataSetSlot(bool);
	virtual void DataSetGateSpinBox_valueChanged( int ); 
	virtual void xFullScaleBox_valueChanged( int );	
	virtual void DataChannelSpinBox_valueChanged( int ); 

protected:
	QSocketDevice*   m_pDataSocket;
	QSocketNotifier* m_pDataSocketNotifier;
	int				m_dataGramPort;
	int				m_datagramPortBase;	//	
	int				m_dataChannel;		//	board source of data CP2 PIRAQ 1-3 
	int				m_DataSetGate;		//	gate in packet to display 
	int				m_packetCount;		//	cumulative packet count on current socket 
	int				m_dataDisplayTimer; 
	unsigned int	m_pulseDisplayDecimation;	//	decimation factor for along range (DATA_SET_PULSE) display: currently set 50
	unsigned int	m_productsDisplayDecimation;	//	decimation factor for products display: currently set 50
	double			m_display_yScale;	//	y-scale factor for plotting timeseries data
	double			m_yScaleMin, m_yScaleMax;	//	y-scale min, max	
	double			m_xFullScale;				//	x-scale max

	std::vector<double> I;		//	timeseries arrays for display
	std::vector<double> Q;
	std::vector<double> ProductData;	//	use array for displaying S-band V, H, velocity, reflectivity, etc.

	CP2_PIRAQ_DATA_TYPE*   m_pSocketBuf;
	CP2_PIRAQ_DATA_TYPE* SABP;	//	S-band ABP data packet generated from VHVH alternating pulses

	void timerEvent(QTimerEvent *e);
   /// Plot type
//   enum PLOTTYPE {TIMESERIES=0, IVSQ=1, SPECTRUM=2, PRODUCT=3};
   /// Product type
   enum PRODUCTTYPE {SVHVP=0, SVHHP=1};

	ScopePlot::PLOTTYPE _plotType;
	int				_productType;
	int				_dataSet;		//	data grouping for scope displays: along pulse, or gate
	unsigned int	_dataSetSize;	//	size of data vector for display or calculations 
	int				_parametersSet;	//	set when piraqx parameters successfully initialized from received data
	//	parameters from piraqx (or other header) "housekeeping" for use by program: data types per piraqx.h
	uint4			_gates;
	uint4			_hits;
	uint4			_dataformat;	//	timeseries or 1 of 3 ABP types
    float4			_prt;			//	pulse repetition time in seconds
	//	parameters from piraqx for power calculations
	float4			_data_sys_sat;	//	receiver saturation power in dBm
	float4			_receiver_gain;		//	horizontal receiver gain
	float4			_vreceiver_gain;	//	vertical receiver gain
	float4			_noise_power;	//	noise power horizontal and vertical channels
	float4			_vnoise_power;	
	//	parameters from piraqx for velocity calculations
	float4			_frequency;		//	radar transmit frequency
	float4			_phaseoffset;	//	phase offset between V and H channels
	//	parameters from piraqx for reflectivity calculations
	float4			_rconst;		//	configured radar constant
	float4			_xmit_pulsewidth;	//	 
	float4			_rcvr_pulsewidth;	//	 
	float4			_peak_power;	//	  
	float4			_xmit_power;	//	  
	float4			_vxmit_power;	//	  
	float4			_antenna_gain;	//	  
	float4			_vantenna_gain;	//	  
	float4			_zdr_fudge_factor;	//	  
	float4			_zdr_bias;	//	 
	float4			_noise_phase_offset;	//	offset to provide noise immunity in velocity estimation 
	//	radar constants for H,V computed at runtime
	float4			_v_rconst;		//	computed V radar constant
	float4			_h_rconst;		//	computed H radar constant
	//	operating parameters derived from piraqx and N-hit implementation:	
	unsigned int	_PNOffset;		//	offset in bytes to pulsenumber from begin of CP2exec-generated packet
	unsigned int	_IQdataOffset;	//	offset to begin of data in (piraqx) packet
	int				_pulseStride;	//	length in bytes of 1 pulse: header + data
	int				_Nhits;			//	

	/// The power spectrum 
	std::vector<double> _spectrum;

	/// The hamming window coefficients
	std::vector<double> _hammingCoefs;

	///	The fftw plan. This is a handle used by
	///	the fftw routines.
	fftw_plan _fftwPlan;

	///	The fftw data array. The fft will
	//	be performed in place, so both input data 
	///	and results are stored here.
	fftw_complex* _fftwData;

	/// Set true if the Hamming window should be applied
	bool _doHamming;

	/// the current minimum scale
//	double _scaleMin;

	/// The current maximum scale
//	double _scaleMax;

	/// The nominal data sample rate in sample per second.
//	double _sampleRateHz;

	/// The tuning frequency in Hz
//	double _tuningFreqHz;
	//	fixed block size for initial cut: 
	unsigned int m_fft_blockSize;

	//	power correction factor applied to (uncorrected) powerSpectrum() output
	double	_powerCorrection;	// approximate power correction to dBm 

	MomentsCompute _momentsCompute;

	int _countSinceBeam;

	double _az;

	int _nGatesOut;

	MomentsMgr* _momentsMgr;

	Params _params;

	std::deque<Pulse *> _pulseQueue;

    
	int Run();

};
#endif
