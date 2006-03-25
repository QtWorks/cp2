
#include "CP2Scope.h"
#include <ScopePlot/ScopePlot.h>
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qspinbox.h>	

#include <Knob/Knob.h>

#include <winsock.h>
#include <iostream>

CP2Scope::CP2Scope():
m_pDataSocket(0),    
m_pDataSocketNotifier(0),
m_pSocketBuf(0),	
_plotType(ScopePlot::TIMESERIES)
{
	this->m_pSpinBoxPort->setMinValue(WINDSP_BASE_PORT);
	this->m_pSpinBoxPort->setMaxValue(WINDSP_BASE_PORT + 3);
	m_dataGramPort	= this->m_pSpinBoxPort->value();
	m_packetCount	= 0; 
	// create the socket that receives packets from WinDSP  
	initializeSocket();	
	connectDataRcv(); 

	// Start a timer to tell the display to update
	m_dataDisplayTimer = startTimer(DISPLAY_REFRESH_INTERVAL);
	//connect( ... do with QTimer later see Notes

	//	set knob assuming TIMESERIES: 
	this->yScaleKnob->setTitle("Y-SCALE ADJ."); 
	m_display_yScale = ySCALEMAX / 10.0;	// start off reasonable
	m_yScaleMin = ySCALEMIN;	// set initial y-scale min, max
	m_yScaleMax	= ySCALEMAX;	

	this->yScaleKnob->setRange(m_yScaleMin, m_yScaleMax); 
	//	set up fft for power calculations: 
	m_fft_blockSize = 256;	//	temp constant for initial proving 
	// allocate the data space for fftw
    _fftwData  = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * m_fft_blockSize);
    // create the plan.
    _fftwPlan = fftw_plan_dft_1d(m_fft_blockSize, _fftwData, _fftwData,
				 FFTW_FORWARD, FFTW_ESTIMATE);
	//	power correction factor applied to (uncorrected) powerSpectrum() output:
	_powerCorrection = 0.0;	//	use for power correction to dBm

	I.resize(500);			//	moved to constructor
	Q.resize(500);

}

CP2Scope::~CP2Scope() {
	if (m_pDataSocketNotifier)
		delete m_pDataSocketNotifier;

	if (m_pDataSocket)
		delete m_pDataSocket;

	if (m_pSocketBuf)
		delete [] m_pSocketBuf;
	//?delete timer? 
}

void 
CP2Scope::timerEvent(QTimerEvent *e) {
	displayData();	//	this works here
}

void 
CP2Scope::plotTypeSlot(bool b)
{
  switch (buttonGroup->selectedId()) {
      case 0:
		  _plotType = ScopePlot::TIMESERIES;
			//	set knob for TIMESERIES: 
			this->yScaleKnob->setTitle("Ampl. Scale"); 
//	m_display_yScale = ySCALEMAX / 10.0;	// start off reasonable
//	m_yScaleMin = ySCALEMIN;	// set initial y-scale min, max
//	m_yScaleMax	= ySCALEMAX;	
	m_display_yScale = m_yScaleMax / 10.0;	// start off reasonable

	this->yScaleKnob->setRange(m_yScaleMin, m_yScaleMax); 
		  break;
      case 1:
		  _plotType = ScopePlot::IVSQ;
			this->yScaleKnob->setTitle("Ampl. Scale"); 
         break;
      case 2:
		  _plotType = ScopePlot::SPECTRUM;
			//	set knob for Power correction: 
			this->yScaleKnob->setTitle("Power Corr"); 
//			m_display_yScale = 0.0;	// start off reasonable
//			m_yScaleMin = -30.0;	// set initial y-scale min, max
//			m_yScaleMax	= 30.0;	
//			this->yScaleKnob->setRange(m_yScaleMin, m_yScaleMax); 
			this->yScaleKnob->setRange(-30.0, 30.0); 

         break;
      default: {}
   }

}

void CP2Scope::yScaleKnob_valueChanged( double yScaleKnobSetting)	{	
//		std::cout << "y-Scale Knob setting " << yScaleKnobSetting << std::endl;
	switch (_plotType) {
		case 0:	//	timeseries displays
		case 1:
		if (yScaleKnobSetting == 0.0)	{	//	y-scale set to minimum
			m_yScaleMax *= 0.1;				//	re-scale /10
			m_display_yScale = m_yScaleMax / 10.0; 
			yScaleKnob->setRange(m_yScaleMin, m_yScaleMax);	//
		}
		else if (yScaleKnobSetting == m_yScaleMax)	{	//	max'd y-scale
			m_yScaleMax *= 10.0;	//	re-scale *10
			yScaleKnob->setRange(m_yScaleMin, m_yScaleMax);
		}
		else
			m_display_yScale = yScaleKnobSetting; 
		break;
		case 2:	//	power display
			_powerCorrection = yScaleKnobSetting; 
			break;
	}
}

void CP2Scope::DatagramPortSpinBox_valueChanged( int datagramPort )	{
	//	change the input data port
	m_dataGramPort    = datagramPort;
	terminateSocket();
	m_packetCount = 0;		//	clear packet count 
	_packetCount->setNum(m_packetCount);	// update display  
	initializeSocket();
	connectDataRcv(); 
}


