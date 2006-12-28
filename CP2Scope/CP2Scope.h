#ifndef CP2SCOPE_H
#define CP2SCOPE_H

#include <winsock2.h>		//	no redefinition errors if before Qt includes?
#include <qsocketdevice.h> 
#include <qsocketnotifier.h>
#include <qevent.h>
#include <deque>
#include <set>
#include <map>

// the fastest fft in the west; used for power spectrum calcs.
#include <fftw3.h>

// The base class created from the designer .ui specification
#include "CP2ScopeBase.h"

// Coponents from the QtToolbox
#include <ScopePlot/ScopePlot.h>

// CP2 timeseries network transfer protocol.
#include "CP2Net.h"

// CP2 pulse collator
#include "CP2PulseCollator.h"

// PlotInfo knows the characteristics of a plot
#include "PlotInfo.h"

// Clases used in the moments computatons:
#include "MomentsCompute.hh"
#include "MomentsMgr.hh"
#include "Params.hh"
#include "Pulse.hh"

#define	ySCALEMIN					0.0
#define	ySCALEMAX					1.0
#define PIRAQ3D_SCALE	1.0/(unsigned int)pow(2,31)	

enum	{	
	DATA_SET_PULSE,	
	DATA_SET_GATE,	
	DATA_SETS	
}; 

// product types:
enum	PLOTTYPE {	
	S_TIMESERIES,	///< S time series
	XH_TIMESERIES,	///< Xh time series
	XV_TIMESERIES,	///< Xv time series
	S_IQ,			///< S IQ
	XH_IQ,			///< Xh IQ
	XV_IQ,			///< Xv IQ
	S_SPECTRUM,		///< S spectrum 
	XH_SPECTRUM,	///< Xh spectrum 
	XV_SPECTRUM,	///< Xv spectrum 
	S_DBMHC,	///< S-band dBm horizontal co-planar
	S_DBMVC,	///< S-band dBm vertical co-planar
	S_DBZHC,	///< S-band dBz horizontal co-planar
	S_DBZVC,	///< S-band dBz vertical co-planar
	S_SNR,		///< S-band SNR
	S_VEL,		///< S-band velocity
	S_WIDTH,	///< S-band spectral width
	S_RHOHV,	///< S-band rhohv
	S_PHIDP,	///< S-band phidp
	S_ZDR,		///< S-band zdr
	X_DBMHC,	///< X-band dBm horizontal co-planar
	X_DBMVX,	///< X-band dBm vertical cross-planar
	X_DBZHC,	///< X-band dBz horizontal co-planar
	X_SNR,		///< X-band SNR
	X_VEL,		///< X-band velocity
	X_WIDTH,	///< X-band spectral width
	X_LDR		///< X-band LDR
}; 

// types of plots available
enum SCOPEPLOTTYPE {
	TIMESERIES,
	IVSQ,
	SPECTRUM,
	PRODUCT
};


class CP2Scope : public CP2ScopeBase {
	Q_OBJECT
public:
	CP2Scope();
	~CP2Scope();
	void initializeSocket(); 
	void displayData(); 
	void resizeDataVectors(); 

public slots:
	virtual void plotTypeSlot(bool);

	// Call when data is available on the data socket.
	void dataSocketActivatedSlot(
		int socket         ///< File descriptor of the data socket
		);
	virtual void plotTypeSlot(int plotType);
	void tabChangeSlot(QWidget* w);

	virtual void gainChangeSlot( double );	
	virtual void offsetChangeSlot( double );	
	virtual void dataSetSlot(bool);
	virtual void DataSetGateSpinBox_valueChanged( int ); 
	virtual void xFullScaleBox_valueChanged( int );	
	virtual void DataChannelSpinBox_valueChanged( int ); 

protected:
	QSocketDevice*   _pSocket;
	QSocketNotifier* _pSocketNotifier;
	int				_dataGramPort;
	int				_dataChannel;					///<	board source of data CP2 PIRAQ 1-3 
	int				_dataSetGate;					///<	gate in packet to display 
	int				_prevPulseCount[3];			///<	prior cumulative pulse count, used for throughput calcs
	int				_pulseCount[3];				///<	cumulative pulse count
	int				_errorCount[3];				///<	cumulative error count
	bool			_eof[3];						///<    set true when fifo eof occurs. Used so that we don't
	///<    keep setting the fifo eof led.
	unsigned int	_pulseDecimation;		///<	decimation factor for along range (DATA_SET_PULSE) display: currently set 50
	unsigned int	_productsDecimation;	///<	decimation factor for products display: currently set 50
	double          _gain;
	double          _offset;
	double			_scopeGain;						///<	
	double			_scopeOffset;						///<	
	double			_xFullScale;					///<	x-scale max

