
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
//m_pDataSocket(0),    
//m_pDataSocketNotifier(0),
//m_pSocketBuf(0),	
_plotType(ScopePlot::TIMESERIES)
{
	m_dataGramPort	= QTDSP_BASE_PORT;
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
	//	allocate S-band APB packet storage
	SABP = new CP2_PIRAQ_DATA_TYPE[16384]; 

	I.resize(1000);			//	default timeseries array size full range at 1KHz
	Q.resize(1000);
	_dataSetSize = 1000;	//	size of data vector for display or calculations 
	_parametersSet = 0;		//	piraqx parameters not yet initialized from received data
	_dataSet = DATA_SET_PULSE;
	//	display decimation, set to get ~50/sec
	m_pulseDisplayDecimation	= 50;	//	default w/prt = 1000Hz, timeseries data 
	m_productsDisplayDecimation	= 5;	//	default w/prt = 1000Hz, hits = 10, products data
	m_datagramPortBase			= QTDSP_BASE_PORT;	//	default timeseries data  
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

	fftw_destroy_plan(_fftwPlan);

	//?delete timer? 
}

void 
CP2Scope::timerEvent(QTimerEvent *e) {
	int	i;
	int	hit;	//	hit in packet to display

return;	//!
	if	(m_packetCount == 0)	//	no data
		return;
//	hit	= (pow(2,16)/(sizeof(PACKETHEADER) + 8*_gates)) - 1;	//	data set PULSE: display LAST pulse in this N-hit packet
hit	= _Nhits - 1;	//	data set PULSE: display LAST pulse in this N-hit packet
	float* p = ((float*)(m_pSocketBuf)) + _pulseStride*hit + (sizeof(PACKETHEADER)/sizeof(float));	//	

	if	(_plotType < ScopePlot::PRODUCT)	{	//	timeseries data
		if	(_dataSet == DATA_SET_PULSE)	{	//	data set pulse: assemble once per timer event
			for (int i = 0; i < 2*m_xFullScale; i+=2) {	
				I[i/2] = p[i]*PIRAQ3D_SCALE;
				Q[i/2] = p[i+1]*PIRAQ3D_SCALE;
			}
			displayData();	
		}
	}
	else if (_plotType >= ScopePlot::PRODUCT)	{	//	S-band products data 
		displayData();	
	}
}

void 
CP2Scope::plotTypeSlot(bool b)
{
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
	//	plot type changed timeseries to products or vice versa
	//	define separator value more generally:
	if	(((_plotType <= ScopePlot::SPECTRUM) && (_prevplotType > ScopePlot::SPECTRUM)) || ((_plotType > ScopePlot::SPECTRUM) && (_prevplotType <= ScopePlot::SPECTRUM)))
		ChangeDatagramPort( m_dataChannel ); 

	resizeDataVectors();	//	resize data vectors
	_prevplotType = _plotType;
}