void
CP2Scope::connectDataRcv()	// connect notifier to data-receive slot 
{
	connect(m_pDataSocketNotifier, SIGNAL(activated(int)), this, SLOT(dataSocketActivatedSlot(int)));
}

void 
CP2Scope::dataSocketActivatedSlot(int socket)
{
	static int count = 0;
	static int errcount = 0;

	int readBufLen, lastreadBufLen;

	m_packetCount++; 
	readBufLen = m_pDataSocket->readBlock((char *)m_pSocketBuf, sizeof(short)*1000000);
	lastreadBufLen = readBufLen;

	if (!(count++ % 50))	{
		if(readBufLen > 0) {
//			std::cout << "read returned " << readBufLen << std::endl;
			_packetCount->setNum(m_packetCount);	// update cumulative packet count 
		}
	}
}


void
CP2Scope::displayData() 
{
	// display data -- activated by QTimer timeout()
//	std::vector<double> I;	//	moved to constructor
//	std::vector<double> Q;
//	I.resize(500);			//	moved to constructor
//	Q.resize(500);
//	I.resize(200);	//	easy zoom from start of data
//	Q.resize(200);

	float* p = ((float*)(m_pSocketBuf)) + 764/sizeof(float);	//	hardwire offset to first datum: 704+32+28
//double p0, p1, p2, p3, p04;// p5; 
//p126, p127, p128, p129, p130, p131, p132, p254, p255; 
//p0 = p[0]; p1 = p[1]; p2 = p[2]; p3 = p[3]; p04 = p[4];// p5 = p[5]; 
//p126 = p[126]; p127 = p[127]; 
//p128 = p[128]; p129 = p[129]; 
//p130 = p[130]; p131 = p[131]; p132 = p[132]; 
//p254 = p[254]; p255 = p[255];  
	for (int i = 0; i <1000; i+=2) {	
//	for (int i = 0; i <400; i+=2) {	//	easy zoom from start of data	
		//	PIRAQ data:
		I[i/2] = p[i];
		Q[i/2] = p[i+1];
	} 

	switch (_plotType) {
		case ScopePlot::TIMESERIES:
			_scopePlot->TimeSeries(I, Q, -m_display_yScale, m_display_yScale, 1);
			break;
		case ScopePlot::IVSQ:
			_scopePlot->IvsQ(I, Q, -m_display_yScale, m_display_yScale, 1); 
			break;
		case ScopePlot::SPECTRUM:
			_spectrum.resize(m_fft_blockSize);	//	probably belongs somewhere else
			for(unsigned int j = 0; j < m_fft_blockSize; j++)	{
			// transfer the data to the fftw input space
				_fftwData[j][0] = I[j];
				_fftwData[j][1] = Q[j];
			}
		    double zeroMoment = powerSpectrum();
			// correct unscaled power data using knob setting: 
			for(unsigned int j = 0; j < m_fft_blockSize; j++)	{
				_spectrum[j] += _powerCorrection;
			}
//			_scopePlot->Spectrum(_spectrum, 1.0e-70, 1.0e+10, 1, "Frequency (Hz)", "Power (dB)"); 
//_scopePlot->Spectrum(_spectrum, -1.0e-04, 1.0e+04, 1000000, "Frequency (Hz)", "Power (dB)"); 
			//	test w/linear limits
_scopePlot->Spectrum(_spectrum, -100, 20.0, 1000000, "Frequency (Hz)", "Power (dB)"); 
			break;
	}
}

void
CP2Scope::displayDataSlot() 
{
	//	display data SLOT -- activated by QTimer timeout() SIGNAL ... 
	std::vector<double> I;
	std::vector<double> Q;
	I.resize(500);
	Q.resize(500);
	float* p = ((float*)(m_pSocketBuf)) + 200;
	for (int i = 0; i <1000; i+=2) {	
		//	PIRAQ data: 
		I[i/2] = p[i]*1e5;
		Q[i/2] = p[i+1]*1e5;
	}
	switch (_plotType) {
		case ScopePlot::TIMESERIES:
			_scopePlot->TimeSeries(I, Q, -m_display_yScale, m_display_yScale, 1);
			break;
		case ScopePlot::IVSQ:
			_scopePlot->IvsQ(I, Q, -m_display_yScale, m_display_yScale, 1); 
				break;
	}
}


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

  // reorder and copy the results int _spectrum
  for (int i = 0 ; i < n/2; i++) {
    double pow =      _fftwData[i][0] * _fftwData[i][0] +
      _fftwData[i][1] * _fftwData[i][1];

    zeroMoment += pow;

    pow /= n*n;
    pow = 10.0*log10(pow);
    _spectrum[i+n/2] = pow;
  }

  for (int i = n/2 ; i < n; i++) {
    double pow =      _fftwData[i][0] * _fftwData[i][0] +
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

void
CP2Scope::initializeSocket()	{	//	?pass port#
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
	m_pSocketBuf = new CP2_PIRAQ_DATA_TYPE[1000000];

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


void 
CP2Scope::terminateSocket( void )	{	//	?pass port#
	if (m_pDataSocketNotifier)
		delete m_pDataSocketNotifier;

	if (m_pDataSocket)
		delete m_pDataSocket;

	if (m_pSocketBuf)
		delete [] m_pSocketBuf;
}
