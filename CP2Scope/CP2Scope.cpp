
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


#include <winsock.h>
#include <iostream>
#include <time.h>

//////////////////////////////////////////////////////////////////////
CP2Scope::CP2Scope():
m_pDataSocket(0),    
m_pDataSocketNotifier(0),
m_pSocketBuf(0),	
_tsDisplayCount(0),
_productDisplayCount(0),
_Sparams(Params::DUAL_FAST_ALT),
_Xparams(Params::DUAL_CP2_XBAND),
_collator(30)
{
	m_dataGramPort	= 3100;
	m_pulseCount	= 0; 
	_plotType = S_TIMESERIES;

	// intialize the data reception socket
	initializeSocket();	
	// 
	connectDataRcv(); 

	initPlots();

	_gainOffsetKnobs->setRanges(-5, 5, -10, 10);
	_gainOffsetKnobs->setTitles("Gain", "Offset");
	_scopeGain = 1;
	_scopeOffset = 0.0;

	m_xFullScale = 980;
	I.resize(m_xFullScale);			//	default timeseries array size full range at 1KHz
	Q.resize(m_xFullScale);
	_dataSetSize = m_xFullScale;	//	size of data vector for display or calculations 
	_dataSet = DATA_SET_PULSE;

	//	display decimation, set to get ~50/sec
	m_pulseDisplayDecimation	= 50;	//	default w/prt = 1000Hz, timeseries data 
	m_productsDisplayDecimation	= 5;	//	default w/prt = 1000Hz, hits = 10, products data
	m_dataChannel				= 1;			//	default to board-select spinner: get value from it

	m_DataSetGate = 50;		//!get spinner value

	//	set up fft for power calculations: 
	m_fft_blockSize = 256;	//	temp constant for initial proving 
	// allocate the data space for fftw
	_fftwData  = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * m_fft_blockSize);
	// create the plan.
	_fftwPlan = fftw_plan_dft_1d(m_fft_blockSize, _fftwData, _fftwData,
		FFTW_FORWARD, FFTW_ESTIMATE);
	//	power correction factor applied to (uncorrected) powerSpectrum() output:
	_powerCorrection = 0.0;	//	use for power correction to dBm

	_momentsSCompute = new MomentsCompute(_Sparams);
	_momentsXCompute = new MomentsCompute(_Xparams);


}
//////////////////////////////////////////////////////////////////////
CP2Scope::~CP2Scope() {
	if (m_pDataSocketNotifier)
		delete m_pDataSocketNotifier;

	if (m_pDataSocket)
		delete m_pDataSocket;

	if (m_pSocketBuf)
		delete [] m_pSocketBuf;

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
		ProductData.resize(m_xFullScale); 
		//	enable x-scale spinner
	}
	else	{	//	timeseries-type or product
		I.resize(m_xFullScale);	//	resize timeseries arrays, x-axis
		Q.resize(m_xFullScale);
		_dataSetSize = m_xFullScale; 
	}
}

//////////////////////////////////////////////////////////////////////
void CP2Scope::gainChangeSlot(double gain)	{	
	_powerCorrection = gain; 
	_scopeGain = pow(10.0,-gain);
}

//////////////////////////////////////////////////////////////////////
void CP2Scope::offsetChangeSlot(double offset)	{
	_scopeOffset = _scopeGain*offset;
}

//////////////////////////////////////////////////////////////////////
void 
CP2Scope::DataChannelSpinBox_valueChanged( int dataChannel )	
{
	//	change the data channel
	m_dataChannel = dataChannel;	
}


//////////////////////////////////////////////////////////////////////
void 
CP2Scope::DataSetGateSpinBox_valueChanged( int SpinBoxGate )	{
	//	change the gate for assembling data sets 
	m_DataSetGate    = SpinBoxGate;
}
//////////////////////////////////////////////////////////////////////
void 
CP2Scope::xFullScaleBox_valueChanged( int xFullScale )	{
	//	change the gate for assembling data sets 
	if	(_plotType == ScopePlot::SPECTRUM)	//	fft: disable spinner
		return;
	m_xFullScale    = xFullScale;
	resizeDataVectors();	//	resize data vectors
}


//////////////////////////////////////////////////////////////////////
void
CP2Scope::connectDataRcv()	// connect notifier to data-receive slot 
{
	connect(m_pDataSocketNotifier, SIGNAL(activated(int)), this, SLOT(dataSocketActivatedSlot(int)));
}


