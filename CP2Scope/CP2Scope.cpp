
#include "CP2Scope.h"
#include <ScopePlot/ScopePlot.h>
#include <Knob/Knob.h>

#include <qbuttongroup.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qspinbox.h>	
#include <qlcdnumber.h>
#include <qslider.h>
#include <qlayout.h>
#include <qtabwidget.h>
#include <qwidget.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qvbox.h>
#include <qframe.h>
#include <qpushbutton.h>
#include <qpalette.h>

#include <algorithm>

#include <winsock.h>
#include <iostream>
#include <time.h>

#include <qwt_wheel.h>

//////////////////////////////////////////////////////////////////////
CP2Scope::CP2Scope():
_pPulseSocket(0),    
_pPulseSocketNotifier(0),
_pPulseSocketBuf(0),	
_tsDisplayCount(0),
_productDisplayCount(0),
_statsUpdateInterval(5),
_rawPlot(TRUE),
_performAutoScale(false),
_dataChannel(0)
{
	_pulseDataPort	= 3100;
	_productDataPort = 3200;

	for (int i = 0; i < 3; i++) {
		_pulseCount[i]	= 0;
		_prevPulseCount[i] = 0;
		_errorCount[i] = 0;
		_eof[i] = false;
		_lastPulseNum[i] = 0;
	}

	_plotType = S_TIMESERIES;

	// intialize the data reception socket.
	// set up the ocket notifier and connect it
	// to the data reception slot
	initSockets();	

	// initialize the book keeping for the plots.
	// This also sets up the radio buttons 
	// in the plot type tab widget
	initPlots();

	_gainKnob->setRange(-5, 5);
	_gainKnob->setTitle("Gain");

	// set the minor ticks
	_gainKnob->setScaleMaxMajor(5);
	_gainKnob->setScaleMaxMinor(5);

	_graphRange = 1;
	_graphCenter = 0.0;
	_knobGain = 0.0;
	_knobOffset = 0.0;

	// set leds to green
	_chan0led->setBackgroundColor(QColor("green"));
	_chan1led->setBackgroundColor(QColor("green"));
	_chan2led->setBackgroundColor(QColor("green"));

	// set the intial plot type
	plotTypeSlot(S_TIMESERIES);

	_xFullScale = 1000;
	I.resize(_xFullScale);			//	default timeseries array size full range at 1KHz
	Q.resize(_xFullScale);
	_dataSetSize = _xFullScale;	//	size of data vector for display or calculations 
	_dataSet = DATA_SET_PULSE;

	//	display decimation, set to get ~50/sec
	_pulseDecimation	= 50;	//	default w/prt = 1000Hz, timeseries data 
	_productsDecimation	= 5;	//	default w/prt = 1000Hz, hits = 10, products data

	_dataSetGate = 50;		//!get spinner value

	//	set up fft for power calculations: 
	_fftBlockSize = 256;	//	temp constant for initial proving 
	// allocate the data space for fftw
	_fftwData  = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * _fftBlockSize);
	// create the plan.
	_fftwPlan = fftw_plan_dft_1d(_fftBlockSize, _fftwData, _fftwData,
		FFTW_FORWARD, FFTW_ESTIMATE);
	//	power correction factor applied to (uncorrected) powerSpectrum() output:
	_powerCorrection = 0.0;	//	use for power correction to dBm

	// start the statistics timer
	startTimer(_statsUpdateInterval*1000);

}
//////////////////////////////////////////////////////////////////////
CP2Scope::~CP2Scope() {
	if (_pPulseSocketNotifier)
		delete _pPulseSocketNotifier;

	if (_pPulseSocket)
		delete _pPulseSocket;

	if (_pProductSocketNotifier)
		delete _pProductSocketNotifier;

	if (_pProductSocket)
		delete _pProductSocket;

}
//////////////////////////////////////////////////////////////////////
void 
CP2Scope::dataSetSlot(bool b)	{
	b = b;  // so we don't get the unreferenced warning
	switch (buttonGroup2->selectedId()) {
		case 0:
			_dataSet = DATA_SET_PULSE;
			//	disable gate spinner
			break;
		case 1:
			_dataSet = DATA_SET_GATE;
			//	enable gate spinner
			break;
		default: {}
	}
	resizeDataVectors();	//	resize data vectors
}


