
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

#include <time.h>


CP2Scope::CP2Scope():
m_pDataSocket(0),    
m_pDataSocketNotifier(0),
m_pSocketBuf(0),	
_plotType(ScopePlot::TIMESERIES)
{
	m_dataGramPort	= 3100;
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

	m_xFullScale = 980;
	I.resize(m_xFullScale);			//	default timeseries array size full range at 1KHz
	Q.resize(m_xFullScale);
	_dataSetSize = m_xFullScale;	//	size of data vector for display or calculations 
	_dataSet = DATA_SET_PULSE;
	//	display decimation, set to get ~50/sec
	m_pulseDisplayDecimation	= 50;	//	default w/prt = 1000Hz, timeseries data 
	m_productsDisplayDecimation	= 5;	//	default w/prt = 1000Hz, hits = 10, products data
	m_datagramPortBase			= 3100;	//	default timeseries data  
	m_dataChannel				= 1;			//	default to board-select spinner: get value from it

	m_DataSetGate = 50;		//!get spinner value
	_productType = 0; 

}

CP2Scope::~CP2Scope() {
	if (m_pDataSocketNotifier)
		delete m_pDataSocketNotifier;

	if (m_pDataSocket)
		delete m_pDataSocket;

	if (m_pSocketBuf)
		delete [] m_pSocketBuf;

}

void 
CP2Scope::timerEvent(QTimerEvent *e) {

	e = e;  // so we don't get the unreferenced warning
}

void 
CP2Scope::plotTypeSlot(bool b)
{
	b = b;  // so we don't get the unreferenced warning

	static int	_prevplotType = ScopePlot::TIMESERIES;

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
			m_display_yScale = m_yScaleMax / 10.0;	// start off reasonable
			this->yScaleKnob->setRange(m_yScaleMin, m_yScaleMax); 
			break;
		case 2:
			_plotType = ScopePlot::SPECTRUM;
			//	set knob for Power correction: 
			this->yScaleKnob->setTitle("Power Corr"); 
			this->yScaleKnob->setRange(-30.0, 30.0); 
			break;
			//	distiguish the various producs, within one _plotType
		case 3:
			_plotType = ScopePlot::PRODUCT;
			_productType = SVHVP; 
			//	set knob for Power correction: 
			this->yScaleKnob->setTitle("dBm Corr"); 
			this->yScaleKnob->setRange(-30.0, 30.0); 
			break;
		case 4:
			_plotType = ScopePlot::PRODUCT;
			_productType = SVHHP; 
			//	set knob for Power correction: 
			this->yScaleKnob->setTitle("dBm Corr"); 
			this->yScaleKnob->setRange(-30.0, 30.0); 
			break;
		case 5:
			_plotType = ScopePlot::PRODUCT;
			_productType = SVEL; 
			//	set knob for Power correction: 
			this->yScaleKnob->setTitle("N/A"); 
			//			this->yScaleKnob->setRange(-30.0, 30.0); 
			break;
		case 6:
			_plotType = ScopePlot::PRODUCT;
			_productType = SNCP; 
			//	set knob for Power correction: 
			this->yScaleKnob->setTitle("N/A"); 
			//			this->yScaleKnob->setRange(-30.0, 30.0); 
			break;
		case 7:
			_plotType = ScopePlot::PRODUCT;
			_productType = SWIDTH; 
			//	set knob for Power correction: 
			this->yScaleKnob->setTitle("N/A"); 
			//			this->yScaleKnob->setRange(-30.0, 30.0); 
			break;
		case 8:
			_plotType = ScopePlot::PRODUCT;
			_productType = SPHIDP; 
			//	set knob for Power correction: 
			this->yScaleKnob->setTitle("N/A"); 
			//			this->yScaleKnob->setRange(-30.0, 30.0); 
			break;
		case 9:
			_plotType = ScopePlot::PRODUCT;
			_productType = VREFL; 
			//	set knob for Power correction: 
			this->yScaleKnob->setTitle("N/A"); 
			//			this->yScaleKnob->setRange(-30.0, 30.0); 
			break;
		case 10:
			_plotType = ScopePlot::PRODUCT;
			_productType = HREFL; 
			//	set knob for Power correction: 
			this->yScaleKnob->setTitle("N/A"); 
			//			this->yScaleKnob->setRange(-30.0, 30.0); 
			break;
		case 11:
			_plotType = ScopePlot::PRODUCT;
			_productType = ZDR; 
			//	set knob for Power correction: 
			this->yScaleKnob->setTitle("N/A"); 
			//			this->yScaleKnob->setRange(-30.0, 30.0); 
			break;
		default: {}
	}

	resizeDataVectors();	//	resize data vectors
	_prevplotType = _plotType;
}

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
		//	enable x-scale spinner
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
		case 2:	//	power spectrum display
			_powerCorrection = yScaleKnobSetting; 
			break;
			//	add products displays
	}
}

