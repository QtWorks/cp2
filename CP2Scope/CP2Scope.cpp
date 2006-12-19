
#include "CP2Scope.h"
#include <ScopePlot/ScopePlot.h>
#include <TwoKnobs/TwoKnobs.h>

#include <qbuttongroup.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qspinbox.h>	
#include <qlcdnumber.h>
#include <qslider.h>


#include <winsock.h>
#include <iostream>
#include <time.h>

//////////////////////////////////////////////////////////////////////
CP2Scope::CP2Scope():
m_pDataSocket(0),    
m_pDataSocketNotifier(0),
m_pSocketBuf(0),	
_plotType(ScopePlot::TIMESERIES),
_tsDisplayCount(0),
_productDisplayCount(0),
_Sparams(Params::DUAL_FAST_ALT),
_Xparams(Params::DUAL_CP2_XBAND),
_collator(30)
{
	m_dataGramPort	= 3100;
	m_pulseCount	= 0; 

	// intialize the data reception socket
	initializeSocket();	
	// 
	connectDataRcv(); 

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
	_productType = 0; 

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
	switch (buttonGroup->selectedId()) {
		case 0:
			_plotType = ScopePlot::TIMESERIES;
			break;
		case 1:
			_plotType = ScopePlot::IVSQ;
			break;
		case 2:
			_plotType = ScopePlot::SPECTRUM;
			break;
			//	distiguish the various producs, within one _plotType
		case 3:
			_plotType = ScopePlot::PRODUCT;
			_productType = SVHVP; 
			break;
		case 4:
			_plotType = ScopePlot::PRODUCT;
			_productType = SVHHP; 
			break;
		case 5:
			_plotType = ScopePlot::PRODUCT;
			_productType = SVEL; 
			break;
		case 6:
			_plotType = ScopePlot::PRODUCT;
			_productType = SNCP; 
			break;
		case 7:
			_plotType = ScopePlot::PRODUCT;
			_productType = SWIDTH; 
			break;
		case 8:
			_plotType = ScopePlot::PRODUCT;
			_productType = SPHIDP; 
			break;
		case 9:
			_plotType = ScopePlot::PRODUCT;
			_productType = VREFL; 
			break;
		case 10:
			_plotType = ScopePlot::PRODUCT;
			_productType = HREFL; 
			break;
		case 11:
			_plotType = ScopePlot::PRODUCT;
			_productType = ZDR; 
			break;
		default: {}
	}

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

	switch (_plotType) {

	case ScopePlot::PRODUCT:
		{
			if (pPulse->header.channel == 0) {
				if(_productType ==  SPHIDP) {
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
				}
			} else {
				if(_productType !=  SPHIDP) {
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
			}

			if (pBeam) {
				// copy the selected product to the productDisplay
				// vector
				// return the beam
				getProduct(pBeam, pPulse->header.gates, _productType);
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
	default:
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
CP2Scope::getProduct(Beam* pBeam, int gates, int productType) 
{
	const Fields* fields = pBeam->getFields();
	int i;
	switch(productType)	
	{
	case SVHVP:		//	compute S-band VH V Power in dBm for display
		for (i = 0; i < gates; i++) {
			ProductData[i] = fields[i].dbmvc;
		}
		break;
	case SVHHP:		//	compute S-band VH H Power in dBm for display
		for (i = 0; i < gates; i++) {
			ProductData[i] = fields[i].dbmhc;
		}
		break;
	case SVEL:		//	compute S-band velocity using V and H data
		for (i = 0; i < gates; i++) {
			ProductData[i] = fields[i].vel;
		}
		break;
	case SNCP:		//	compute S-band NCP using calculation A2*A2+B2*B2/Pv+Ph
		break;
	case SWIDTH:	//  compute S-band width
		for (i = 0; i < gates; i++) {
			ProductData[i] = fields[i].width;
		}
		break;			
	case SPHIDP:	//	compute S-band phidp 
		for (i = 0; i < gates; i++) {
			ProductData[i] = fields[i].phidp;
		}
		break;			
	case VREFL:		//	compute S-band V reflectivity  
		for (i = 0; i < gates; i++) {
			ProductData[i] = fields[i].dbzvc;
		}
		break;			
	case HREFL:		//	compute S-band H reflectivity  
		for (i = 0; i < gates; i++) {
			ProductData[i] = fields[i].dbzhc;
		}
		break;			
	case ZDR:		//	compute S-band Zdr 
		for (i = 0; i < gates; i++) {
			ProductData[i] = fields[i].zdr;
		}
		break;		
	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////
void
CP2Scope::displayData() 
{
	// display data -- called on decimation interval if pulse set, or fft size assembled if gate set. 

	switch (_plotType) {
	case ScopePlot::TIMESERIES:
		_scopePlot->TimeSeries(I, Q, -_scopeGain+_scopeOffset, _scopeGain+_scopeOffset, 1);
		break;
	case ScopePlot::IVSQ:
		_scopePlot->IvsQ(I, Q, -_scopeGain+_scopeOffset, _scopeGain+_scopeOffset, 1); 
		break;
	case ScopePlot::SPECTRUM:
		//	use Spectrum() w/linear limits, linear data on y-axis:
		_scopePlot->Spectrum(_spectrum, -100, 20.0, 1000000, false, "Frequency (Hz)", "Power (dB)");	
		break;
	case ScopePlot::PRODUCT:
		//	proliferate product types here:
		switch(_productType)	{
	case SVHVP:		//	compute S-band VH V Power in dBm for display
		_scopePlot->Product(ProductData, _productType, -_scopeGain+_scopeOffset, _scopeGain+_scopeOffset, m_xFullScale, "Gate", "S V Power (dBm)"); 
		break;
	case SVHHP:		//	compute S-band VH H Power in dBm for display
		_scopePlot->Product(ProductData, _productType, -_scopeGain+_scopeOffset, _scopeGain+_scopeOffset, m_xFullScale, "Gate", "S H Power (dBm)"); 
		break;
	case SVEL:		//	compute S-band velocity using V and H data
		_scopePlot->Product(ProductData, _productType, -_scopeGain+_scopeOffset, _scopeGain+_scopeOffset, m_xFullScale, "Gate", "Velocity (m/s)"); 
		break;
	case SNCP:		//	compute S-band NCP using calculation A2*A2+B2*B2/Pv+Ph
		_scopePlot->Product(ProductData, _productType, -_scopeGain+_scopeOffset, _scopeGain+_scopeOffset, m_xFullScale, "Gate", "NCP"); 
		break;
	case SWIDTH:
		_scopePlot->Product(ProductData, _productType, -_scopeGain+_scopeOffset, _scopeGain+_scopeOffset, m_xFullScale, "Gate", "Spectral Width"); 
		break;			
	case SPHIDP:
		//	compute S-band phidp 
		_scopePlot->Product(ProductData, _productType, -180.0, 180.0, m_xFullScale, "Gate", "phidp degrees"); 
		break;			
	case VREFL:
		//	compute S-band V reflectivity  
		_scopePlot->Product(ProductData, _productType, -_scopeGain+_scopeOffset, _scopeGain+_scopeOffset, m_xFullScale, "Gate", "V Reflectivity"); 
		break;			
	case HREFL:
		//	compute S-band H reflectivity  
		_scopePlot->Product(ProductData, _productType, -_scopeGain+_scopeOffset, _scopeGain+_scopeOffset, m_xFullScale, "Gate", "H Reflectivity"); 
		break;			
	case ZDR:
		//	compute S-band Zdr 
		_scopePlot->Product(ProductData, _productType, -_scopeGain+_scopeOffset, _scopeGain+_scopeOffset, m_xFullScale, "Gate", "Zdr"); 
		break;			
		}
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
