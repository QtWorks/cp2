#include "CP2PPI.h"
#include "../include/CP2.h"
#include <PPI/PPI.h>
#include <qlcdnumber.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qlayout.h>

#include <iostream>

struct productDisplayConfiguration SBandProductDisplayConfiguration[NUM_SBANDPRODUCTS] = {
	-80.0, 20.0, "S V Power (dBm)",
	-80.0, 20.0, "S H Power (dBm)",
	-30.0, 30.0, "Velocity (m/s)",
	0.0, 1.0, "NCP",
	0.0, 30.0, "Spectral Width (m/s)",
	-180.0, 180.0, "phidp (degrees)",
	-100.0, 100.0, "V Reflectivity",
	-100.0, 100.0, "H Reflectivity",
	-10.0, 10.0, "Zdr"
};


CP2PPI::CP2PPI( int nVars,
				 QWidget* parent, 
				 const char* name, 
				 WFlags fl):
CP2PPIBase(parent,
			name,
			fl),
			_angle(0.0),
			_nVars(nVars),
			_angleInc(1.0),
			_gates(1000)
{

	//	Designer seems to have trouble doing this: 
    _dataSourceButtons->setProperty( "selectedId", SBAND_DATA_SOURCE );
	m_dataSource = SBAND_DATA_SOURCE;	//	set data source for PPI display to test  
	if	(m_dataSource == SBAND_DATA_SOURCE)	{	//	Default to S-band 
		_nVars = NUM_SBANDPRODUCTS;
		setupTestProducts(SBandProductDisplayConfiguration);	//	set up test product buttons now 
	}
	//	else use received data to get radar identification and set up products at that time 
	m_receiving = 0;	
	m_dataGramPort	= 21014; //#define -- wired to data from 2nd piraq in processor, corresponding ABP port from QtDSP
	m_packetCount	= 0; 
	_parametersSet = 0;		//	piraqx parameters not yet initialized from received data
	//	allocate S-band APB packet storage
	SABP = new CP2_PIRAQ_DATA_TYPE[16384]; 
	//	create the socket that receives packets from QtDSP  
	initializeSocket();	
	connectDataRcv();
}


///////////////////////////////////////////////////////////////////////

CP2PPI::~CP2PPI() 
{
	delete _colorBar;
	for (int i = 0; i < _maps.size(); i++)
		delete _maps[i];
}
///////////////////////////////////////////////////////////////////////

void
CP2PPI::startStopDisplaySlot()	//	start/stop display
{
	if	(m_receiving)	{		//	currently receiving data
//		for (int i = 0; i < QTDSP_RECEIVE_CHANNELS; i++) {	//	all receive channels
//		    terminateReceiveSocket(&rcvChannel[i]);
//		}
		m_receiving = !m_receiving; 
		_timer.stop();	//	?test data only?
//		statusLog->append( "Not Receiving" ); 
		return;
	}
	//	toggle start/stop
	m_receiving = !m_receiving;
	_timer.start(50);	//	test data only

}

///////////////////////////////////////////////////////////////////////

void 
CP2PPI::dataSourceSlot()	{
	switch (_dataSourceButtons->selectedId()) {
		case 0:
			m_dataSource = XBAND_DATA_SOURCE;
			break;
		case 1:
			m_dataSource = SBAND_DATA_SOURCE;
			//setupSBandProductsDisplay();
			break;
		default: {}
	}
//could need this or equivalent:	resizeDataVectors();	//	resize data vectors
}

///////////////////////////////////////////////////////////////////////

void CP2PPI::zoomInSlot()
{
	_ppi->setZoom(2.0);
	ZoomFactor->display(_ppi->getZoom());
}

///////////////////////////////////////////////////////////////////////

void CP2PPI::zoomOutSlot()
{
	_ppi->setZoom(0.5);
	ZoomFactor->display(_ppi->getZoom());
}

///////////////////////////////////////////////////////////////////////