//////////////////////////////////////////////////////////////////////
//	set data array sizes based on plot type, UI x-scale max setting
void CP2Scope::resizeDataVectors()	{	
	//	resize data vectors to x-axis max, not gates: fft size if that is the plot type. 
	if	(_plotType >= ScopePlot::PRODUCT)	{	//	current product to display
		_ProductData.resize(_xFullScale); 
		//	enable x-scale spinner
	}
	else	{	//	timeseries-type or product
		I.resize(_xFullScale);	//	resize timeseries arrays, x-axis
		Q.resize(_xFullScale);
		_dataSetSize = _xFullScale; 
	}
}


//////////////////////////////////////////////////////////////////////
void 
CP2Scope::DataChannelSpinBox_valueChanged( int dataChannel )	
{
	//	change the data channel
	_dataChannel = dataChannel;	
}

//////////////////////////////////////////////////////////////////////
void 
CP2Scope::DataSetGateSpinBox_valueChanged( int SpinBoxGate )	{
	//	change the gate for assembling data sets 
	_dataSetGate    = SpinBoxGate;
}
//////////////////////////////////////////////////////////////////////
void 
CP2Scope::xFullScaleBox_valueChanged( int xFullScale )	{
	//	change the gate for assembling data sets 
	if	(_plotType == ScopePlot::SPECTRUM)	//	fft: disable spinner
		return;
	_xFullScale    = xFullScale;
	resizeDataVectors();	//	resize data vectors
}

//////////////////////////////////////////////////////////////////////
void 
CP2Scope::newProductSlot(int)
{
	CP2Packet packet;
	int	readBufLen = _pProductSocket->readBlock(
		&_pProductSocketBuf[0], 
		_pProductSocketBuf.size());

	if (readBufLen > 0) {
		// put this datagram into a packet
		bool packetBad = packet.setProductData(readBufLen, &_pProductSocketBuf[0]);

		// Extract the products and process them.
		// From here on out, we are divorced from the
		// data transport.
		if (!packetBad) {
			for (int i = 0; i < packet.numProducts(); i++) {
				CP2Product* pProduct = packet.getProduct(i);
				// do all of the heavy lifting for this pulse
				processProduct(pProduct);
			} 
		} else {
			// packet error. What to do?
			int x = 0;
		}
	} else {
		// read error. What to do?
		int e = WSAGetLastError();
	}

}

//////////////////////////////////////////////////////////////////////
void 
CP2Scope::newPulseSlot(int)
{
	CP2Packet packet;
	int	readBufLen = _pPulseSocket->readBlock(
		&_pPulseSocketBuf[0], 
		_pPulseSocketBuf.size());

	if (readBufLen > 0) {
		// put this datagram into a packet
		bool packetBad = packet.setPulseData(readBufLen, &_pPulseSocketBuf[0]);

		// Extract the pulses and process them.
		// Observe paranoia for validating packets and pulses.
		// From here on out, we are divorced from the
		// data transport.
		if (!packetBad) {
			for (int i = 0; i < packet.numPulses(); i++) {
				CP2Pulse* pPulse = packet.getPulse(i);
				if (pPulse) {
					int chan = pPulse->header.channel;
					if (chan >= 0 && chan < 3) {
						// do all of the heavy lifting for this pulse
						processPulse(pPulse);
						// sanity check on channel
						_pulseCount[chan]++;
						if (pPulse->header.status & PIRAQ_FIFO_EOF) {
							switch (chan) 
							{
							case 0:
								if (!_eof[0]) {
									_eof[0] = true;
									_chan0led->setBackgroundColor(QColor("red"));
								}
								break;
							case 1:
								if (!_eof[1]) {
									_eof[1] = true;
									_chan1led->setBackgroundColor(QColor("red"));
								}
								break;
							case 2:
								if (!_eof[2]) {
									_eof[2] = true;
									_chan2led->setBackgroundColor(QColor("red"));
								}
								break;
							}
						}
					} 
				} 
			}
		} 
	} else {
		// read error. What should we do here?
	}
}