void CP2Scope::DataChannelSpinBox_valueChanged( int dataChannel )	{
	//	change the input data channel
	m_dataChannel = dataChannel;	
	if	(_plotType < ScopePlot::PRODUCT)	//	timeseries-type display
		m_dataGramPort    = m_datagramPortBase + (dataChannel - 1);	//	 
	else	//	product-type display
		m_dataGramPort    = m_datagramPortBase + (dataChannel - 1) + 3;	//	3 = PIRAQs 
	//was here:	terminateSocket();
	m_packetCount = 0;		//	clear packet count

	_packetCount->setNum(m_packetCount);	// update display  
	initializeSocket();
	connectDataRcv(); 
}

void CP2Scope::DataSetGateSpinBox_valueChanged( int SpinBoxGate )	{
	//	change the gate for assembling data sets 
	m_DataSetGate    = SpinBoxGate;
}

void CP2Scope::xFullScaleBox_valueChanged( int xFullScale )	{
	//	change the gate for assembling data sets 
	if	(_plotType == ScopePlot::SPECTRUM)	//	fft: disable spinner
		return;
	m_xFullScale    = xFullScale;
	resizeDataVectors();	//	resize data vectors
}

void
CP2Scope::connectDataRcv()	// connect notifier to data-receive slot 
{
	connect(m_pDataSocketNotifier, SIGNAL(activated(int)), this, SLOT(dataSocketActivatedSlot(int)));
}

void 
CP2Scope::dataSocketActivatedSlot(int socket)
{
	socket = socket;  // so we don't get the unreferenced warning
	CP2Packet packet;
	int	readBufLen = m_pDataSocket->readBlock((char *)m_pSocketBuf, sizeof(short)*1000000);

	// put this datagram into a packet
	packet.setData(readBufLen, m_pSocketBuf);

	// Extract the pulses and process them.
	// From here on out, we are divorced from the
	// data transport.
	for (int i = 0; i < packet.numPulses(); i++) {
		CP2Pulse* pPulse = packet.getPulse(i);
		processPulse(pPulse);
	}

}

void
CP2Scope::processPulse(CP2Pulse* pPulse) 
{
	static int displayHit = 0; 
	static int displayProduct = 0; 

	float* data = &(pPulse->data[0]);

	switch (_plotType) {

	case ScopePlot::PRODUCT:
		{
			_momentsCompute.processPulse(data, 
				pPulse->header.gates,
				1.0e-6, 
				pPulse->header.el, 
				pPulse->header.az, 
				pPulse->header.pulse_num);	

			Beam* pBeam = _momentsCompute.getNewBeam();
			if (pBeam) {
				const Fields* fields = pBeam->getFields();
				int i;
				switch(_productType)	
				{
				case SVHVP:		//	compute S-band VH V Power in dBm for display
					for (i = 0; i < pPulse->header.gates; i++) {
						ProductData[i] = fields[i].dbmvc;
					}
					break;
				case SVHHP:		//	compute S-band VH H Power in dBm for display
					for (i = 0; i < pPulse->header.gates; i++) {
						ProductData[i] = fields[i].dbmhc;
					}
					break;
				case SVEL:		//	compute S-band velocity using V and H data
					for (i = 0; i < pPulse->header.gates; i++) {
						ProductData[i] = fields[i].vel;
					}
					break;
				case SNCP:		//	compute S-band NCP using calculation A2*A2+B2*B2/Pv+Ph
					break;
				case SWIDTH:	//  compute S-band width
					for (i = 0; i < pPulse->header.gates; i++) {
						ProductData[i] = fields[i].width;
					}
					break;			
				case SPHIDP:	//	compute S-band phidp 
					for (i = 0; i < pPulse->header.gates; i++) {
						ProductData[i] = fields[i].phidp;
					}
					break;			
				case VREFL:		//	compute S-band V reflectivity  
					for (i = 0; i < pPulse->header.gates; i++) {
						ProductData[i] = fields[i].dbzvc;
					}
					break;			
				case HREFL:		//	compute S-band H reflectivity  
					for (i = 0; i < pPulse->header.gates; i++) {
						ProductData[i] = fields[i].dbzhc;
					}
					break;			
				case ZDR:		//	compute S-band Zdr 
					for (i = 0; i < pPulse->header.gates; i++) {
						ProductData[i] = fields[i].zdr;
					}
					break;			
				}

				delete pBeam;

				displayData();

				m_packetCount++;
				_packetCount->setNum(m_packetCount);	// update cumulative packet count 
			}
			break;
		}

	default:
		{
			displayHit++;
			if	(displayHit >= m_pulseDisplayDecimation)	{	//	
				displayHit = displayHit % m_pulseDisplayDecimation;	//	index Nth hit since last display
				for (int i = 0; i < 2*m_xFullScale; i+=2) {	
					I[i/2] = pPulse->data[i]*PIRAQ3D_SCALE;
					Q[i/2] = pPulse->data[i+1]*PIRAQ3D_SCALE;
				}
				displayData();	
				displayHit = 0; 
			}
		}
	}
}

