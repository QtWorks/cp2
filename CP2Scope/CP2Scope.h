#ifndef CP2SCOPE_H
#define CP2SCOPE_H

#include <winsock2.h>		//	no redefinition errors if before Qt includes?
#include <qsocketdevice.h> 
#include <qsocketnotifier.h>
#include <qevent.h>

#include <fftw3.h>

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
#ifdef SCALE_DATA
#define	ySCALEMAX					PIRAQ_NORMALIZED_FULL_SCALE
#else
#define	ySCALEMAX					PIRAQ_HALF_SCALE
#endif

enum	{	DATA_SET_PULSE,	DATA_SET_GATE,	DATA_SETS	}; 

class CP2Scope : public CP2ScopeBase {
	Q_OBJECT		// per Qt documentation: "strongly recommended"
public:
	CP2Scope();
	~CP2Scope();
	void initializeSocket(); 
	void terminateSocket(); 
	void connectDataRcv();
	double powerSpectrum(); 
	int	getParameters( CP2_PIRAQ_DATA_TYPE[] );		//	get program operating parameters from piraqx (or other) "housekeeping" header structure

	void displayData(); // remove when SLOT functions 

public slots:
    virtual void plotTypeSlot(bool);

	// Call when data is available on the data socket.
	void dataSocketActivatedSlot(
		int socket         // File descriptor of the data socket
		);
	void displayDataSlot(void); 
    virtual void yScaleKnob_valueChanged( double );	
	virtual void DatagramPortSpinBox_valueChanged( int ); 
    virtual void dataSetSlot(bool);
	virtual void DataSetGateSpinBox_valueChanged( int ); 

protected:
	QSocketDevice*   m_pDataSocket;
	QSocketNotifier* m_pDataSocketNotifier;
	int				m_dataGramPort;
	int				m_DataSetGate;		//	gate in packet to display 
	int				m_packetCount;		//	cumulative packet count on current socket 
	int				m_dataDisplayTimer; 
	double			m_display_yScale;	//	y-scale factor for plotting timeseries data
	double			m_yScaleMin, m_yScaleMax;	//	y-scale min, max

	std::vector<double> I;	//	single-pulse timeseries arrays; names need decoration throughout
	std::vector<double> Q;
	CP2_PIRAQ_DATA_TYPE*   m_pSocketBuf;
	void timerEvent(QTimerEvent *e);

	ScopePlot::PLOTTYPE _plotType;
	int				_dataSet;		//	data grouping for scope displays: along pulse, or gate
	int				_parametersSet;	//	set when piraqx parameters successfully initialized from received data
	//	operating parameters from piraqx (or other header) for use by program: data types per piraqx.h
	uint4			_gates;
	//	operating parameters derived from piraqx and N-hit implementation:	
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

};
#endif