//////////////////////////////////////////////////////////////////////
void
CP2Scope::processPulse(CP2Pulse* pPulse) 
{
	int chan = pPulse->header.channel;

	// look for pulse number errors
	if (_lastPulseNum[chan]) {
		if (_lastPulseNum[chan]+1 != pPulse->header.pulse_num) {
			_errorCount[chan]++;
		}
	}
	_lastPulseNum[chan] = pPulse->header.pulse_num;

	float* data = &(pPulse->data[0]);

	PlotInfo* pi = &_plotInfo[_plotType];
	switch (pi->getDisplayType()) 
	{
	case ScopePlot::SPECTRUM:
		{
			if (pPulse->header.channel != _dataChannel)
				break;
			_tsDisplayCount++;
			if	(_tsDisplayCount >= _pulseDecimation)	{	
				_spectrum.resize(_fftBlockSize);	//	probably belongs somewhere else
				for( int j = 0; j < _fftBlockSize; j++)	{
					// transfer the data to the fftw input space
					_fftwData[j][0] = pPulse->data[2*j]*PIRAQ3D_SCALE;
					_fftwData[j][1] = pPulse->data[2*j+1]*PIRAQ3D_SCALE;
				}
				double zeroMoment = powerSpectrum();
				// correct unscaled power data using knob setting: 
				for(j = 0; j < _fftBlockSize; j++)	{
					_spectrum[j] += _powerCorrection;
				}
				displayData();	
				_tsDisplayCount = 0; 
			}
			break;
		}
	case TIMESERIES:
	case IVSQ:
		{
			if (pPulse->header.channel != _dataChannel)
				break;
			_tsDisplayCount++;
			if	(_tsDisplayCount >= _pulseDecimation)	{	
				I.resize(pPulse->header.gates);
				Q.resize(pPulse->header.gates);
				for (int i = 0; i < 2*pPulse->header.gates; i+=2) {	
					I[i/2] = pPulse->data[i]*PIRAQ3D_SCALE;
					Q[i/2] = pPulse->data[i+1]*PIRAQ3D_SCALE;
				}
				displayData();	
				_tsDisplayCount = 0; 
			}
		}
	default:
		// ignore others
		break;
	}
}

//////////////////////////////////////////////////////////////////////
void
CP2Scope::processProduct(CP2Product* pProduct) 
{
	// if we are displaying a raw plot, just ignore
	if (_rawPlot) 
		return;

	PRODUCT_TYPES prodType = pProduct->header.prodType;

	if (prodType == _productType) {
		// this is the product that we are currntly displaying
		// extract the data and display it.
		_ProductData.resize(pProduct->header.gates);
		for (int i = 0; i < pProduct->header.gates; i++) {
			_ProductData[i] = pProduct->data[i];
		}
		displayData();
	}
}

//////////////////////////////////////////////////////////////////////
void
CP2Scope::displayData() 
{
	double yBottom = _graphCenter - _graphRange;
	double yTop =    _graphCenter + _graphRange;
	if (_rawPlot) {
		PlotInfo* pi = &_plotInfo[_plotType];

		switch (pi->getDisplayType())
		{
		case ScopePlot::TIMESERIES:
			if (_performAutoScale)
				autoScale(I, Q);
			_scopePlot->TimeSeries(I, Q, yBottom, yTop, 1);
			break;
		case ScopePlot::IVSQ:
			if (_performAutoScale)
				autoScale(I, Q);
			_scopePlot->IvsQ(I, Q, yBottom, yTop, 1); 
			break;
		case ScopePlot::SPECTRUM:
			_scopePlot->Spectrum(_spectrum, -100, 20.0, 1000000, false, "Frequency (Hz)", "Power (dB)");	
			break;
		}

	} else {
		if (_performAutoScale)
			autoScale(_ProductData);
		PlotInfo* pi = &_prodPlotInfo[_productType];
		_scopePlot->Product(_ProductData, 
			pi->getId(), 
			yBottom, 
			yTop, 
			_ProductData.size());
	}
}

