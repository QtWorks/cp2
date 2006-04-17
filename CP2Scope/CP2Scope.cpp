
#include "CP2Scope.h"
#include <ScopePlot/ScopePlot.h>
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qspinbox.h>	
#include <qlcdnumber.h>

#include <Knob/Knob.h>

#include <winsock.h>
//#include <winsock2.h>
#include <iostream>

CP2Scope::CP2Scope():
m_pDataSocket(0),    
m_pDataSocketNotifier(0),
m_pSocketBuf(0),	
_plotType(ScopePlot::TIMESERIES)
{
	this->m_pSpinBoxPort->setMinValue(QTDSP_BASE_PORT);
	this->m_pSpinBoxPort->setMaxValue(QTDSP_BASE_PORT + 3);
	m_dataGramPort	= this->m_pSpinBoxPort->value();
	m_packetCount	= 0; 
	// create the socket that receives packets from WinDSP  
	initializeSocket();	
	connectDataRcv(); 

	// Start a timer to tell the display to update
	m_dataDisplayTimer = startTimer(CP2SCOPE_DISPLAY_REFRESH_INTERVAL);
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
	//	make the LCD more visible: 
	_PNerrors->setSegmentStyle(QLCDNumber::Flat);

	I.resize(1000);			//	default timeseries array size full range at 1KHz
	Q.resize(1000);

	_parametersSet = 0;		//	piraqx parameters not yet initialized from received data
	_dataSet = DATA_SET_PULSE;
m_DataSetGate = 50;		//!get spinner value
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
	int	i;
	int	hit;	//	hit in packet to display

	if	(m_packetCount == 0)	//	no data
		return;
	hit	= (pow(2,16)/(sizeof(PACKETHEADER) + 8*_gates)) - 1;	//	data set PULSE: display LAST pulse in this packet
	float* p = ((float*)(m_pSocketBuf)) + _pulseStride*hit + (sizeof(PACKETHEADER)/sizeof(float));	//	

	if	(_dataSet == DATA_SET_PULSE)	{	//	data set pulse: assemble once per timer event
		for (int i = 0; i < 2*_gates; i+=2) {	
#ifdef SCALE_DATA	//	PIRAQ data: scale to +/- 1.0 full scale PIRAQ3D_SCALE
			I[i/2] = p[i]*PIRAQ3D_SCALE;
			Q[i/2] = p[i+1]*PIRAQ3D_SCALE;
#else
			I[i/2] = p[i];
			Q[i/2] = p[i+1];
#endif
		}
		displayData();	
	}
//only if PULSE:	displayData();	
}

void 
CP2Scope::plotTypeSlot(bool b)
{
	switch (buttonGroup->selectedId()) {
		case 0:
			_plotType = ScopePlot::TIMESERIES;
			//	set knob for TIMESERIES: 
			this->yScaleKnob->setTitle("Ampl. Scale"); 
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
			this->yScaleKnob->setRange(-30.0, 30.0); 
			break;
		default: {}
   }

}

void 
CP2Scope::dataSetSlot(bool b)	{
	switch (buttonGroup2->selectedId()) {
		case 0:
			_dataSet = DATA_SET_PULSE;
			I.resize(m_fft_blockSize);	//	resize timeseries arrays, x-axis
			Q.resize(m_fft_blockSize);
			//	disable gate spinner
			break;
		case 1:
			_dataSet = DATA_SET_GATE;
			I.resize(_gates);	//	resize timeseries arrays, x-axis
			Q.resize(_gates);
			//	enable gate spinner
			break;
		default: {}
	}
}

void CP2Scope::yScaleKnob_valueChanged( double yScaleKnobSetting)	{	
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
	_parametersSet = 0;		//	request get parameters for this data channel

	_packetCount->setNum(m_packetCount);	// update display  
	initializeSocket();
	connectDataRcv(); 
}

void CP2Scope::DataSetGateSpinBox_valueChanged( int SpinBoxGate )	{
	//	change the gate for assembling data sets 
	m_DataSetGate    = SpinBoxGate;
}


void
CP2Scope::connectDataRcv()	// connect notifier to data-receive slot 
{
	connect(m_pDataSocketNotifier, SIGNAL(activated(int)), this, SLOT(dataSocketActivatedSlot(int)));
}