void 
CP2Scope::dataSetSlot(bool b)	{
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
	if	(_plotType == ScopePlot::SPECTRUM)	{	//	fft
		I.resize(m_fft_blockSize);	//	resize timeseries arrays, x-axis
		Q.resize(m_fft_blockSize);
		_dataSetSize = m_fft_blockSize; 
//	disable x-scale spinner
	}
	else if	(_plotType >= ScopePlot::PRODUCT)	{	//	current product to display
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
	terminateSocket();
	if	(_plotType < ScopePlot::PRODUCT)	//	timeseries-type display
		m_dataGramPort    = m_datagramPortBase + (dataChannel - 1);	//	 
	else	//	product-type display
		m_dataGramPort    = m_datagramPortBase + (dataChannel - 1) + 3;	//	3 = PIRAQs 
//was here:	terminateSocket();
	m_packetCount = 0;		//	clear packet count
	_parametersSet = 0;		//	request get parameters for this data channel

	_packetCount->setNum(m_packetCount);	// update display  
	initializeSocket();
	connectDataRcv(); 
}

void CP2Scope::ChangeDatagramPort( int dataChannel )	{
	//	change the input data port
	terminateSocket();
	if	(_plotType < ScopePlot::PRODUCT)	//	timeseries-type display
		m_dataGramPort    = m_datagramPortBase + (dataChannel - 1);	//	 
	else	//	product-type display
		m_dataGramPort    = m_datagramPortBase + (dataChannel - 1) + 3;	//	3 = PIRAQs 
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

// get program operating parameters from piraqx (or other) "housekeeping" header structure; resize arrays, etc. 
int
CP2Scope::getParameters( CP2_PIRAQ_DATA_TYPE[] )	
{
	//	piraqx parameters (or other header) "housekeeping" for use by program: data types per piraqx.h
	_gates = *(int *)((m_pSocketBuf) + 108/sizeof(int));	//	gates: 32+28+48 as expected;
	_hits = *(int *)((m_pSocketBuf) + 116/sizeof(int));		//	hits: 32+28+54 as expected;
	_prt = *(float *)((m_pSocketBuf) + 132/sizeof(float));	//	dataformat: 32+28+72
	_dataformat = *(uint4 *)((m_pSocketBuf) + 84/sizeof(float));	//	offset to dataformat: 32+28+24
	//	piraqx parameters for power calculations
	_data_sys_sat = *(float *)((m_pSocketBuf) + 504/sizeof(float));	//	data_sys_sat: per piraqx.xls, offset = 32+28+444
	_receiver_gain = *(float *)((m_pSocketBuf) + 492/sizeof(float));		//	horizontal receiver gain: offset = 32+28+432
	_vreceiver_gain = *(float *)((m_pSocketBuf) + 696/sizeof(float));	//	vertical receiver gain: offset = 32+28+636
	_noise_power = *(float *)((m_pSocketBuf) + 488/sizeof(float));	//	noise power horizontal channel offset = 32+28+428
	_vnoise_power = *(float *)((m_pSocketBuf) + 692/sizeof(float));	//	noise power vertical channel offset = 32+28+632
	//	parameters from piraqx for velocity calculations
	_frequency = *(float *)((m_pSocketBuf) + 480/sizeof(float));	//	radar transmit frequency: offset = 32+28+420
	_phaseoffset = *(float *)((m_pSocketBuf) + 528/sizeof(float));	//	phase offset between V and H channels: offset = 32+28+468
	//	parameters from piraqx for reflectivity calculations
	_rconst = *(float *)((m_pSocketBuf) + 524/sizeof(float));	//	: offset = 32+28+464 
	_xmit_pulsewidth = *(float *)((m_pSocketBuf) + 520/sizeof(float));	//	: offset = 32+28+460 
	_rcvr_pulsewidth = *(float *)((m_pSocketBuf) + 128/sizeof(float));	//	: offset = 32+28+68
	_peak_power = *(float *)((m_pSocketBuf) + 712/sizeof(float));	//	: offset = 32+28+652 
	_xmit_power = *(float *)((m_pSocketBuf) + 356/sizeof(float));	//	: offset = 32+28+296 
	_vxmit_power = *(float *)((m_pSocketBuf) + 684/sizeof(float));	//	: offset = 32+28+624 
	_antenna_gain = *(float *)((m_pSocketBuf) + 508/sizeof(float));	//	: offset = 32+28+448 
	_vantenna_gain = *(float *)((m_pSocketBuf) + 700/sizeof(float));	//	: offset = 32+28+640 
	_zdr_fudge_factor = *(float *)((m_pSocketBuf) + 532/sizeof(float));	//	: offset = 32+28+472 
	_zdr_bias = *(float *)((m_pSocketBuf) + 732/sizeof(float));	//	: offset = 32+28+672
	_noise_phase_offset = *(float *)((m_pSocketBuf) + 736/sizeof(float));	//	noise phase offset get from spare[0]: offset = 32+28+692
	//	operating parameters derived from piraqx and N-hit implementation:	
	_pulseStride = (sizeof(PACKETHEADER) + 8*_gates)/sizeof(float);	//	stride in floats; 2nd term DATASIZE: 8 bytes per gate wired for now 
	_PNOffset = 32 + 28 + 32;	//	pulsenumber offset from begin of piraqx packet: sizeof(COMMAND) + sizeof(UDPHEADER) + offset to PN
	_IQdataOffset = sizeof(PACKETHEADER);	//	offset in bytes to begin of data in (piraqx) packet
	m_pulseDisplayDecimation = ceil(1.0 / (DISPLAY_DECIMATION * _prt)); 
	m_productsDisplayDecimation = (m_pulseDisplayDecimation / _hits) + 1;	//	round up: result is a small number
	//	compute #hits combined by piraq: equal in both piraq executable (CP2_DCCS3_1.out) and host applications
	//	8 bytes per gate: define per SVH APB data type: 
	_Nhits = UDPSENDSIZEMAX / (HEADERSIZE + (8*_gates));	
	if	(_Nhits % 2)	//	odd #hits
		_Nhits--;		//	make it even: PIRAQ sends even #hits per packet

	_parametersSet = 1;	//	condition this	
	resizeDataVectors();	//	resize data vectors w/current operating parameters
	this->xFullScale->setValue( _gates );	//	set x-axis to full scale

	return(_parametersSet);	 
}

//	this needs clean separation of timeseries and products display functions
void 
CP2Scope::dataSocketActivatedSlot(int socket)
{
	static int errcount = 0;
	static int lasterrcount = 0;
	static int readBufLen, lastreadBufLen;
	static int j = 0;	//	Data Set Gate Idx; move to constructor to make accessible to datagram port change to reset
	int hit = 0;		//	index into N-hit packet
	int	r_c;	//	return_code
	//	define p: pointer to packet data. set to start of data, 1st pulse in packet.
	float* p = ((float*)(m_pSocketBuf)) + _pulseStride*hit + (sizeof(PACKETHEADER)/sizeof(float));	//	
	uint8* PNptr = (uint8*)((char *)(m_pSocketBuf) + 32 + 28 + 32);	//	
	static	uint8	PN, lastPN;
	uint4	gates, lastgates; 
	static int displayHit = 0; 
	static int displayProduct = 0; 

	m_packetCount++; 
	readBufLen = m_pDataSocket->readBlock((char *)m_pSocketBuf, sizeof(short)*1000000);
	if	(readBufLen != lastreadBufLen)	{	//	packet size changed: hmmmmm .... could be all right if CP2exec stop/start so generalize to cover this. 
		errcount++;  
	}
	if	(!_parametersSet)	{	//	piraqx parameters need to be initialized from received data
		r_c = getParameters( m_pSocketBuf );	//	 
		if	(r_c == 1)	{		//	success
//			_parametersSet = 1; 
			if	(_plotType < ScopePlot::PRODUCT)	{	//	timeseries type display (presently 0, 1, or 2)
			}
			else if (_plotType >= ScopePlot::PRODUCT)	{	//	S-band ABPs: compute products 
//				SABP.resize(8*_gates + sizeof(PACKETHEADER)/4);	//	define 8; S-band "ABPABPAB" data + ample for piraqx header 
//				ProductData.resize(_gates);		//	one power per gate 
			}
		}
	}

	//	all timeseries-related: from here ...
	if	(_plotType < ScopePlot::PRODUCT)	{	//	timeseries type display (presently 0, 1, or 2)
		if	(_dataSet == DATA_SET_GATE)	{	//	gather data along one gate ("along the beam")
			for	(hit = 0; hit < _Nhits; hit++)	{	//	all hits in packet
				PN	= *PNptr;
				I[j/2] = p[2*m_DataSetGate]*PIRAQ3D_SCALE;	//	get selected gate data
				Q[j/2] = p[(2*m_DataSetGate)+1]*PIRAQ3D_SCALE;
				//	maintain p, detect full set, break
				p += _pulseStride; 
				j += 2; 
				if	(PN != lastPN + 1)	{	
					errcount++;	
					_PNerrors->display(errcount);	//	display accumulated PN errors
				}	//	we'll always get 1 
				lastPN = PN; 
				PNptr = (uint8 *)((char *)PNptr + _pulseStride*sizeof(float)); 
				if	(j >= _dataSetSize*2)	{	//	set size of data vectors for display or calculations 
					if	(lasterrcount == errcount)	{	//	no missed pulses
						displayData(); 
					}
					j = 0; 
					lasterrcount = errcount; 
				}
			}	//	end for _Nhits
		}	//	end if (_dataSet == DATA_SET_GATE)
		else if	(_dataSet == DATA_SET_PULSE)	{	//	DATA_SET_PULSE: count pulses until find Nth: display it. 
			displayHit += _Nhits;
			if	(displayHit >= m_pulseDisplayDecimation)	{	//	
				displayHit = displayHit % m_pulseDisplayDecimation;	//	index Nth hit since last display
				p = ((float*)(m_pSocketBuf)) + _pulseStride*hit + (sizeof(PACKETHEADER)/sizeof(float));	//	
				for (int i = 0; i < 2*m_xFullScale; i+=2) {	
					I[i/2] = p[i]*PIRAQ3D_SCALE;
					Q[i/2] = p[i+1]*PIRAQ3D_SCALE;
				}
				displayData();	
				displayHit = 0; 
			}
		}
	}	//	... to here: end if (_plotType < ScopePlot::PRODUCT)
	//	products-type display
	else if (_plotType >= ScopePlot::PRODUCT)	{	
		displayProduct++;
		if	(displayProduct < m_productsDisplayDecimation)	//	1: display every ~50mSec
			return;
		//!again proliferate product types: switch _product Type
		switch(_plotType)	{
			case(ScopePlot::PRODUCT):	//	
				if	(displayProduct == m_productsDisplayDecimation)	{	//	1: display every ~50mSec
					memmove((void *)SABP,(const void *)m_pSocketBuf,readBufLen);	//	copy received ABP packet 
					displayData();	
					displayProduct = 0;
				}
				break;
		}
	}
	if (!(m_packetCount % m_pulseDisplayDecimation))	{
		if(readBufLen > 0) {
			_packetCount->setNum(m_packetCount);	// update cumulative packet count 
		}
	}	
	lastreadBufLen = readBufLen; 
}


void
CP2Scope::displayData() 
{
	// display data -- called on decimation interval if pulse set, or fft size assembled if gate set. 

    double zeroMoment; 
	unsigned int j; 
	CP2_PIRAQ_DATA_TYPE * SVHData;	//	 

	//	locals for calculating Power
	double OffsetTo_dBm;	//	factor for correcting power to dBm scale 
	double rawNoisePower;
	double SVP_;
	//	locals for calculating velocity
	double v1a, v2a, v, theta, dp, ph_off; 
	double angle_to_velocity_scale_factor;
	//	locals for calculating NCP, spectral width
	double A2, B2;	//	lag2 A, B 
	double Pv, Ph; 
	double widthconst;
	double NCP;
	//	locals for calculating reflectivity and Zdr
	double v_channel_radar_constant, h_channel_radar_constant;
	double range_correction = 0.0;
	double HOffsetTo_dBm, VOffsetTo_dBm, HrawNoisePower, VrawNoisePower; 
	CP2_PIRAQ_DATA_TYPE * SVHDataH, * SVHDataV; 

	switch (_plotType) {
		case ScopePlot::TIMESERIES:
			_scopePlot->TimeSeries(I, Q, -m_display_yScale, m_display_yScale, 1);
			break;
		case ScopePlot::IVSQ:
			_scopePlot->IvsQ(I, Q, -m_display_yScale, m_display_yScale, 1); 
			break;
		case ScopePlot::SPECTRUM:
			_spectrum.resize(m_fft_blockSize);	//	probably belongs somewhere else
			for(j = 0; j < m_fft_blockSize; j++)	{
			// transfer the data to the fftw input space
				_fftwData[j][0] = I[j];
				_fftwData[j][1] = Q[j];
			}
		    zeroMoment = powerSpectrum();
			// correct unscaled power data using knob setting: 
			for(j = 0; j < m_fft_blockSize; j++)	{
				_spectrum[j] += _powerCorrection;
			}
			//	use Spectrum() w/linear limits, linear data on y-axis:
			_scopePlot->Spectrum(_spectrum, -100, 20.0, 1000000, false, "Frequency (Hz)", "Power (dB)");	
			break;
		case ScopePlot::PRODUCT:
			//	proliferate product types here:
			switch(_productType)	{
				case SVHVP:		//	compute S-band VH V Power in dBm for display
					OffsetTo_dBm = _data_sys_sat - _vreceiver_gain + 10.0 * log10(2.0);	// correct for half power measurement 
					// compute "uncorrected" noise power -- noise power on raw scale, from config'd dBm: 
					rawNoisePower = (_vnoise_power > -10.0) ? 0.0 : pow(10.0,((_vnoise_power - OffsetTo_dBm)/10.0)); 
					SVHData = SABP + _IQdataOffset/sizeof(float) + 2;	//	+2 offset to V pulse raw Power
					for (j = 0; j < m_xFullScale; j++)	{	//	all gates to display
//						SVP_ = *SVHData;
//						ProductData[j] -= rawNoisePower;	//	correct the raw power ... rawNoisePower just not visible in debugger
#if 0	//	as it was:
						if	(ProductData[j] <= 0.0)		//	corrected to negative value
							ProductData[j] = pow(10.0,((NOISE_FLOOR - OffsetTo_dBm) / 10.0)); // put in something small: gets NOISE_LIMIT dBm
						ProductData[j] = (double)(10.0*log10(*SVHData) + OffsetTo_dBm);	//	scale result to dBm 
#else
						SVP_ = *SVHData;
						*SVHData -= rawNoisePower;	//	correct the raw power ... rawNoisePower just not visible in debugger
						if	(*SVHData <= 0.0)		//	corrected to negative value
							*SVHData = pow(10.0,((NOISE_FLOOR - OffsetTo_dBm) / 10.0)); // put in something small: gets NOISE_LIMIT dBm
						else
							ProductData[j] = (double)(10.0*log10(*SVHData) + OffsetTo_dBm);	//	scale result to dBm 
#endif
						SVHData += SVHABP_STRIDE;	//	index next gate
					}
					_scopePlot->Product(ProductData, _productType, -80, 20.0, m_xFullScale, "Gate", "S V Power (dBm)"); 
					break;
				case SVHHP:		//	compute S-band VH H Power in dBm for display
					OffsetTo_dBm = _data_sys_sat - _receiver_gain + 10.0 * log10(2.0);	//	log10(2.0): correct for half power measurement 
					// compute "uncorrected" noise power -- noise power on raw scale, from config'd dBm: 
					rawNoisePower = (_noise_power > -10.0) ? 0.0 : pow(10.0,((_noise_power - OffsetTo_dBm)/10.0)); 
					SVHData = SABP + _IQdataOffset/sizeof(float) + 5;	//	+5 offset to H pulse raw Power
					for (j = 0; j < m_xFullScale; j++)	{	//	all gates to display
//						SVP_ = *SVHData;
//						ProductData[j] -= rawNoisePower;	//	correct the raw power ... rawNoisePower just not visible in debugger
						if	(ProductData[j] <= 0.0)		//	corrected to negative value
							ProductData[j] = pow(10.0,((NOISE_FLOOR - OffsetTo_dBm) / 10.0)); // put in something small: gets NOISE_LIMIT dBm
						ProductData[j] = (double)(10.0*log10(*SVHData) + OffsetTo_dBm);	//	scale result to dBm 
						SVHData += SVHABP_STRIDE;	//	index next gate
					}
					_scopePlot->Product(ProductData, _productType, -80, 20.0, m_xFullScale, "Gate", "S H Power (dBm)"); 
					break;
				case SVEL:		//	compute S-band velocity using V and H data
					angle_to_velocity_scale_factor = C / (2.0 * _frequency * 2.0 * M_PI * _prt);
					ph_off = _noise_phase_offset * M_PI / 180.0;	//	SPol sets this phase offset to 20 deg 
					SVHData = SABP + _IQdataOffset/sizeof(float);	//	point to data
#if 0				//	Hubbert method: 
					double	a, b, c, d; 
					double	alpha, beta; 
					double	phidp; 
					double	phivelocity;

					//	+1 offset to V-H pulse B, 0 to A, +4, +3 H-V pulse B, A
					for (j = 0; j < m_xFullScale; j++)	{	//	all gates to display
						//	Hubbert method: 
						a = *(SVHData + 3); b = *(SVHData + 4); c = *(SVHData + 0); d = *(SVHData + 1);
						phidp = atan2( (b*c - a*d), (a*c + b*d)) / 2.0; 
						alpha = cos(phidp); beta = sin(phidp); 
						phivelocity = atan2( (b*alpha - (a*beta)), (a*alpha + b*beta)); 

						/* velocity in m/s */
						ProductData[j] = phivelocity * angle_to_velocity_scale_factor; 
						SVHData += SVHABP_STRIDE;	//	index next gate
					}
#else				//	Mitch method: 
					//	+1 offset to V pulse B
					for (j = 0; j < m_xFullScale; j++)	{	//	all gates to display
						/* subtract out the system phase from v1a */
						v1a = atan2(*(SVHData + 1),*(SVHData + 0));	//	S-band H-V B,A
						v2a = atan2(*(SVHData + 4),*(SVHData + 3));	//	S-band V-H B,A
						v1a -= _phaseoffset;
						if	(v1a < -M_PI)	
							v1a += 2.0 * M_PI;
						else if (v1a > M_PI)
							v1a -= 2.0 * M_PI;
						/* add in the system phase to v2a */
						v2a += _phaseoffset;
						if	(v2a < -M_PI)
							v2a += 2.0 * M_PI;
						else if (v2a > M_PI)
							v2a -= 2.0 * M_PI;
						/* compute the total difference */
						theta = v2a - v1a;
						if (theta > M_PI)
							theta -= 2.0 * M_PI;
						else if (theta < -M_PI)
							theta += 2.0 * M_PI;      
						/* figure the differential phase (from - 20 to +160) */
						dp = theta * 0.5;
						if (dp < -ph_off)
							dp += M_PI;
						/* note: dp cannot be greater than +160, range is +/- 90 */        
						/* compute the velocity */
						v = v1a + dp;
						if (v < -M_PI)
							v += 2.0 * M_PI;
						else if (v > M_PI)
							v -= 2.0 * M_PI;
						/* velocity in m/s */
						ProductData[j] = v * angle_to_velocity_scale_factor; 
						SVHData += SVHABP_STRIDE;	//	index next gate
					}
#endif
					//	compute velocity limits from prt, frequency: 
					_scopePlot->Product(ProductData, _productType, -30.0, 30.0, m_xFullScale, "Gate", "Velocity (m/s)"); 
					break;
				case SNCP:		//	compute S-band NCP using calculation A2*A2+B2*B2/Pv+Ph
					SVHData = SABP + _IQdataOffset/sizeof(float);	//	point to data
					for (j = 0; j < m_xFullScale; j++)	{	//	all gates to display
						//	calculate S-band NCP:
						A2 = *(SVHData + 6); B2 = *(SVHData + 7); Pv = *(SVHData + 2); Ph = *(SVHData + 5);	//	
						ProductData[j] = sqrt(A2*A2 + B2*B2) / (Pv + Ph);
						SVHData += SVHABP_STRIDE;	//	index next gate
					}
					_scopePlot->Product(ProductData, _productType, 0.0, 1.0, m_xFullScale, "Gate", "NCP"); 
					break;
				case SWIDTH:
				//	compute S-band spectral width 
					SVHData = SABP + _IQdataOffset/sizeof(float);	//	point to data
					widthconst = (C / _frequency) / _prt / (4.0 * sqrt(2.0) * M_PI); 
					for (j = 0; j < m_xFullScale; j++)	{	//	all gates to display
						//	calculate S-band Spectral Width:
						A2 = *(SVHData + 6); B2 = *(SVHData + 7); Pv = *(SVHData + 2); Ph = *(SVHData + 5);	//	
						NCP = sqrt(A2*A2 + B2*B2) / (Pv + Ph);	//	calculate NCP
						if	(log(NCP) > 0.0)
							NCP = 1.0; 
						ProductData[j] = widthconst * sqrt(-log(NCP));
						SVHData += SVHABP_STRIDE;	//	index next gate
					}
					_scopePlot->Product(ProductData, _productType, 0.0, 30.0, m_xFullScale, "Gate", "Spectral Width (m/s)"); 
					break;			
				case SPHIDP:
				//	compute S-band phidp 
					SVHData = SABP + _IQdataOffset/sizeof(float);	//	point to data
					ph_off = _noise_phase_offset * M_PI / 180.0;	//	SPol sets this phase offset to be 20 deg */
					for (j = 0; j < m_xFullScale; j++)	{	//	all gates to display
						v1a = atan2(*(SVHData + 1),*(SVHData + 0));	//	S-band H-V B,A
						v2a = atan2(*(SVHData + 4),*(SVHData + 3));	//	S-band V-H B,A
						theta = v2a - v1a;
						if (theta > M_PI)
							theta -= 2.0 * M_PI;
						else if (theta < -M_PI)
							theta += 2.0 * M_PI;      
						/* figure the differential phase (from - 20 to +160) */
						dp = theta * 0.5;
						if (dp < -ph_off)
							dp += M_PI;
						/* note: dp cannot be greater than +160, range is +/- 90 */        
						//	calculate S-band phidp in degrees:
						ProductData[j] = dp * 180.0 / M_PI;
						SVHData += SVHABP_STRIDE;	//	index next gate
					}
					_scopePlot->Product(ProductData, _productType, -180.0, 180.0, m_xFullScale, "Gate", "phidp degrees"); 
					break;			
				case VREFL:
				//	compute S-band V reflectivity  
					SVHData = SABP + _IQdataOffset/sizeof(float);	//	point to data
					v_channel_radar_constant = _rconst - 20.0 * log10(_xmit_pulsewidth / _rcvr_pulsewidth)
												+ 10.0 * log10(_peak_power / _vxmit_power)
												+ 2.0 * (_antenna_gain - _vantenna_gain);	
					OffsetTo_dBm = _data_sys_sat - _vreceiver_gain + 10.0 * log10(2.0);	// correct for half power measurement 
					// compute "uncorrected" noise power -- noise power on raw scale, from config'd dBm: 
					rawNoisePower = (_vnoise_power > -10.0) ? 0.0 : pow(10.0,((_vnoise_power - OffsetTo_dBm)/10.0)); 
					SVHData = SABP + _IQdataOffset/sizeof(float) + 2;	//	+2 offset to V pulse raw Power
					for (j = 0; j < m_xFullScale; j++)	{	//	all gates to display
						if	(j)	//	range not zero 
							range_correction = 20.0 * log10(j * 0.0005 * C * _rcvr_pulsewidth);
						//	calculate S-band V Power, then reflectivity: 
						*SVHData -= rawNoisePower;	//	correct the raw power 
						if	(*SVHData <= 0.0)		//	corrected to negative value
							*SVHData = pow(10.0,((NOISE_FLOOR - OffsetTo_dBm) / 10.0)); // put in something small: gets NOISE_LIMIT dBm
//						else
							ProductData[j] = (double)(10.0*log10(*SVHData) + OffsetTo_dBm) + v_channel_radar_constant + range_correction;	//	 
						SVHData += SVHABP_STRIDE;	//	index next gate
					}
					_scopePlot->Product(ProductData, _productType, -100.0, 100.0, m_xFullScale, "Gate", "V Reflectivity"); 
					break;			
				case HREFL:
				//	compute S-band H reflectivity  
					h_channel_radar_constant = _rconst - 20.0 * log10(_xmit_pulsewidth / _rcvr_pulsewidth)
												+ 10.0 * log10(_peak_power / _xmit_power);
					OffsetTo_dBm = _data_sys_sat - _receiver_gain + 10.0 * log10(2.0);	//	log10(2.0): correct for half power measurement 
					// compute "uncorrected" noise power -- noise power on raw scale, from config'd dBm: 
					rawNoisePower = (_noise_power > -10.0) ? 0.0 : pow(10.0,((_noise_power - OffsetTo_dBm)/10.0)); 
					SVHData = SABP + _IQdataOffset/sizeof(float) + 5;	//	+5 offset to H pulse raw Power
					for (j = 0; j < m_xFullScale; j++)	{	//	all gates to display
						if	(j)	//	range not zero 
							range_correction = 20.0 * log10(j * 0.0005 * C * _rcvr_pulsewidth);
						//	calculate S-band H Power, then adjust to reflectivity: 
						*SVHData -= rawNoisePower;	//	correct the raw power 
						if	(*SVHData <= 0.0)		//	corrected to negative value
							*SVHData = pow(10.0,((NOISE_FLOOR - OffsetTo_dBm) / 10.0)); // put in something small: gets NOISE_LIMIT dBm
						else
							ProductData[j] = (double)(10.0*log10(*SVHData) + OffsetTo_dBm) + h_channel_radar_constant + range_correction;	//	 
						SVHData += SVHABP_STRIDE;	//	index next gate
					}
					_scopePlot->Product(ProductData, _productType, -100.0, 100.0, m_xFullScale, "Gate", "H Reflectivity"); 
					break;			
				case ZDR:
				//	compute S-band Zdr 
					h_channel_radar_constant = _rconst - 20.0 * log10(_xmit_pulsewidth / _rcvr_pulsewidth)
												+ 10.0 * log10(_peak_power / _xmit_power);
					v_channel_radar_constant = _rconst - 20.0 * log10(_xmit_pulsewidth / _rcvr_pulsewidth)
												+ 10.0 * log10(_peak_power / _vxmit_power)
												+ 2.0 * (_antenna_gain - _vantenna_gain);	
					HOffsetTo_dBm = _data_sys_sat - _receiver_gain + 10.0 * log10(2.0);	// correct for half power measurement 
					VOffsetTo_dBm = _data_sys_sat - _vreceiver_gain + 10.0 * log10(2.0);	
					// compute "uncorrected" noise powers -- noise power on raw scale, from config'd dBm: 
					HrawNoisePower = (_noise_power > -10.0) ? 0.0 : pow(10.0,((_noise_power - HOffsetTo_dBm)/10.0)); 
					VrawNoisePower = (_vnoise_power > -10.0) ? 0.0 : pow(10.0,((_vnoise_power - VOffsetTo_dBm)/10.0)); 
					//	point to H, V power data
					SVHDataH = SABP + _IQdataOffset/sizeof(float) + 5;	
					SVHDataV = SABP + _IQdataOffset/sizeof(float) + 2;
					double SVZ, SHZ;	//	S-band V, H reflectivity
					for (j = 0; j < m_xFullScale; j++)	{	//	all gates to display
						if	(j)	//	range not zero 
							range_correction = 20.0 * log10(j * 0.0005 * C * _rcvr_pulsewidth);
						//	calculate S-band H and V Power, then adjust to reflectivity, and compute Zdr: 
						*SVHDataH -= HrawNoisePower;	//	correct the raw power 
						if	(*SVHDataH <= 0.0)		//	corrected to negative value
							*SVHDataH = pow(10.0,((NOISE_FLOOR - HOffsetTo_dBm) / 10.0)); // put in something small: gets NOISE_LIMIT dBm
						else
							SHZ = (double)(10.0*log10(*SVHDataH) + HOffsetTo_dBm) + h_channel_radar_constant + range_correction;	//	 
						*SVHDataV -= VrawNoisePower;	//	correct the raw power 
						if	(*SVHDataV <= 0.0)		//	corrected to negative value
							*SVHDataV = pow(10.0,((NOISE_FLOOR - VOffsetTo_dBm) / 10.0)); // put in something small: gets NOISE_LIMIT dBm
						else
							SVZ = (double)(10.0*log10(*SVHDataV) + VOffsetTo_dBm) + v_channel_radar_constant + range_correction;	//	 
						//	compute Zdr
						ProductData[j] = SHZ - SVZ + _zdr_fudge_factor + _zdr_bias;
						SVHDataH += SVHABP_STRIDE; SVHDataV += SVHABP_STRIDE;	//	index next gate
					}
					_scopePlot->Product(ProductData, _productType, -10.0, 10.0, m_xFullScale, "Gate", "Zdr"); 
					break;			
			}
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