#if 1	//	implementation for dynamically-allocated beams: real radar data 
void
CP2PPI::addBeam() 
{

	int rep = 1;
	double angle;
	CP2_PIRAQ_DATA_TYPE * SVHData;	//	 pointer to working datum in SVH ABP data 
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
	double SVZ, SHZ;	//	S-band V, H reflectivity for Zdr calculation 
	CP2_PIRAQ_DATA_TYPE * SVHDataH, * SVHDataV;	//	data pointer for Zdr 

	angle = _angle;
	for (int t = 0; t < rep; t++) { 
		double startAngle;
		double stopAngle;

		if (angle > 360.0)
			angle -= 360.0;

		if (_angleInc > 0.0) {
			startAngle = angle-0.05;
			stopAngle = angle + _angleInc+0.05;
		} else {
			startAngle = angle + _angleInc-0.05;
			stopAngle = angle+0.05;
		}

		if (startAngle < 0.0)
			startAngle += 360.0;
		if (startAngle > 360.0)
			startAngle -= 360.0;
		if (stopAngle < 0.0)
			stopAngle += 360.0;
		if (stopAngle > 360.0)
			stopAngle -= 360.0;
		//	compute quantities used for product calculations
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
		angle_to_velocity_scale_factor = C / (2.0 * _frequency * 2.0 * M_PI * _prt);
		ph_off = _noise_phase_offset * M_PI / 180.0;	//	SPol sets this phase offset to 20 deg
		widthconst = (C / _frequency) / _prt / (4.0 * sqrt(2.0) * M_PI); 

		for (int p = 0; p < _nVars; p++) {	//	all products (presently S-band)
			//	point to begin of data: gate 0 
			SVHDataH = SABP + _IQdataOffset/sizeof(float);	
			SVHDataV = SABP + _IQdataOffset/sizeof(float);
			SVHData = SABP + _IQdataOffset/sizeof(float);
			_beamData[p].resize(_gates);
				//	initialize ABP data pointers for current product to calculate
				switch (p)	{	//	p = S-band product enum  
					case SVHVP:		//	S-band VH V Power in dBm 
						SVHData += 2;	break;	//	+2 offset to V pulse raw Power
					case SVHHP:		//	S-band VH H Power in dBm 
						SVHData += 5;	break;	//	+5 offset to H pulse raw Power
					case SVEL:		//	S-band velocity using V and H data
					case SNCP:		//	S-band NCP using calculation A2*A2+B2*B2/Pv+Ph
					case SWIDTH:	//	S-band spectral width 
					case SPHIDP:	//	compute S-band phidp 
										break;	//	no offset
					case VREFL:		//	S-band V reflectivity  
						SVHData += 2;	break;	//	+2 offset to V pulse raw Power
					case HREFL:		//	S-band V reflectivity  
						SVHData += 5;	break;	//	+5 offset to H pulse raw Power
					case ZDR:		//	S-band Zdr 
						SVHDataV += 2;			//	+2 offset to V pulse raw Power
						SVHDataH += 5;	break;	//	+5 offset to H pulse raw Power
					default:			break;	//	enough already
				}	//	end switch () to adjust data pointer initial value
			for (int g = 0; g < _gates; g++) {	//	all gates
				switch (p)	{	//	p = S-band product enum
					case SVHVP:		//	compute S-band VH V Power in dBm 
						SVP_ = *SVHData;
						*SVHData -= VrawNoisePower;	//	correct the raw power 
						if	(*SVHData <= 0.0)		//	corrected to negative value
							*SVHData = pow(10.0,((NOISE_FLOOR - VOffsetTo_dBm) / 10.0)); // put in something small: gets NOISE_LIMIT dBm
						else	{
							_beamData[p][g] = (double)(10.0*log10(*SVHData) + VOffsetTo_dBm);	//	scale result to dBm 
						}
						break;
					case SVHHP:	//	compute S-band VH H Power in dBm 
						//SVP_ = *SVHData;
						*SVHData -= HrawNoisePower;	//	correct the raw power 
						if	(*SVHData <= 0.0)		//	corrected to negative value
							*SVHData = pow(10.0,((NOISE_FLOOR - HOffsetTo_dBm) / 10.0)); // put in something small: gets NOISE_LIMIT dBm
						else	{
							_beamData[p][g] = (double)(10.0*log10(*SVHData) + HOffsetTo_dBm);	//	scale result to dBm 
						}
						break;
					case SVEL:	//	compute S-band velocity using V and H data
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
						_beamData[p][g] = v * angle_to_velocity_scale_factor; 
						break;
					case SNCP:		//	compute S-band NCP using calculation A2*A2+B2*B2/Pv+Ph
						A2 = *(SVHData + 6); B2 = *(SVHData + 7); Pv = *(SVHData + 2); Ph = *(SVHData + 5);	//	
						_beamData[p][g] = sqrt(A2*A2 + B2*B2) / (Pv + Ph);
						break;
					case SWIDTH:	//	compute S-band spectral width 
						A2 = *(SVHData + 6); B2 = *(SVHData + 7); Pv = *(SVHData + 2); Ph = *(SVHData + 5);	//	
						NCP = sqrt(A2*A2 + B2*B2) / (Pv + Ph);	//	calculate NCP
						if	(log(NCP) > 0.0)
							NCP = 1.0; 
						_beamData[p][g] = widthconst * sqrt(-log(NCP));
						break;
					case SPHIDP:	//	compute S-band phidp 
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
						_beamData[p][g] = dp * 180.0 / M_PI;
						break;			
					case VREFL:	//	compute S-band V reflectivity  
						SVHData = SABP + _IQdataOffset/sizeof(float) + 2;	//	+2 offset to V pulse raw Power
						if	(g)	//	range not zero 
							range_correction = 20.0 * log10(g * 0.0005 * C * _rcvr_pulsewidth);
						//	calculate S-band V Power, then reflectivity: 
						*SVHData -= VrawNoisePower;	//	correct the raw power 
						if	(*SVHData <= 0.0)		//	corrected to negative value
							*SVHData = pow(10.0,((NOISE_FLOOR - VOffsetTo_dBm) / 10.0)); // put in something small: gets NOISE_LIMIT dBm
						_beamData[p][g] = (double)(10.0*log10(*SVHData) + VOffsetTo_dBm) + v_channel_radar_constant + range_correction;	//	 
						break;			
					case HREFL:	//	compute S-band H reflectivity  
						if	(g)	//	range not zero 
							range_correction = 20.0 * log10(g * 0.0005 * C * _rcvr_pulsewidth);
						//	calculate S-band V Power, then reflectivity: 
						*SVHData -= HrawNoisePower;	//	correct the raw power 
						if	(*SVHData <= 0.0)		//	corrected to negative value
							*SVHData = pow(10.0,((NOISE_FLOOR - HOffsetTo_dBm) / 10.0)); // put in something small: gets NOISE_LIMIT dBm
						_beamData[p][g] = (double)(10.0*log10(*SVHData) + HOffsetTo_dBm) + h_channel_radar_constant + range_correction;	//	 
						break;			
					case ZDR:
						if	(g)	//	range not zero 
							range_correction = 20.0 * log10(g * 0.0005 * C * _rcvr_pulsewidth);
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
						_beamData[p][g] = SHZ - SVZ + _zdr_fudge_factor + _zdr_bias;
						SVHDataH += SVHABP_STRIDE; SVHDataV += SVHABP_STRIDE;	//	index next gate
						break;
				}	//	end S-band product switch
				SVHData += SVHABP_STRIDE;	//	index next gate	in ABP data 
			}	//	end for (all gates)
		}	//	end for (all products)

		_ppi->addBeam(startAngle, stopAngle, _gates, _beamData, 1, _maps);

		angle += _angleInc;

//		LCDNumberCurrentAngle->display(angle);
	}

	_angle += rep*_angleInc;
	if (_angle > 360.0)
		_angle -= 360.0;

	if (_angle < 0.0)
		_angle += 360.0;
}