int
CP2Scope::getParameters( CP2_PIRAQ_DATA_TYPE[] )	// get program operating parameters from piraqx (or other) "housekeeping" header structure 
{
	_gates = *(int *)((m_pSocketBuf) + 108/sizeof(int));	//	gates: 32+28+48 as expected; 
	_pulseStride = (sizeof(PACKETHEADER) + 8*_gates)/sizeof(float);	//	stride in floats; 2nd term DATASIZE: 8 bytes per gate wired for now 
	//	compute #hits combined by piraq: equal in both piraq executable (CP2_DCCS3_1.out) and host applications
	_Nhits = UDPSENDSIZE / (HEADERSIZE + (8*_gates));	//	8 bytes per gate: define
	return(1);	//	condition this 
}

void 
CP2Scope::dataSocketActivatedSlot(int socket)
{
	static int errcount = 0;
	static int lasterrcount = 0;
	static int readBufLen, lastreadBufLen;
	static int j = 0;	//	Data Set Gate Idx; move to constructor to make accessible to datagram port change to reset
	int hit = 0;		//	index into N-hit packet
	int	r_c;	//	return_code
	//	define p: pointer to packet data
	float* p = ((float*)(m_pSocketBuf)) + _pulseStride*hit + (sizeof(PACKETHEADER)/sizeof(float));	//	
	uint8* PNptr = (uint8*)((char *)(m_pSocketBuf) + 32 + 28 + 32);	//	
	static	uint8	PN, lastPN;
	uint4	gates, lastgates; 

	m_packetCount++; 
	readBufLen = m_pDataSocket->readBlock((char *)m_pSocketBuf, sizeof(short)*1000000);
	if	(readBufLen != lastreadBufLen)	{	//	packet size changed: hmmmmm .... could be all right if CP2exec stop/start so generalize to cover this. 
//		errcount++;  
	}
	lastreadBufLen = readBufLen;
		if	((errcount % 100) == 10)	//	currently counting out-of-sequence PNs
			r_c = 5;	//	

	if	(!_parametersSet)	{	//	piraqx parameters need to be initialized from received data
		r_c = getParameters( m_pSocketBuf );	//	 
		if	(r_c == 1)	{		//	success
			if	(_dataSet == DATA_SET_PULSE)	{	//	data set along pulse
				I.resize(_gates);	//	resize timeseries arrays, x-axis
				Q.resize(_gates);
			}
			else	{	//if	(_dataSet == DATA_SET_GATE)	{	//	data set along beam: 1 gate per pulse
				I.resize(m_fft_blockSize);	//	resize timeseries arrays, x-axis
				Q.resize(m_fft_blockSize);
			}
			_parametersSet = 1; 
		}
	}
	//	NOT DONE! gather Nhits data per notification; deal w/partial sets ... ? could drop them continuity w/in set is what is required. 
	if	(_dataSet == DATA_SET_GATE)	{	//	gather data along one gate ("along the beam")#ifdef SCALE_DATA
		//	set p to 1st pulse in packet
		for	(hit = 0; hit < _Nhits; hit++)	{	//	all hits in packet
			PN	= *PNptr;
#ifdef	SCALE_DATA
			I[j/2] = p[2*m_DataSetGate]*PIRAQ3D_SCALE;	//	get selected gate data
			Q[j/2] = p[(2*m_DataSetGate)+1]*PIRAQ3D_SCALE;
#else
			I[j/2] = p[2*m_DataSetGate];
			Q[j/2] = p[2*m_DataSetGate+1];
#endif
			//	maintain p, detect full set, break
			p += _pulseStride; 
			j += 2; 
			if	(PN != lastPN + 1)	{	
				errcount++;	
				_PNerrors->display(errcount);	//	display accumulated PN errors
			}	//	we'll always get 1 
			lastPN = PN; 
			PNptr = (uint8 *)((char *)PNptr + _pulseStride*sizeof(float)); 
			if	(j >= m_fft_blockSize*2)	{	//	full set 
				if	(lasterrcount == errcount)	{	//	no missed pulses
					displayData(); 
				}
				j = 0; 
				lasterrcount = errcount; 
			}
		}
	}
	if (!(m_packetCount % 50))	{
		if(readBufLen > 0) {
			_packetCount->setNum(m_packetCount);	// update cumulative packet count 
		}
	}
}


void
CP2Scope::displayData() 
{
	// display data -- activated by QTimer timeout() if pulse set, or fft size assembled if gate set. 

//float p0, p1, p2, p3, p04;// p5; 
//p126, p127, p128, p129, p130, p131, p132, p254, p255; 
//p0 = p[0]; p1 = p[1]; p2 = p[2]; p3 = p[3]; p04 = p[4];// p5 = p[5]; 
//p126 = p[126]; p127 = p[127]; 
//p128 = p[128]; p129 = p[129]; 
//p130 = p[130]; p131 = p[131]; p132 = p[132]; 
//p254 = p[254]; p255 = p[255];

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
			//	use Spectrum() w/linear limits:
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
