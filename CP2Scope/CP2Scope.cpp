
#include "CP2Scope.h"
#include <ScopePlot/ScopePlot.h>
#include <TwoKnobs/TwoKnobs.h>

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

//////////////////////////////////////////////////////////////////////
CP2Scope::CP2Scope():
_pSocket(0),    
_pSocketNotifier(0),
_pSocketBuf(0),	
_tsDisplayCount(0),
_productDisplayCount(0),
_Sparams(Params::DUAL_FAST_ALT),
_Xparams(Params::DUAL_CP2_XBAND),
_collator(1000),
_statsUpdateInterval(5)
{
	_dataGramPort	= 3100;
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
	initializeSocket();	

	// initialize the book keeping for the plots.
	// This also sets up the radio buttons 
	// in the plot type tab widget
	initPlots();

	_gainOffsetKnobs->setRanges(-5, 5, -10, 10);
	_gainOffsetKnobs->setTitles("Gain", "Offset");

	// set the minor ticks
	_gainOffsetKnobs->setScaleMaxMinor(1, 5);
	_gainOffsetKnobs->setScaleMaxMinor(2, 5);

	_scopeGain = 1;
	_scopeOffset = 0.0;

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

	// create the moment compute engine for S band
	_momentsSCompute = new MomentsCompute(_Sparams);

	// create the moment compute engine for X band
	_momentsXCompute = new MomentsCompute(_Xparams);

	// start the statistics timer
	startTimer(_statsUpdateInterval*1000);

}
//////////////////////////////////////////////////////////////////////
CP2Scope::~CP2Scope() {
	if (_pSocketNotifier)
		delete _pSocketNotifier;

	if (_pSocket)
		delete _pSocket;

	if (_pSocketBuf)
		delete [] _pSocketBuf;

	delete _momentsSCompute;

	delete _momentsXCompute;

}
//////////////////////////////////////////////////////////////////////
void 
CP2Scope::plotTypeSlot(bool b)
{
	b = b;  // so we don't get the unreferenced warning

	resizeDataVectors();	//	resize data vectors
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
void CP2Scope::gainChangeSlot(double gain)	{	
	_powerCorrection = gain; 
	_scopeGain = pow(10.0,-gain);
	_gain = gain;
}

//////////////////////////////////////////////////////////////////////
void CP2Scope::offsetChangeSlot(double offset)	{
	_scopeOffset = _scopeGain*offset;
	_offset = offset;
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
CP2Scope::dataSocketActivatedSlot(int)
{
	CP2Packet packet;
	int	readBufLen = _pSocket->readBlock((char *)_pSocketBuf, sizeof(short)*1000000);

	if (readBufLen > 0) {
		// put this datagram into a packet
		bool packetBad = packet.setData(readBufLen, _pSocketBuf);

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

	// beam will point to computed moments when they are ready,
	// or null if not ready
	Beam* pBeam = 0;

	PlotInfo* pi = &_plotInfo[_plotType];
	switch (pi->getDisplayType()) 
	{

		// for a product display, send the pulse to the moments compute engine.
		// and then send the moments to the display when a Beam is ready.
	case ScopePlot::PRODUCT:
		{
			// S band pulses: are successive coplaner H and V pulses
			// this horizontal switch is a hack for now; we really
			// need to find the h/v flag in the pulse header.
			if (chan == 0) {
				if (_sMomentsPlots.find(_plotType) != _sMomentsPlots.end())
				{
					_momentsSCompute->processPulse(data,
						0,
						pPulse->header.gates,
						1.0e-6, 
						pPulse->header.antEl, 
						pPulse->header.antAz, 
						pPulse->header.pulse_num,
						pPulse->header.horiz);	
					// ask for a completed beam. The return value will be
					// 0 if nothing is ready yet.
					pBeam = _momentsSCompute->getNewBeam();
				} else {
					pBeam = 0;
				}
			} else {
				if (_sMomentsPlots.find(_plotType)==_sMomentsPlots.end())
				{

					// X band will have H coplanar pulse on channel 1
					// and V cross planar pulses on channel 2. We need
					// to buffer up the data from the pulses and send it to 
					// the collator to match beam numbers, since matching
					// pulses (i.e. identical pulse numbers) are required
					// for the moments calculations.

					// create a CP2FullPuse, which is a class that will
					// hold the IQ data.
					CP2FullPulse* pFullPulse = new CP2FullPulse(pPulse);

					// send the pulse to the collator. The collator finds matching 
					// pulses. If orphan pulses are detected, they are deleted
					// by the collator. Otherwise, matching pulses returned from
					// the collator can be deleted here.
					_collator.addPulse(pFullPulse, pFullPulse->header()->channel - 1);

					// now see if we have some matching pulses
					CP2FullPulse* pHPulse;
					CP2FullPulse* pVPulse;
					if (_collator.gotMatch(&pHPulse, &pVPulse)) {
						// a matching pair was found. Send them to the X band
						// moments compute engine.
						_momentsXCompute->processPulse(
							pHPulse->data(), 
							pVPulse->data(),
							pPulse->header.gates,
							1.0e-6,
							pPulse->header.antEl, 
							pPulse->header.antAz, 
							pPulse->header.pulse_num,
							true);	
						// finished with these pulses, so delete them.
						delete pHPulse;
						delete pVPulse;
						// ask for a completed beam. The return value will be
						// 0 if nothing is ready yet.
						pBeam = _momentsXCompute->getNewBeam();
					}else {
						pBeam = 0;
					}
				}
			}

			if (pBeam) {
				// copy the selected product to the productDisplay
				// vector
				// return the beam
				getProduct(pBeam, pPulse->header.gates);
				// return the beam
				delete pBeam;
				// update the display
				displayData();
			}
			break;
		}
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
			if	(_tsDisplayCount >= _pulseDecimation)	{	//	
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
	}
}

//////////////////////////////////////////////////////////////////////
void 
CP2Scope::getProduct(Beam* pBeam, int gates) 
{
	const Fields* fields = pBeam->getFields();
	int i;

	if (_ProductData.size() != gates) {
		_ProductData.resize(gates);
	}

	switch(_plotType)	
	{
	case S_DBMHC:	///< S-band dBm horizontal co-planar
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].dbmhc;  } break;
	case S_DBMVC:	///< S-band dBm vertical co-planar
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].dbmvc;  } break;
	case S_DBZHC:	///< S-band dBz horizontal co-planar
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].dbzhc;  } break;
	case S_DBZVC:	///< S-band dBz vertical co-planar
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].dbzvc;  } break;
	case S_RHOHV:	///< S-band rhohv
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].rhohv;  } break;
	case S_PHIDP:	///< S-band phidp
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].phidp;  } break;
	case S_ZDR:	///< S-band zdr
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].zdr;  } break;
	case S_WIDTH:	///< S-band spectral width
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].width;  } break;
	case S_VEL:		///< S-band velocity
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].vel;    } break;
	case S_SNR:		///< S-band SNR
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].snr;    } break;
	case X_DBMHC:	///< X-band dBm horizontal co-planar
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].dbmhc;  } break;
	case X_DBMVX:	///< X-band dBm vertical cross-planar
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].dbmvx;  } break;
	case X_DBZHC:	///< X-band dBz horizontal co-planar
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].dbzhc;  } break;
	case X_WIDTH:	///< X-band spectral width
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].width;  } break;
	case X_VEL:		///< X-band velocity
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].vel;    } break;
	case X_SNR:		///< X-band SNR
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].snr;    } break;
	case X_LDR:		///< X-band LDR
		for (i = 0; i < gates; i++) { _ProductData[i] = fields[i].ldrh;   } break;
	}
}