#else	//	initial test implementation using preallocated beams 
void
CP2PPI::addBeam() 
{

	int rep = 1;
	double angle;

	angle = _angle;
	for (int t = 0; t < rep; t++) { 
		double startAngle;
		double stopAngle;

		if (angle > 360.0)
			angle -= 360.0;

		if (_angleInc > 0.0) {
			startAngle = angle-0.05;
			stopAngle = angle + _angleInc+0.05;
		} else {
			startAngle = angle + _angleInc-0.05;
			stopAngle = angle+0.05;
		}

		if (startAngle < 0.0)
			startAngle += 360.0;
		if (startAngle > 360.0)
			startAngle -= 360.0;
		if (stopAngle < 0.0)
			stopAngle += 360.0;
		if (stopAngle > 360.0)
			stopAngle -= 360.0;

		for (int v = 0; v < _nVars; v++) {
			_beamData[v].resize(_gates);
			for (int g = 0; g < _gates; g++) {
				_beamData[v][g] = 100.0*((double)rand())/RAND_MAX;
			}
		}

		_ppi->addBeam(startAngle, stopAngle, _gates, _beamData, 1, _maps);

		angle += _angleInc;

//		LCDNumberCurrentAngle->display(angle);
	}

	_angle += rep*_angleInc;
	if (_angle > 360.0)
		_angle -= 360.0;

	if (_angle < 0.0)
		_angle += 360.0;
}
#endif
////////////////////////////////////////////////////////////////////////