void
CP2Scope::displayData() 
{
	// display data -- called on decimation interval if pulse set, or fft size assembled if gate set. 

	switch (_plotType) {
	case ScopePlot::TIMESERIES:
		_scopePlot->TimeSeries(I, Q, -m_display_yScale, m_display_yScale, 1);
		break;
	case ScopePlot::IVSQ:
		_scopePlot->IvsQ(I, Q, -m_display_yScale, m_display_yScale, 1); 
		break;
	case ScopePlot::SPECTRUM:
		// correct unscaled power data using knob setting: 
		//			for(j = 0; j < m_fft_blockSize; j++)	{
		//				_spectrum[j] += _powerCorrection;
		//			}
		//	use Spectrum() w/linear limits, linear data on y-axis:
		_scopePlot->Spectrum(ProductData, -100, 20.0, 1000000, false, "Frequency (Hz)", "Power (dB)");	
		break;
	case ScopePlot::PRODUCT:
		//	proliferate product types here:
		switch(_productType)	{
	case SVHVP:		//	compute S-band VH V Power in dBm for display
		_scopePlot->Product(ProductData, _productType, -80, 20.0, m_xFullScale, "Gate", "S V Power (dBm)"); 
		break;
	case SVHHP:		//	compute S-band VH H Power in dBm for display
		_scopePlot->Product(ProductData, _productType, -80, 20.0, m_xFullScale, "Gate", "S H Power (dBm)"); 
		break;
	case SVEL:		//	compute S-band velocity using V and H data
		_scopePlot->Product(ProductData, _productType, -30.0, 30.0, m_xFullScale, "Gate", "Velocity (m/s)"); 
		break;
	case SNCP:		//	compute S-band NCP using calculation A2*A2+B2*B2/Pv+Ph
		_scopePlot->Product(ProductData, _productType, 0.0, 2.0, m_xFullScale, "Gate", "NCP"); 
		break;
	case SWIDTH:
		_scopePlot->Product(ProductData, _productType, 0.0, 30.0, m_xFullScale, "Gate", "Spectral Width"); 
		break;			
	case SPHIDP:
		//	compute S-band phidp 
		_scopePlot->Product(ProductData, _productType, -180.0, 180.0, m_xFullScale, "Gate", "phidp degrees"); 
		break;			
	case VREFL:
		//	compute S-band V reflectivity  
		_scopePlot->Product(ProductData, _productType, -100.0, 100.0, m_xFullScale, "Gate", "V Reflectivity"); 
		break;			
	case HREFL:
		//	compute S-band H reflectivity  
		_scopePlot->Product(ProductData, _productType, -100.0, 100.0, m_xFullScale, "Gate", "H Reflectivity"); 
		break;			
	case ZDR:
		//	compute S-band Zdr 
		_scopePlot->Product(ProductData, _productType, -10.0, 10.0, m_xFullScale, "Gate", "Zdr"); 
		break;			
		}
		break;
	}
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
