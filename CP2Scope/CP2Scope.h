#ifndef CP2SCOPE_H
#define CP2SCOPE_H

#include <winsock2.h>		//	no redefinition errors if before Qt includes?
#include <qsocketdevice.h> 
#include <qsocketnotifier.h>
#include <qevent.h>
#include <deque>

// the fastest fft in the west; used for power spectrum calcs.
#include <fftw3.h>

// The base class created from the designer .ui specification
#include "CP2ScopeBase.h"

// Coponents from the QtToolbox
#include <ScopePlot/ScopePlot.h>

// CP2 timeseries network transfer protocol.
#include "CP2Net.h"

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

//	product types:
enum	{	
	SVHVP,	
	SVHHP,	
	SVEL,	
	SNCP,	
	SWIDTH,	
	SPHIDP,	
	VREFL,	
	HREFL,	
	ZDR	
}; 


class CP2Scope : public CP2ScopeBase {
	Q_OBJECT
public:
	CP2Scope();
	~CP2Scope();
	void initializeSocket(); 
	void connectDataRcv();
	void displayData(); 
	void resizeDataVectors(); 

public slots:
	virtual void plotTypeSlot(bool);

	// Call when data is available on the data socket.
	void dataSocketActivatedSlot(
		int socket         ///< File descriptor of the data socket
		);
	virtual void yScaleKnob_valueChanged( double );	
	virtual void dataSetSlot(bool);
	virtual void DataSetGateSpinBox_valueChanged( int ); 
	virtual void xFullScaleBox_valueChanged( int );	
	virtual void DataChannelSpinBox_valueChanged( int ); 

protected:
	QSocketDevice*   m_pDataSocket;
	QSocketNotifier* m_pDataSocketNotifier;
	int				m_dataGramPort;
	int				m_dataChannel;					///<	board source of data CP2 PIRAQ 1-3 
	int				m_DataSetGate;					///<	gate in packet to display 
	int				m_pulseCount;					///<	cumulative pulse count
	unsigned int	m_pulseDisplayDecimation;		///<	decimation factor for along range (DATA_SET_PULSE) display: currently set 50
	unsigned int	m_productsDisplayDecimation;	///<	decimation factor for products display: currently set 50
	double			m_display_yScale;				///<	y-scale factor for plotting timeseries data
	double			m_yScaleMin, m_yScaleMax;		///<	y-scale min, max	
	double			m_xFullScale;					///<	x-scale max

	std::vector<double> I;		//	timeseries arrays for display
	std::vector<double> Q;
	std::vector<double> _spectrum;
	std::vector<double> ProductData;	//	use array for displaying products.

	char*   m_pSocketBuf;

	//   enum PLOTTYPE {TIMESERIES=0, IVSQ=1, SPECTRUM=2, PRODUCT=3};
	ScopePlot::PLOTTYPE _plotType;
	int				_productType;
	int				_dataSet;		//	data grouping for scope displays: along pulse, or gate
	unsigned int	_dataSetSize;	//	size of data vector for display or calculations 

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
	unsigned int m_fft_blockSize;

	//	power correction factor applied to (uncorrected) powerSpectrum() output
	double	_powerCorrection;	///< approximate power correction to dBm 

	/// Set true if the Hamming window should be applied
	bool _doHamming;

	/// The moment computation engine.  Pulses
	/// are passed to _momentsCompute. It will make a beam 
	/// available when enough beams have been provided.
	MomentsCompute _momentsCompute;

	double _az;

	/// Process the pulse, feeding it to the moments processor
	/// if a product display is requested.
	/// @param pPulse The pulse to be processed.
	void processPulse(CP2Pulse* pPulse);

	/// copy the selected product from the beam
	/// into the _productData vector.
	/// @param pBeam The beam containing the moment results
	/// @param gates The number of gates
	/// @param product type
	void getProduct(Beam* pBeam, int gates, int productType);

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

};

#endif