//////////////////////////////////////////////////////////////////////
void
CP2Scope::initSockets()	
{
	int result;

	// allocate the socket read buffers
	_pPulseSocketBuf.resize(1000000);
	_pProductSocketBuf.resize(1000000);

	// creat the sockets
	_pPulseSocket = new QSocketDevice(QSocketDevice::Datagram);
	_pProductSocket = new QSocketDevice(QSocketDevice::Datagram);

	// get the IP name for this machine.
	// hmmm - how does it know which interface to use?
	// This could be a problem
	char nameBuf[1000];
	if (gethostname(nameBuf, sizeof(nameBuf))) {
		qWarning("gethostname failed");
		exit(1);
	}
	//strcpy(nameBuf, "127.0.0.1");
	struct hostent* pHostEnt = gethostbyname(nameBuf);
	if (!pHostEnt) {
		qWarning("gethostbyname failed");
		exit(1);
	}

	std::string myIPname = nameBuf;
	std::string myIPaddress = inet_ntoa(*(struct in_addr*)pHostEnt->h_addr_list[0]);
	m_pTextIPname->setText(myIPname.c_str());
	m_pTextIPaddress->setNum(+_pulseDataPort);	

	// bind sockets to port/network
	QHostAddress qHost;
	qHost.setAddress(myIPaddress.c_str());
	int optval = 1;
	result = setsockopt(_pPulseSocket->socket(), 
		SOL_SOCKET, 
		SO_REUSEADDR, 
		(const char*)&optval, 
		sizeof(optval)); 
	if (!_pPulseSocket->bind(qHost, _pulseDataPort)) {
		qWarning("Unable to bind to %s:%d", qHost.toString().ascii(), _pulseDataPort);
		exit(1); 
	}
	result = setsockopt(_pProductSocket->socket(), 
		SOL_SOCKET, 
		SO_REUSEADDR, 
		(const char*)&optval, 
		sizeof(optval)); 
	if (!_pProductSocket->bind(qHost, _productDataPort)) {
		qWarning("Unable to bind to %s:%d", qHost.toString().ascii(), _productDataPort);
		exit(1); 
	}

	// set the system recevie buffer size
	int sockbufsize = CP2SCOPE_RCVBUF;

	result = setsockopt (_pPulseSocket->socket(),
		SOL_SOCKET,
		SO_RCVBUF,
		(char *) &sockbufsize,
		sizeof sockbufsize);
	if (result) {
		qWarning("Set receive buffer size for socket failed");
		exit(1); 
	}
	result = setsockopt (_pProductSocket->socket(),
		SOL_SOCKET,
		SO_RCVBUF,
		(char *) &sockbufsize,
		sizeof sockbufsize);
	if (result) {
		qWarning("Set receive buffer size for socket failed");
		exit(1); 
	}

	// create notifiers and connect to slots
	_pPulseSocketNotifier = new QSocketNotifier(_pPulseSocket->socket(), QSocketNotifier::Read);
	_pProductSocketNotifier = new QSocketNotifier(_pProductSocket->socket(), QSocketNotifier::Read);

	connect(_pPulseSocketNotifier, SIGNAL(activated(int)), this, SLOT(newPulseSlot(int)));
	connect(_pProductSocketNotifier, SIGNAL(activated(int)), this, SLOT(newProductSlot(int)));

}