void CP2PPI::productSelectSlot(int index)
{
	_ppi->selectVar(index); 
	_colorBar->configure(*_maps[index]);
}

///////////////////////////////////////////////////////////////////////
#if 0	//	introduce these as supported

void CP2PPI::changeDir() 
{
	_angleInc = -1.0*_angleInc;
}

///////////////////////////////////////////////////////////////////////

void CP2PPI::clearVarSlot(int index)
{
	_ppi->clearVar(index);
}

///////////////////////////////////////////////////////////////////////

void CP2PPI::panSlot(int panIndex)
{
	switch (panIndex) {
	case 0:
		_ppi->setZoom(0.0);
		break;
	case 1:
		_ppi->pan(0.0, 0.10);
		break;
	case 2:
		_ppi->pan(-0.10, 0.10);
		break;
	case 3:
		_ppi->pan(-0.10, 0.0);
		break;
	case 4:
		_ppi->pan(-0.10, -0.10);
		break;
	case 5:
		_ppi->pan(00, -0.10);
		break;
	case 6:
		_ppi->pan(0.10, -0.10);
		break;
	case 7:
		_ppi->pan(0.10, 0.0);
		break;
	case 8:
		_ppi->pan(0.10, 0.10);
		break;
	default:
		break;
	}
}
#endif	//	end base-class support conditional 

#if 1	//	configure color bar, buttons from source-radar configuration
void
CP2PPI::setupTestProducts( productDisplayConfiguration * )	//	!use this!
{
	float	colorBarMin, colorBarMax;
	char *	buttonLabel; 

	// create the color maps
	for (int i = 0; i < _nVars; i++) {
		colorBarMin = SBandProductDisplayConfiguration[i].rangeMin;
		colorBarMax = SBandProductDisplayConfiguration[i].rangeMax;
		_maps.push_back(new ColorMap(colorBarMin, colorBarMax));
	}
	QVBoxLayout* colorBarLayout = new QVBoxLayout(frameColorBar);
	_colorBar = new ColorBar(frameColorBar);
	_colorBar->configure(*_maps[0]);
	colorBarLayout->addWidget(_colorBar);

	////////////////////////////////////////////////////////////////////////
	//
	// Create radio buttons for selecting the variable to display
	//

	// configure policy for the button group
	_productButtons->setColumnLayout(0, Qt::Vertical );
	_productButtons->layout()->setSpacing( 6 );

	// create a layout manager for the button group
	QVBoxLayout* btnGroupVarSelectLayout1 = new QVBoxLayout( _productButtons->layout());
	btnGroupVarSelectLayout1->setAlignment( Qt::AlignTop );

	// create the buttons
	for (int r = 0; r < _nVars; r++) {
		QRadioButton* rb1 = new QRadioButton(_productButtons);
		// add button to the group layout
		btnGroupVarSelectLayout1->addWidget(rb1);
		QString s = QString("%1").arg(SBandProductDisplayConfiguration[r].label);
		rb1->setText(s);
		if (r == 0){
			rb1->setChecked(true);
		}
	}

	// Note that the following call determines whether PPI will 
	// use preallocated or dynamically allocated beams. If a third
	// parameter is specified, it will set the number of preallocated
	// beams.
	_ppi->configure(_nVars, _gates);	//	dynamically allocated beams

	// connect the button pressed signal to the var changed slot
	connect( _productButtons, SIGNAL( clicked(int) ), this, SLOT( productSelectSlot(int) ));

	////////////////////////////////////////////////////////////////////////
	//
	//  
	//

	_beamData.resize(_nVars);
}
#else	//	test data, preallocated; initial method 
void
CP2PPI::setupTestProducts( void )	//	
{
	// create the color maps
	for (int i = 0; i < _nVars; i++) {
		_maps.push_back(new ColorMap(0.0, 100.0/(i+1)));
	}
	QVBoxLayout* colorBarLayout = new QVBoxLayout(frameColorBar);
	_colorBar = new ColorBar(frameColorBar);
	_colorBar->configure(*_maps[0]);
	colorBarLayout->addWidget(_colorBar);

	////////////////////////////////////////////////////////////////////////
	//
	// Create radio buttons for selecting the variable to display
	//

	// configure policy for the button group
	_productButtons->setColumnLayout(0, Qt::Vertical );
	_productButtons->layout()->setSpacing( 6 );

	// create a layout manager for the button group
	QVBoxLayout* btnGroupVarSelectLayout1 = new QVBoxLayout( _productButtons->layout());
	btnGroupVarSelectLayout1->setAlignment( Qt::AlignTop );

	// create the buttons
	for (int r = 0; r < _nVars; r++) {
		QRadioButton* rb1 = new QRadioButton(_productButtons);
		// add button to the group layout
		btnGroupVarSelectLayout1->addWidget(rb1);
		QString s = QString("%1").arg(r);
		rb1->setText(s);
		if (r == 0){
			rb1->setChecked(true);
		}
	}

	// Note that the following call determines whether PPI will 
	// use preallocated or dynamically allocated beams. If a third
	// parameter is specified, it will set the number of preallocated
	// beams.
	_ppi->configure(_nVars, _gates, 360);

	// connect the button pressed signal to the var changed slot
	connect( _productButtons, SIGNAL( clicked(int) ), this, SLOT( productSelectSlot(int) ));

	////////////////////////////////////////////////////////////////////////
	//
	//  
	//
	connect(&_timer, SIGNAL(timeout()), this, SLOT(addBeam()));

	_beamData.resize(_nVars);
}
#endif