//////////////////////////////////////////////////////////////////////
void
CP2Scope::displayData() 
{
	// display data -- called on decimation interval if pulse set, or fft size assembled if gate set. 

	PlotInfo* pi = &_plotInfo[_plotType];

	switch (pi->getDisplayType())
	{
	case ScopePlot::TIMESERIES:
		_scopePlot->TimeSeries(I, Q, -_scopeGain+_scopeOffset, _scopeGain+_scopeOffset, 1);
		break;
	case ScopePlot::IVSQ:
		_scopePlot->IvsQ(I, Q, -_scopeGain+_scopeOffset, _scopeGain+_scopeOffset, 1); 
		break;
	case ScopePlot::SPECTRUM:
		_scopePlot->Spectrum(_spectrum, -100, 20.0, 1000000, false, "Frequency (Hz)", "Power (dB)");	
		break;
	case ScopePlot::PRODUCT:
		_scopePlot->Product(_ProductData, pi->getId(), -_scopeGain+_scopeOffset, _scopeGain+_scopeOffset, _ProductData.size());
		break;
	}
}

//////////////////////////////////////////////////////////////////////
void
CP2Scope::initializeSocket()	
{
	_pSocket = new QSocketDevice(QSocketDevice::Datagram);

	QHostAddress qHost;

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
	_pSocketBuf = new char[1000000];

	std::string myIPname = nameBuf;
	std::string myIPaddress = inet_ntoa(*(struct in_addr*)pHostEnt->h_addr_list[0]);
	std::cout << "ip name: " << myIPname.c_str() << ", id address  " << myIPaddress.c_str() << std::endl;

	m_pTextIPname->setText(myIPname.c_str());

	m_pTextIPaddress->setNum(+_dataGramPort);	// diagnostic print
	qHost.setAddress(myIPaddress.c_str());

	std::cout << "qHost:" << qHost.toString() << std::endl;
	std::cout << "datagram port:" << _dataGramPort << std::endl;

	if (!_pSocket->bind(qHost, _dataGramPort)) {
		qWarning("Unable to bind to %s:%d", qHost.toString().ascii(), _dataGramPort);
		exit(1); 
	}
	int sockbufsize = CP2SCOPE_RCVBUF;

	int result = setsockopt (_pSocket->socket(),
		SOL_SOCKET,
		SO_RCVBUF,
		(char *) &sockbufsize,
		sizeof sockbufsize);
	if (result) {
		qWarning("Set receive buffer size for socket failed");
		exit(1); 
	}

	_pSocketNotifier = new QSocketNotifier(_pSocket->socket(), QSocketNotifier::Read);

	connect(_pSocketNotifier, SIGNAL(activated(int)), this, SLOT(dataSocketActivatedSlot(int)));
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
CP2Scope::plotTypeSlot(int newPlotType)
{
	if (newPlotType == _plotType) {
		return;
	}

	PlotInfo* pi;

	// save the gain and offset of the existing plot type
	pi = &_plotInfo[_plotType];
	pi->setGain(pi->getGainMin(), pi->getGainMax(), _gain);
	pi->setOffset(pi->getOffsetMin(), pi->getOffsetMax(), _offset);

	// change the plot type
	_plotType = (PLOTTYPE)newPlotType;

	// restore gain and offset 
	pi = &_plotInfo[_plotType];
	gainChangeSlot(pi->getGainCurrent());
	offsetChangeSlot(pi->getOffsetCurrent());

	// set the knobs for the new plot type
	_gainOffsetKnobs->setValues(pi->getGainCurrent(), pi->getOffsetCurrent());

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

	_sMomentsPlots.insert(S_DBMHC);
	_sMomentsPlots.insert(S_DBMVC);
	_sMomentsPlots.insert(S_DBZHC);
	_sMomentsPlots.insert(S_DBZVC);
	_sMomentsPlots.insert(S_WIDTH);
	_sMomentsPlots.insert(S_VEL);
	_sMomentsPlots.insert(S_SNR);
	_sMomentsPlots.insert(S_RHOHV);
	_sMomentsPlots.insert(S_PHIDP);
	_sMomentsPlots.insert(S_ZDR);

	_xMomentsPlots.insert(X_DBMHC);
	_xMomentsPlots.insert(X_DBMVX);
	_xMomentsPlots.insert(X_DBZHC);
	_xMomentsPlots.insert(X_SNR);
	//	_xMomentsPlots.insert(X_WIDTH);
	//	_xMomentsPlots.insert(X_VEL);
	_xMomentsPlots.insert(X_LDR);

	_plotInfo[S_TIMESERIES]  = PlotInfo( S_TIMESERIES, TIMESERIES, "I and Q", "S:  I and Q", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XH_TIMESERIES] = PlotInfo(XH_TIMESERIES, TIMESERIES, "I and Q", "Xh: I and Q", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XV_TIMESERIES] = PlotInfo(XV_TIMESERIES, TIMESERIES, "I and Q", "Xv: I and Q", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_IQ]          = PlotInfo(         S_IQ,       IVSQ, "I vs Q", "S:  I vs Q", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XH_IQ]         = PlotInfo(        XH_IQ,       IVSQ, "I vs Q", "Xh: I vs Q", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XV_IQ]         = PlotInfo(        XV_IQ,       IVSQ, "I vs Q", "Xv: I vs Q", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_SPECTRUM]    = PlotInfo(   S_SPECTRUM,   SPECTRUM, "Power Spectrum", "S:  Power Spectrum", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XH_SPECTRUM]   = PlotInfo(  XH_SPECTRUM,   SPECTRUM, "Power Spectrum", "Xh: Power Spectrum", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XV_SPECTRUM]   = PlotInfo(  XV_SPECTRUM,   SPECTRUM, "Power Spectrum", "Xv: Power Spectrum", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_DBMHC]       = PlotInfo(      S_DBMHC,    PRODUCT, "H Dbm", "Sh: Dbm", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_DBMVC]       = PlotInfo(      S_DBMVC,    PRODUCT, "V Dbm", "Sv: Dbm", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_DBZHC]       = PlotInfo(      S_DBZHC,    PRODUCT, "H Dbz", "Sh: Dbz", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_DBZVC]       = PlotInfo(      S_DBZVC,    PRODUCT, "V Dbz", "Sv: Dbz", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_WIDTH]       = PlotInfo(      S_WIDTH,    PRODUCT, "Width", "S:  Width", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_VEL]         = PlotInfo(        S_VEL,    PRODUCT, "Velocity", "S:  Velocity", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_SNR]         = PlotInfo(        S_SNR,    PRODUCT, "SNR", "S:  SNR", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_RHOHV]       = PlotInfo(      S_RHOHV,    PRODUCT, "Rhohv", "S:  Rhohv", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_PHIDP]       = PlotInfo(      S_RHOHV,    PRODUCT, "Phidp", "S:  Phidp", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_ZDR]         = PlotInfo(      S_ZDR,      PRODUCT, "Zdr", "S:  Zdr", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[X_DBMHC]       = PlotInfo(      X_DBMHC,    PRODUCT, "H Dbm", "Xh: Dbm", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[X_DBMVX]       = PlotInfo(      X_DBMVX,    PRODUCT, "V Cross Dbm", "Xv: Dbm", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[X_DBZHC]       = PlotInfo(      X_DBZHC,    PRODUCT, "H Dbz", "Xh: Dbz", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[X_SNR]         = PlotInfo(        X_SNR,    PRODUCT, "SNR", "Xh: SNR", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[X_WIDTH]       = PlotInfo(      X_WIDTH,    PRODUCT, "Width", "Xh: Width", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[X_VEL]         = PlotInfo(        X_VEL,    PRODUCT, "Velocity", "Xh: Velocity", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[X_LDR]         = PlotInfo(        X_LDR,    PRODUCT, "LDR", "Xhv:LDR", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);

	// remove the one tab that was put there by designer
	_typeTab->removePage(_typeTab->page(0));

	// add tabs, and save the button group for
	// for each tab.
	QButtonGroup* pGroup;

	pGroup = addPlotTypeTab("Raw", _rawPlots);
	_tabButtonGroups.push_back(pGroup);

	pGroup = addPlotTypeTab("S", _sMomentsPlots);
	_tabButtonGroups.push_back(pGroup);

	pGroup = addPlotTypeTab("X", _xMomentsPlots);
	_tabButtonGroups.push_back(pGroup);

	connect(_typeTab, SIGNAL(currentChanged(QWidget *)), 
		this, SLOT(tabChangeSlot(QWidget*)));
}
//////////////////////////////////////////////////////////////////////
void
CP2Scope::tabChangeSlot(QWidget* w) 
{
	// find out the index of the current page
	int pageNum = _typeTab->currentPageIndex();

	// get the radio button id of the currently selected button
	// on that page.
	int plotType = _tabButtonGroups[pageNum]->selectedId();

	// change the plot type
	plotTypeSlot(plotType);

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