//////////////////////////////////////////////////////////////////////
double
CP2Scope::powerSpectrum()
{
	// apply the hamming window to the time series
	//  if (_doHamming)
	//    doHamming();

	// caclulate the fft
	fftw_execute(_fftwPlan);

	double zeroMoment = 0.0;
	int n = _fftBlockSize;

	// reorder and copy the results into _spectrum
	for (int i = 0 ; i < n/2; i++) {
		double pow = 
			_fftwData[i][0] * _fftwData[i][0] +
			_fftwData[i][1] * _fftwData[i][1];

		zeroMoment += pow;

		pow /= n*n;
		pow = 10.0*log10(pow);
		_spectrum[i+n/2] = pow;
	}

	for (int i = n/2 ; i < n; i++) {
		double pow =      
			_fftwData[i][0] * _fftwData[i][0] +
			_fftwData[i][1] * _fftwData[i][1];

		zeroMoment += pow;

		pow /= n*n;
		pow = 10.0*log10(pow);
		_spectrum[i - n/2] = pow;
	}

	zeroMoment /= n*n;
	zeroMoment = 10.0*log10(zeroMoment);

	return zeroMoment;
}
////////////////////////////////////////////////////////////////////
void
CP2Scope::plotTypeSlot(int plotType) {
	// find out the index of the current page
	int pageNum = _typeTab->currentPageIndex();

	// get the radio button id of the currently selected button
	// on that page.
	int ptype = _tabButtonGroups[pageNum]->selectedId();

	if (pageNum == 0) {
		// change to a raw plot type
		PLOTTYPE plotType = (PLOTTYPE)ptype;
		plotTypeChange(&_plotInfo[plotType], plotType, (PRODUCT_TYPES)0 , true);
	} else {
		// change to a product plot type
		PRODUCT_TYPES productType = (PRODUCT_TYPES)ptype;
		plotTypeChange(&_prodPlotInfo[productType], (PLOTTYPE)0, productType , false);
	}
}
//////////////////////////////////////////////////////////////////////
void
CP2Scope::tabChangeSlot(QWidget* w) 
{
	// find out the index of the current page
	int pageNum = _typeTab->currentPageIndex();

	// get the radio button id of the currently selected button
	// on that page.
	int ptype = _tabButtonGroups[pageNum]->selectedId();

	if (pageNum == 0) {
		// change to a raw plot type
		PLOTTYPE plotType = (PLOTTYPE)ptype;
		plotTypeChange(&_plotInfo[plotType], plotType, (PRODUCT_TYPES)0 , true);
	} else {
		// change to a product plot type
		PRODUCT_TYPES productType = (PRODUCT_TYPES)ptype;
		plotTypeChange(&_prodPlotInfo[productType], (PLOTTYPE)0, productType , false);
	}
}

////////////////////////////////////////////////////////////////////
void
CP2Scope::plotTypeChange(PlotInfo* pi, 
					   PLOTTYPE newPlotType, 
					   PRODUCT_TYPES newProductType, 
					   bool rawPlot)
{

	// save the gain and offset of the current plot type
	PlotInfo* currentPi;
	if (_rawPlot) {
		currentPi = &_plotInfo[_plotType];
	} else {
		currentPi = &_prodPlotInfo[_productType];
	}
	currentPi->setGain(pi->getGainMin(), pi->getGainMax(), _knobGain);
	currentPi->setOffset(pi->getOffsetMin(), pi->getOffsetMax(), _graphCenter);

	// restore gain and offset for new plot type
	gainChangeSlot(pi->getGainCurrent());
	_graphCenter = pi->getOffsetCurrent();


	// set the knobs for the new plot type
	_gainKnob->setValue(_knobGain);

	// change the plot type
	if (rawPlot) {
		_plotType = newPlotType;
	} else {
		_productType = newProductType;
	}
	
	_rawPlot = rawPlot;

	if (_rawPlot) {
		// change data channel if necessary
	switch(_plotType) 
	{
	case S_TIMESERIES:	// S time series
	case S_IQ:			// S IQ
	case S_SPECTRUM:		// S spectrum 
		_dataChannel = 0;
		break;
	case XH_TIMESERIES:	// Xh time series
	case XH_IQ:			// Xh IQ
	case XH_SPECTRUM:	// Xh spectrum 
		_dataChannel = 1;
		break;
	case XV_TIMESERIES:	// Xv time series
	case XV_IQ:			// Xv IQ
	case XV_SPECTRUM:	// Xv spectrum 
		_dataChannel = 2;
		break;
	default:
		break;
	}
	}

}