//////////////////////////////////////////////////////////////////////
void 
CP2Scope::dataSocketActivatedSlot(int socket)
{
	socket = socket;  // so we don't get the unreferenced warning
	CP2Packet packet;
	int	readBufLen = m_pDataSocket->readBlock((char *)m_pSocketBuf, sizeof(short)*1000000);

	if (readBufLen > 0) {
		// put this datagram into a packet
		bool packetBad = packet.setData(readBufLen, m_pSocketBuf);

		// Extract the pulses and process them.
		// Observe paranoia for validating packets and pulses.
		// From here on out, we are divorced from the
		// data transport.
		if (!packetBad) {
			for (int i = 0; i < packet.numPulses(); i++) {
				CP2Pulse* pPulse = packet.getPulse(i);
				if (pPulse) {
					processPulse(pPulse);
					m_pulseCount++;
					if (!(m_pulseCount % 1000))
						_pulseCount->setNum(m_pulseCount/1000);	// update cumulative packet count 
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
	float* data = &(pPulse->data[0]);

	// beam will point to computed moments when they are ready,
	// or null if not ready
	Beam* pBeam = 0;

	PlotInfo* pi = &_plotInfo[_plotType];
	switch (pi->getDisplayType()) 
	{

	case ScopePlot::PRODUCT:
		{
			if (pPulse->header.channel == 0) {
				bool horizontal = (pPulse->header.pulse_num %2);
				_momentsSCompute->processPulse(data,
					0,
					pPulse->header.gates,
					1.0e-6, 
					pPulse->header.el, 
					pPulse->header.az, 
					pPulse->header.pulse_num,
					horizontal);	
				pBeam = _momentsSCompute->getNewBeam();
			} else {
				// copy the pulse data, since we have to save it for collation
				CP2FullPulse* pFullPulse = new CP2FullPulse(pPulse);
				// send the pulse to the collator
				_collator.addPulse(pFullPulse, pFullPulse->header()->channel - 1);

				// now see if we have some matching beams
				CP2FullPulse* pHPulse;
				CP2FullPulse* pVPulse;
				if (_collator.gotMatch(&pHPulse, &pVPulse)) {
					_momentsXCompute->processPulse(pHPulse->data(), 
						pVPulse->data(),
						pPulse->header.gates,
						1.0e-6, 
						pPulse->header.el, 
						pPulse->header.az, 
						pPulse->header.pulse_num,
						true);	
					delete pHPulse;
					delete pVPulse;
					pBeam = _momentsXCompute->getNewBeam();
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
			if (pPulse->header.channel != m_dataChannel)
				break;
			_tsDisplayCount++;
			if	(_tsDisplayCount >= m_pulseDisplayDecimation)	{	
				_spectrum.resize(m_fft_blockSize);	//	probably belongs somewhere else
				for( int j = 0; j < m_fft_blockSize; j++)	{
					// transfer the data to the fftw input space
					_fftwData[j][0] = pPulse->data[2*j]*PIRAQ3D_SCALE;
					_fftwData[j][1] = pPulse->data[2*j+1]*PIRAQ3D_SCALE;
				}
				double zeroMoment = powerSpectrum();
				// correct unscaled power data using knob setting: 
				for(j = 0; j < m_fft_blockSize; j++)	{
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
			if (pPulse->header.channel != m_dataChannel)
				break;
			_tsDisplayCount++;
			if	(_tsDisplayCount >= m_pulseDisplayDecimation)	{	//	
				for (int i = 0; i < 2*m_xFullScale; i+=2) {	
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
	switch(_plotType)	
	{
	case S_DBMHC:	///< S-band dBm horizontal co-planar
		for (i = 0; i < gates; i++) { ProductData[i] = fields[i].dbmhc;  } break;
	case S_DBMVC:	///< S-band dBm vertical co-planar
		for (i = 0; i < gates; i++) { ProductData[i] = fields[i].dbmvc;  } break;
	case S_DBZHC:	///< S-band dBz horizontal co-planar
		for (i = 0; i < gates; i++) { ProductData[i] = fields[i].dbzhc;  } break;
	case S_DBZVC:	///< S-band dBz vertical co-planar
		for (i = 0; i < gates; i++) { ProductData[i] = fields[i].dbzvc;  } break;
	case S_RHOHV:	///< S-band rhohv
		for (i = 0; i < gates; i++) { ProductData[i] = fields[i].rhohv;  } break;
	case S_WIDTH:	///< S-band spectral width
		for (i = 0; i < gates; i++) { ProductData[i] = fields[i].width;  } break;
	case S_VEL:		///< S-band velocity
		for (i = 0; i < gates; i++) { ProductData[i] = fields[i].vel;    } break;
	case S_SNR:		///< S-band SNR
		for (i = 0; i < gates; i++) { ProductData[i] = fields[i].snr;    } break;
	case X_DBMHC:	///< X-band dBm horizontal co-planar
		for (i = 0; i < gates; i++) { ProductData[i] = fields[i].dbmhc;  } break;
	case X_DBMVX:	///< X-band dBm vertical cross-planar
		for (i = 0; i < gates; i++) { ProductData[i] = fields[i].dbmvx;  } break;
	case X_DBZHC:	///< X-band dBz horizontal co-planar
		for (i = 0; i < gates; i++) { ProductData[i] = fields[i].dbzhc;  } break;
	case X_WIDTH:	///< X-band spectral width
		for (i = 0; i < gates; i++) { ProductData[i] = fields[i].width;  } break;
	case X_VEL:		///< X-band velocity
		for (i = 0; i < gates; i++) { ProductData[i] = fields[i].vel;    } break;
	case X_SNR:		///< X-band SNR
		for (i = 0; i < gates; i++) { ProductData[i] = fields[i].snr;    } break;
	case X_LDR:		///< X-band LDR
		for (i = 0; i < gates; i++) { ProductData[i] = fields[i].ldrh;   } break;
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
		_scopePlot->Product(ProductData, pi->getId(), -_scopeGain+_scopeOffset, _scopeGain+_scopeOffset, 1);
		break;
	}
}

//////////////////////////////////////////////////////////////////////
void
CP2Scope::initializeSocket()	
{
	m_pDataSocket = new QSocketDevice(QSocketDevice::Datagram);

	QHostAddress qHost;

	char nameBuf[1000];
	if (gethostname(nameBuf, sizeof(nameBuf))) {
		qWarning("gethostname failed");
		exit(1);
	}

	struct hostent* pHostEnt = gethostbyname(nameBuf);
	if (!pHostEnt) {
		qWarning("gethostbyname failed");
		exit(1);
	}
	m_pSocketBuf = new char[1000000];

	std::string myIPname = nameBuf;
	std::string myIPaddress = inet_ntoa(*(struct in_addr*)pHostEnt->h_addr_list[0]);
	std::cout << "ip name: " << myIPname.c_str() << ", id address  " << myIPaddress.c_str() << std::endl;

	m_pTextIPname->setText(myIPname.c_str());

	m_pTextIPaddress->setNum(+m_dataGramPort);	// diagnostic print
	qHost.setAddress(myIPaddress.c_str());

	std::cout << "qHost:" << qHost.toString() << std::endl;
	std::cout << "datagram port:" << m_dataGramPort << std::endl;

	if (!m_pDataSocket->bind(qHost, m_dataGramPort)) {
		qWarning("Unable to bind to %s:%d", qHost.toString().ascii(), m_dataGramPort);
		exit(1); 
	}
	int sockbufsize = 1000000;

	int result = setsockopt (m_pDataSocket->socket(),
		SOL_SOCKET,
		SO_RCVBUF,
		(char *) &sockbufsize,
		sizeof sockbufsize);
	if (result) {
		qWarning("Set receive buffer size for socket failed");
		exit(1); 
	}

	m_pDataSocketNotifier = new QSocketNotifier(m_pDataSocket->socket(), QSocketNotifier::Read);
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
	int n = m_fft_blockSize;

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
CP2Scope::initPlots()
{

	// Here are all of the possible plot types:
	//	S_TIMESERIES,	// S time series
	//	XH_TIMESERIES,	// Xh time series
	//	XV_TIMESERIES,	// Xv time series
	//	S_IQ,			// S IQ
	//	XH_IQ,			// Xh IQ
	//	XV_IQ,			// Xv IQ
	//	S_SPECTRUM,		// S spectrum 
	//	XH_SPECTRUM,	// Xh spectrum 
	//	XV_SPECTRUM,	// Xv spectrum 
	//	S_DBMHC,	// S-band dBm horizontal co-planar
	//	S_DBMVC,	// S-band dBm vertical co-planar
	//	S_DBZHC,	// S-band dBz horizontal co-planar
	//	S_DBZVC,	// S-band dBz vertical co-planar
	//	S_RHOHV,	// S-band rhohv
	//	S_WIDTH,	// S-band spectral width
	//	S_VEL,		// S-band velocity
	//	S_SNR,		// S-band SNR
	//	X_DBMHC,	// X-band dBm horizontal co-planar
	//	X_DBMVX,	// X-band dBm vertical cross-planar
	//	X_DBZHC,	// X-band dBz horizontal co-planar
	//	X_SNR,		// X-band SNR
	//	X_WIDTH,	// X-band spectral width
	//	X_VEL,		// X-band velocity
	//	X_SNR,		// X-band SNR
	//	X_LDR		// X-band LDR

	_timeSeriesPlots.insert(S_TIMESERIES);
	_timeSeriesPlots.insert(XH_TIMESERIES);
	_timeSeriesPlots.insert(XV_TIMESERIES);

	_iqPlots.insert(S_IQ);
	_iqPlots.insert(XH_IQ);
	_iqPlots.insert(XV_IQ);

	_spectrumPlots.insert(S_SPECTRUM);
	_spectrumPlots.insert(XH_SPECTRUM);
	_spectrumPlots.insert(XV_SPECTRUM);

	_momentsPlots.insert(S_DBMHC);
	_momentsPlots.insert(S_DBMVC);
	_momentsPlots.insert(S_DBZHC);
	_momentsPlots.insert(S_DBZVC);
	_momentsPlots.insert(S_RHOHV);
	_momentsPlots.insert(S_WIDTH);
	_momentsPlots.insert(S_VEL);
	_momentsPlots.insert(S_SNR);
	_momentsPlots.insert(X_DBMHC);
	_momentsPlots.insert(X_DBMVX);
	_momentsPlots.insert(X_DBZHC);
	_momentsPlots.insert(X_SNR);
	_momentsPlots.insert(X_WIDTH);
	_momentsPlots.insert(X_VEL);
	_momentsPlots.insert(X_SNR);
	_momentsPlots.insert(X_LDR);

	_sMomentsPlots.insert(S_DBMHC);
	_sMomentsPlots.insert(S_DBMVC);
	_sMomentsPlots.insert(S_DBZHC);
	_sMomentsPlots.insert(S_DBZVC);
	_sMomentsPlots.insert(S_RHOHV);
	_sMomentsPlots.insert(S_WIDTH);
	_sMomentsPlots.insert(S_VEL);
	_sMomentsPlots.insert(S_SNR);

	_xMomentsPlots.insert(X_DBMHC);
	_xMomentsPlots.insert(X_DBMVX);
	_xMomentsPlots.insert(X_DBZHC);
	_xMomentsPlots.insert(X_SNR);
	_xMomentsPlots.insert(X_WIDTH);
	_xMomentsPlots.insert(X_VEL);
	_xMomentsPlots.insert(X_SNR);
	_xMomentsPlots.insert(X_LDR);

	_plotInfo[S_TIMESERIES]  = PlotInfo( S_TIMESERIES, TIMESERIES, "S band", "S Band I&Q", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XH_TIMESERIES] = PlotInfo(XH_TIMESERIES, TIMESERIES, "S band", "S Band I vs Q", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XV_TIMESERIES] = PlotInfo(XV_TIMESERIES, TIMESERIES, "S band", "S Band Power Spectrum", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_IQ]          = PlotInfo(         S_IQ,       IVSQ, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XH_IQ]         = PlotInfo(        XH_IQ,       IVSQ, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XV_IQ]         = PlotInfo(        XV_IQ,       IVSQ, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_SPECTRUM]    = PlotInfo(   S_SPECTRUM,   SPECTRUM, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XH_SPECTRUM]   = PlotInfo(  XH_SPECTRUM,   SPECTRUM, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[XV_SPECTRUM]   = PlotInfo(  XV_SPECTRUM,   SPECTRUM, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_DBMHC]       = PlotInfo(      S_DBMHC,    PRODUCT, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_DBMVC]       = PlotInfo(      S_DBMVC,    PRODUCT, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_DBZHC]       = PlotInfo(      S_DBZHC,    PRODUCT, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_DBZVC]       = PlotInfo(      S_DBZVC,    PRODUCT, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_RHOHV]       = PlotInfo(      S_RHOHV,    PRODUCT, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_WIDTH]       = PlotInfo(      S_WIDTH,    PRODUCT, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_VEL]         = PlotInfo(        S_VEL,    PRODUCT, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[S_SNR]         = PlotInfo(        S_SNR,    PRODUCT, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[X_DBMHC]       = PlotInfo(      X_DBMHC,    PRODUCT, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[X_DBMVX]       = PlotInfo(      X_DBMVX,    PRODUCT, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[X_DBZHC]       = PlotInfo(      X_DBZHC,    PRODUCT, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[X_SNR]         = PlotInfo(        X_SNR,    PRODUCT, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[X_WIDTH]       = PlotInfo(      X_WIDTH,    PRODUCT, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[X_VEL]         = PlotInfo(        X_VEL,    PRODUCT, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[X_SNR]         = PlotInfo(        X_SNR,    PRODUCT, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_plotInfo[X_LDR]         = PlotInfo(        X_LDR,    PRODUCT, "S band", "S band", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);

	QWidget* pPage;
	QButtonGroup* pGroup;
	QVBoxLayout* pVbox;
	QVBoxLayout* pButtonVbox;
	QRadioButton* pRadio;
	QFrame* pFrame;

/**
    Xproducts = new QWidget( _typeTab, "Xproducts" );
    XproductsLayout = new QVBoxLayout( Xproducts, 11, 6, "XproductsLayout"); 

	buttonGroup7 = new QButtonGroup( Xproducts, "buttonGroup7" );
    buttonGroup7->setColumnLayout(0, Qt::Vertical );
    buttonGroup7->layout()->setSpacing( 6 );
    buttonGroup7->layout()->setMargin( 11 );
    buttonGroup7Layout = new QVBoxLayout( buttonGroup7->layout() );
    buttonGroup7Layout->setAlignment( Qt::AlignTop );

    radioButton19 = new QRadioButton( buttonGroup7, "radioButton19" );
    buttonGroup7Layout->addWidget( radioButton19 );

    radioButton20 = new QRadioButton( buttonGroup7, "radioButton20" );
    buttonGroup7Layout->addWidget( radioButton20 );

    radioButton21 = new QRadioButton( buttonGroup7, "radioButton21" );
    buttonGroup7Layout->addWidget( radioButton21 );
    XproductsLayout->addWidget( buttonGroup7 );
    _typeTab->insertTab( Xproducts, QString::fromLatin1("") );
**/

	_typeTab->removePage(_typeTab->page(0));

	pPage = new QWidget(_typeTab, "Chan");
	pVbox = new QVBoxLayout(pPage);

	pGroup = new QButtonGroup(pPage);
    pGroup->setColumnLayout(0, Qt::Vertical );
    pGroup->layout()->setSpacing( 6 );
    pGroup->layout()->setMargin( 11 );

	pButtonVbox = new QVBoxLayout(pGroup->layout());
    pButtonVbox->setAlignment( Qt::AlignTop );

	std::set<PLOTTYPE>::iterator i;
	for (i = _timeSeriesPlots.begin(); i != _timeSeriesPlots.end(); i++) 
	{
		int id = _plotInfo[*i].getId();
		pRadio = new QRadioButton(pGroup);
		pGroup->insert(pRadio, id);
		const QString label = _plotInfo[*i].getLongName().c_str();
		pRadio->setText(label);
		pButtonVbox->addWidget(pRadio);
	}

	for (i = _iqPlots.begin(); i != _iqPlots.end(); i++) 
	{
		int id = _plotInfo[*i].getId();
		pRadio = new QRadioButton(pGroup);
		pGroup->insert(pRadio, id);
		const QString label = _plotInfo[*i].getLongName().c_str();
		pRadio->setText(label);
		pButtonVbox->addWidget(pRadio);
	}

	for (i = _spectrumPlots.begin(); i != _spectrumPlots.end(); i++) 
	{
		int id = _plotInfo[*i].getId();
		pRadio = new QRadioButton(pGroup);
		pGroup->insert(pRadio, id);
		const QString label = _plotInfo[*i].getLongName().c_str();
		pRadio->setText(label);
		pButtonVbox->addWidget(pRadio);
	}


	pVbox->addWidget(pGroup);
	_typeTab->insertTab(pPage, "my tab");


}