void
CP2PPI::connectDataRcv()	// connect notifier to data-receive slot 
{
	connect(m_pDataSocketNotifier, SIGNAL(activated(int)), this, SLOT(dataSocketActivatedSlot(int)));
}


// get program operating parameters from piraqx (or other) "housekeeping" header structure; resize arrays, etc. 
int
CP2PPI::getParameters( CP2_PIRAQ_DATA_TYPE[] )	
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
	_IQdataOffset = sizeof(PACKETHEADER);	//	offset in bytes to begin of data, IQ or ABP, in (piraqx) packet
	//	compute #hits combined by piraq: equal in both piraq executable (CP2_DCCS3_1.out) and host applications
	//	8 bytes per gate: define per SVH APB data type: 
	_Nhits = UDPSENDSIZEMAX / (HEADERSIZE + (8*_gates));	
	if	(_Nhits % 2)	//	odd #hits
		_Nhits--;		//	make it even: PIRAQ sends even #hits per packet

	_parametersSet = 1;	//	condition this	
	return(_parametersSet);	 
}

//	
void 
CP2PPI::dataSocketActivatedSlot(int socket)
{
	static int errcount = 0;
	static int lasterrcount = 0;
	static int readBufLen, lastreadBufLen;
	int	r_c;	//	return_code

	m_packetCount++; 
	readBufLen = m_pDataSocket->readBlock((char *)m_pSocketBuf, sizeof(short)*1000000);
	if	(readBufLen != lastreadBufLen)	{	//	packet size changed: hmmmmm .... could be all right if CP2exec stop/start so generalize to cover this. 
		errcount++;  
	}
	if	(!_parametersSet)	{	//	piraqx parameters need to be initialized from received data
		r_c = getParameters( m_pSocketBuf );	//	 
		if	(r_c == 1)	{		//	success
		}
	}
	if (!(m_packetCount % 50))	{
		if(readBufLen > 0) {
//implement:			_packetCount->setNum(m_packetCount);	// update cumulative packet count 
		}
	}	
	lastreadBufLen = readBufLen;
	if	(_timer.isActive())	{	//	display activated 
		memmove((void *)SABP,(const void *)m_pSocketBuf,readBufLen);	//	copy received ABP packet 
		addBeam();
	}
}

void
CP2PPI::initializeSocket()	{	//	?pass port#
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
//	display: 
//	m_pTextIPname->setText(myIPname.c_str());

//	m_pTextIPaddress->setNum(+m_dataGramPort);	// diagnostic print
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
CP2PPI::terminateSocket( void )	{	//	?pass port#
	if (m_pDataSocketNotifier)
		delete m_pDataSocketNotifier;

	if (m_pDataSocket)
		delete m_pDataSocket;

	if (m_pSocketBuf)
		delete [] m_pSocketBuf;
}