////////////////////////////////////////////////////////////////////
void
CP2Scope::initPlots()
{

	_rawPlots.insert(S_TIMESERIES);
	_rawPlots.insert(XH_TIMESERIES);
	_rawPlots.insert(XV_TIMESERIES);

	_rawPlots.insert(S_IQ);
	_rawPlots.insert(XH_IQ);
	_rawPlots.insert(XV_IQ);

	_rawPlots.insert(S_SPECTRUM);
	_rawPlots.insert(XH_SPECTRUM);
	_rawPlots.insert(XV_SPECTRUM);

	_sMomentsPlots.insert(PROD_S_DBMHC);
	_sMomentsPlots.insert(PROD_S_DBMVC);
	_sMomentsPlots.insert(PROD_S_DBZHC);
	_sMomentsPlots.insert(PROD_S_DBZVC);
	_sMomentsPlots.insert(PROD_S_WIDTH);
	_sMomentsPlots.insert(PROD_S_VEL);
	_sMomentsPlots.insert(PROD_S_SNR);
	_sMomentsPlots.insert(PROD_S_RHOHV);
	_sMomentsPlots.insert(PROD_S_PHIDP);
	_sMomentsPlots.insert(PROD_S_ZDR);

	_xMomentsPlots.insert(PROD_X_DBMHC);
	_xMomentsPlots.insert(PROD_X_DBMVX);
	_xMomentsPlots.insert(PROD_X_DBZHC);
	_xMomentsPlots.insert(PROD_X_SNR);
	//	_xMomentsPlots.insert(PROD_X_WIDTH);
	//	_xMomentsPlots.insert(PROD_X_VEL);
	_xMomentsPlots.insert(PROD_X_LDR);

	_plotInfo[S_TIMESERIES]  = PlotInfo( S_TIMESERIES, TIMESERIES, "I and Q", "S:  I and Q", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XH_TIMESERIES] = PlotInfo(XH_TIMESERIES, TIMESERIES, "I and Q", "Xh: I and Q", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XV_TIMESERIES] = PlotInfo(XV_TIMESERIES, TIMESERIES, "I and Q", "Xv: I and Q", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_IQ]          = PlotInfo(         S_IQ,       IVSQ, "I vs Q", "S:  I vs Q", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XH_IQ]         = PlotInfo(        XH_IQ,       IVSQ, "I vs Q", "Xh: I vs Q", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XV_IQ]         = PlotInfo(        XV_IQ,       IVSQ, "I vs Q", "Xv: I vs Q", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_SPECTRUM]    = PlotInfo(   S_SPECTRUM,   SPECTRUM, "Power Spectrum", "S:  Power Spectrum", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XH_SPECTRUM]   = PlotInfo(  XH_SPECTRUM,   SPECTRUM, "Power Spectrum", "Xh: Power Spectrum", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XV_SPECTRUM]   = PlotInfo(  XV_SPECTRUM,   SPECTRUM, "Power Spectrum", "Xv: Power Spectrum", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);

	_prodPlotInfo[PROD_S_DBMHC]       = PlotInfo(      PROD_S_DBMHC,    PRODUCT, "H Dbm", "Sh: Dbm", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_prodPlotInfo[PROD_S_DBMVC]       = PlotInfo(      PROD_S_DBMVC,    PRODUCT, "V Dbm", "Sv: Dbm", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_prodPlotInfo[PROD_S_DBZHC]       = PlotInfo(      PROD_S_DBZHC,    PRODUCT, "H Dbz", "Sh: Dbz", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_prodPlotInfo[PROD_S_DBZVC]       = PlotInfo(      PROD_S_DBZVC,    PRODUCT, "V Dbz", "Sv: Dbz", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_prodPlotInfo[PROD_S_WIDTH]       = PlotInfo(      PROD_S_WIDTH,    PRODUCT, "Width", "S:  Width", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_prodPlotInfo[PROD_S_VEL]         = PlotInfo(        PROD_S_VEL,    PRODUCT, "Velocity", "S:  Velocity", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_prodPlotInfo[PROD_S_SNR]         = PlotInfo(        PROD_S_SNR,    PRODUCT, "SNR", "S:  SNR", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_prodPlotInfo[PROD_S_RHOHV]       = PlotInfo(      PROD_S_RHOHV,    PRODUCT, "Rhohv", "S:  Rhohv", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_prodPlotInfo[PROD_S_PHIDP]       = PlotInfo(      PROD_S_RHOHV,    PRODUCT, "Phidp", "S:  Phidp", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_prodPlotInfo[PROD_S_ZDR]         = PlotInfo(      PROD_S_ZDR,      PRODUCT, "Zdr", "S:  Zdr", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_prodPlotInfo[PROD_X_DBMHC]       = PlotInfo(      PROD_X_DBMHC,    PRODUCT, "H Dbm", "Xh: Dbm", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_prodPlotInfo[PROD_X_DBMVX]       = PlotInfo(      PROD_X_DBMVX,    PRODUCT, "V Cross Dbm", "Xv: Dbm", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_prodPlotInfo[PROD_X_DBZHC]       = PlotInfo(      PROD_X_DBZHC,    PRODUCT, "H Dbz", "Xh: Dbz", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_prodPlotInfo[PROD_X_SNR]         = PlotInfo(        PROD_X_SNR,    PRODUCT, "SNR", "Xh: SNR", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_prodPlotInfo[PROD_X_WIDTH]       = PlotInfo(      PROD_X_WIDTH,    PRODUCT, "Width", "Xh: Width", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_prodPlotInfo[PROD_X_VEL]         = PlotInfo(        PROD_X_VEL,    PRODUCT, "Velocity", "Xh: Velocity", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_prodPlotInfo[PROD_X_LDR]         = PlotInfo(        PROD_X_LDR,    PRODUCT, "LDR", "Xhv:LDR", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);

	// remove the one tab that was put there by designer
	_typeTab->removePage(_typeTab->page(0));

	// add tabs, and save the button group for
	// for each tab.
	QButtonGroup* pGroup;

	pGroup = addPlotTypeTab("Raw", _rawPlots);
	_tabButtonGroups.push_back(pGroup);

	pGroup = addProductTypeTab("S", _sMomentsPlots);
	_tabButtonGroups.push_back(pGroup);

	pGroup = addProductTypeTab("X", _xMomentsPlots);
	_tabButtonGroups.push_back(pGroup);

	connect(_typeTab, SIGNAL(currentChanged(QWidget *)), 
		this, SLOT(tabChangeSlot(QWidget*)));
}
//////////////////////////////////////////////////////////////////////
QButtonGroup*
CP2Scope::addPlotTypeTab(std::string tabName, std::set<PLOTTYPE> types)
{
	QWidget* pPage;
	QButtonGroup* pGroup;
	QVBoxLayout* pVbox;
	QVBoxLayout* pButtonVbox;
	QRadioButton* pRadio;

	pPage = new QWidget(_typeTab, tabName.c_str());
	pVbox = new QVBoxLayout(pPage);

	pGroup = new QButtonGroup(pPage);
	pGroup->setColumnLayout(0, Qt::Vertical );
	pGroup->layout()->setSpacing( 6 );
	pGroup->layout()->setMargin( 11 );

	pButtonVbox = new QVBoxLayout(pGroup->layout());
	pButtonVbox->setAlignment( Qt::AlignTop );

	std::set<PLOTTYPE>::iterator i;

	for (i = types.begin(); i != types.end(); i++) 
	{
		int id = _plotInfo[*i].getId();
		pRadio = new QRadioButton(pGroup);
		pGroup->insert(pRadio, id);
		const QString label = _plotInfo[*i].getLongName().c_str();
		pRadio->setText(label);
		pButtonVbox->addWidget(pRadio);

		// set the first radio button of the group
		// to be selected.
		if (i == types.begin()) {
			pRadio->setChecked(true);
		}
	}

	pVbox->addWidget(pGroup);
	_typeTab->insertTab(pPage, tabName.c_str());

	// connect the button released signal to our
	// our plot type change slot.
	connect(pGroup, SIGNAL(released(int)), this, SLOT(plotTypeSlot(int)));

	return pGroup;
}
//////////////////////////////////////////////////////////////////////
QButtonGroup*
CP2Scope::addProductTypeTab(std::string tabName, std::set<PRODUCT_TYPES> types)
{
	QWidget* pPage;
	QButtonGroup* pGroup;
	QVBoxLayout* pVbox;
	QVBoxLayout* pButtonVbox;
	QRadioButton* pRadio;

	pPage = new QWidget(_typeTab, tabName.c_str());
	pVbox = new QVBoxLayout(pPage);

	pGroup = new QButtonGroup(pPage);
	pGroup->setColumnLayout(0, Qt::Vertical );
	pGroup->layout()->setSpacing( 6 );
	pGroup->layout()->setMargin( 11 );

	pButtonVbox = new QVBoxLayout(pGroup->layout());
	pButtonVbox->setAlignment( Qt::AlignTop );

	std::set<PRODUCT_TYPES>::iterator i;

	for (i = types.begin(); i != types.end(); i++) 
	{
		int id = _prodPlotInfo[*i].getId();
		pRadio = new QRadioButton(pGroup);
		pGroup->insert(pRadio, id);
		const QString label = _prodPlotInfo[*i].getLongName().c_str();
		pRadio->setText(label);
		pButtonVbox->addWidget(pRadio);

		// set the first radio button of the group
		// to be selected.
		if (i == types.begin()) {
			pRadio->setChecked(true);
		}
	}

	pVbox->addWidget(pGroup);
	_typeTab->insertTab(pPage, tabName.c_str());

	// connect the button released signal to our
	// our plot type change slot.
	connect(pGroup, SIGNAL(released(int)), this, SLOT(plotTypeSlot(int)));

	return pGroup;
}
//////////////////////////////////////////////////////////////////////
void
CP2Scope::timerEvent(QTimerEvent*) 
{

	int rate[3];
	for (int i = 0; i < 3; i++) 
	{
		rate[i] = (_pulseCount[i] - _prevPulseCount[i])/(double)_statsUpdateInterval;
		_prevPulseCount[i] = _pulseCount[i];
	}
	_chan0pulseCount->setNum(_pulseCount[0]/1000);
	_chan0pulseRate->setNum(rate[0]);
	_chan0errors->setNum(_errorCount[0]);
	_chan1pulseCount->setNum(_pulseCount[1]/1000);
	_chan1pulseRate->setNum(rate[1]);
	_chan1errors->setNum(_errorCount[1]);
	_chan2pulseCount->setNum(_pulseCount[2]/1000);
	_chan2pulseRate->setNum(rate[2]);
	_chan2errors->setNum(_errorCount[2]);
}

//////////////////////////////////////////////////////////////////////
void CP2Scope::gainChangeSlot(double gain)	{	

	// keep a local copy of the gain knob value
	_knobGain = gain;

	_powerCorrection = gain; 

	_graphRange = pow(10.0,-gain);

	_gainKnob->setValue(gain);

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
void 
CP2Scope::upSlot()	{
	_graphCenter -= 0.03*_graphRange;
}
//////////////////////////////////////////////////////////////////////
void 
CP2Scope::dnSlot()	{
	_graphCenter += 0.03*_graphRange;
}
//////////////////////////////////////////////////////////////////////
void 
CP2Scope::autoScale(std::vector<double>& data)
{
	double min = 1.0e10;
	double max = -1.0e10;
	for (int i = 0; i < data.size(); i++) {
		if (data[i] > max)
			max = data[i]; 
		else
			if (data[i] < min)
				min = data[i];
	}
	adjustGainOffset(min, max);
	_performAutoScale = false;
}
//////////////////////////////////////////////////////////////////////
void 
CP2Scope::autoScale(std::vector<double>& data1, std::vector<double>& data2)
{
	double min = 1.0e10;
	double max = -1.0e10;
	for (int i = 0; i < data1.size(); i++) {
		if (data1[i] > max)
			max = data1[i]; 
		else
			if (data1[i] < min)
				min = data1[i];
	}

	for  (int i = 0; i < data2.size(); i++) {
		if (data2[i] > max)
			max = data2[i]; 
		else
			if (data2[i] < min)
				min = data2[i];
	}
	adjustGainOffset(min, max);
	_performAutoScale = false;
}
//////////////////////////////////////////////////////////////////////
void 
CP2Scope::adjustGainOffset(double min, double max)
{
	double factor = 0.8;
	_graphCenter = (min+max)/2.0;
	_graphRange = (1/factor)*(max - min)/2.0;

	_knobGain = -log10(_graphRange);

	_gainKnob->setValue(_knobGain);
}
//////////////////////////////////////////////////////////////////////
void 
CP2Scope::autoScaleSlot()
{
	_performAutoScale = true;
}