	std::vector<double> I;		//	timeseries arrays for display
	std::vector<double> Q;
	std::vector<double> _spectrum;
	std::vector<double> _ProductData;	//	use array for displaying products.

	char*   _pSocketBuf;

	// how often to update the statistics (in seconds)
	int _statsUpdateInterval;

	PLOTTYPE        _plotType;
	int				_dataSet;		//	data grouping for scope displays: along pulse, or gate
	unsigned int	_dataSetSize;	//	size of data vector for display or calculations 

	// The builtin timer will be used to calculate beam statistics.
	void timerEvent(QTimerEvent*);

	/// The hamming window coefficients
	std::vector<double> _hammingCoefs;

	///	The fftw plan. This is a handle used by
	///	the fftw routines.
	fftw_plan _fftwPlan;

	///	The fftw data array. The fft will
	//	be performed in place, so both input data 
	///	and results are stored here.
	fftw_complex* _fftwData;

	//	fixed block size for initial cut: 
	unsigned int _fftBlockSize;

	//	power correction factor applied to (uncorrected) powerSpectrum() output
	double	_powerCorrection;	///< approximate power correction to dBm 

	/// Set true if the Hamming window should be applied
	bool _doHamming;

	/// Moment computation parameters for S band
	Params _Sparams;

	/// Moment computation parameters for X band
	Params _Xparams;

	/// The S band moment computation engine.  Pulses
	/// are passed to _momentsCompute. It will make a beam 
	/// available when enough beams have been provided.
	MomentsCompute* _momentsSCompute;

	/// The X band moment computation engine.  Pulses
	/// are passed to _momentsCompute. It will make a beam 
	/// available when enough beams have been provided.
	MomentsCompute* _momentsXCompute;

	double _az;

	/// Process the pulse, feeding it to the moments processor
	/// if a product display is requested.
	/// @param pPulse The pulse to be processed. 
	void processPulse(CP2Pulse* pPulse);

	/// copy the selected product from the beam moments
	/// into the _productData vector. _plotType will determine
	/// which field to use from the moments.
	/// @param pBeam The beam containing the moment results
	/// @param gates The number of gates
	void getProduct(Beam* pBeam, int gates);

	/// Counter of time series, used for decimating 
	/// the timeseries (and I/Q and Spectrum)
	/// display updates.
	int _tsDisplayCount; 

	/// Counter of product cacluations, used
	/// for decimating the product display updates.
	int _productDisplayCount; 

	/// Compute the power spectrum. The input values will come
	/// I[]and Q[], the power spectrum will be written to 
	/// _spectrum[]
	/// @return The zero moment
	double powerSpectrum();

	/// Collator collects and matches time tags
	/// from H and V Xband channels
	CP2PulseCollator _collator;
	std::vector<CP2FullPulse*> _xHPulses;
	std::vector<CP2FullPulse*> _xVPulses;

	/// For each PLOTTYPE, there will be an entry in this map.
	std::map<PLOTTYPE, PlotInfo> _plotInfo;

	/// This set contains PLOTTYPEs for all timeseries plots
	std::set<PLOTTYPE> _timeSeriesPlots;

	/// This set contains PLOTTYPEs for all raw data plots
	std::set<PLOTTYPE> _rawPlots;

	/// This set contains PLOTTYPEs for all S band moments plots
	std::set<PLOTTYPE> _sMomentsPlots;

	/// This set contains PLOTTYPEs for all X band moments plots
	std::set<PLOTTYPE> _xMomentsPlots;

	/// save the button group for each tab,
	/// so that we can find the selected button
	/// and change the plot type when tabs are switched.
	std::vector<QButtonGroup*> _tabButtonGroups;

	/// initialize all of the book keeping structures
	/// for the various plots.
	void initPlots();
	/// add a tab to the plot type selection tab widget.
	/// Radio buttons are created for all of specified
	/// plty types, and grouped into one button group.
	/// _plotInfo provides the label information for
	/// the radio buttons.
	/// @param tabName The title for the tab.
	/// @param types A set of the desired PLOTTYPE types 
	/// @return The button group that the inserted buttons
	/// belong to.
	QButtonGroup* addPlotTypeTab(std::string tabName, std::set<PLOTTYPE> types);

};

#endif
