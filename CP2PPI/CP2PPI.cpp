#include "CP2PPI.h"
//#include "PPI.h"
#include <PPI/PPI.h>
#include <qlcdnumber.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qlayout.h>

#include <iostream>

CP2PPI::CP2PPI( int nVars,
				 QWidget* parent, 
				 const char* name, 
				 bool modal, 
				 WFlags fl):
CP2PPIBase(parent,
			name,
			modal,
			fl),
			_angle(0.0),
			_nVars(nVars),
			_angleInc(1.0),
			_gates(1000)
{

	// create the color maps
	for (int i = 0; i < _nVars; i++) {
		_maps1.push_back(new ColorMap(0.0, 100.0/(i+1)));
	}

	QVBoxLayout* colorBarLayout1 = new QVBoxLayout(frameColorBar1);
	_colorBar1 = new ColorBar(frameColorBar1);
	_colorBar1->configure(*_maps1[0]);
	colorBarLayout1->addWidget(_colorBar1);
	////////////////////////////////////////////////////////////////////////
	//
	// Create radio buttons for selecting the variable to display
	//

	// configure policy for the button group
	btnGroupVarSelect1->setColumnLayout(0, Qt::Vertical );
	btnGroupVarSelect1->layout()->setSpacing( 6 );

	// create a layout manager for the button group
	QVBoxLayout* btnGroupVarSelectLayout1 = new QVBoxLayout( btnGroupVarSelect1->layout());
	btnGroupVarSelectLayout1->setAlignment( Qt::AlignTop );

	// create the buttons
	for (int r = 0; r < _nVars; r++) {
		QRadioButton* rb1 = new QRadioButton(btnGroupVarSelect1);
		// add button to the group layou
		btnGroupVarSelectLayout1->addWidget(rb1);
		QString s = QString("%1").arg(r);
		rb1->setText(s);
		if (r == 0){
			rb1->setChecked(true);
		}
	}

	// Note that the following call determines whether PPI will 
	// use preallocated or dynamically allocated beams. If a third
	// parameter is specifiec, it will set the number of preallocated
	// beams.
	_ppi1->configure(_nVars, _gates, 360);

	// connect the button pressed signal to the var changed slot
	connect( btnGroupVarSelect1, SIGNAL( clicked(int) ), this, SLOT( varSelectSlot1(int) ));

	////////////////////////////////////////////////////////////////////////
	//
	//  
	//
	connect(&_timer, SIGNAL(timeout()), this, SLOT(addBeam()));

	_beamData.resize(_nVars);
}


///////////////////////////////////////////////////////////////////////

CP2PPI::~CP2PPI() 
{
	delete _colorBar1;
	for (int i = 0; i < _maps1.size(); i++)
		delete _maps1[i];
}
///////////////////////////////////////////////////////////////////////

#if 0	//	introduce these as base class supports
void
CP2PPI::startSlot() {

	_timer.start(50);

}

///////////////////////////////////////////////////////////////////////

void
CP2PPI::stopSlot() {
	_timer.stop();

}
///////////////////////////////////////////////////////////////////////

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

		_ppi1->addBeam(startAngle, stopAngle, _gates, _beamData, 1, _maps1);

		angle += _angleInc;

		LCDNumberCurrentAngle->display(angle);
//		LCDNumberBeamCount->display(_ppi2->numBeams());
//		LCDNumberZoomFactor->display(_ppi2->getZoom());
	}

	_angle += rep*_angleInc;
	if (_angle > 360.0)
		_angle -= 360.0;

	if (_angle < 0.0)
		_angle += 360.0;
}


///////////////////////////////////////////////////////////////////////

void CP2PPI::zoomInSlot()
{
	_ppi1->setZoom(2.0);
}

///////////////////////////////////////////////////////////////////////

void CP2PPI::zoomOutSlot()
{
	_ppi1->setZoom(0.5);
}

///////////////////////////////////////////////////////////////////////

void CP2PPI::varSelectSlot1(int index)
{
	_ppi1->selectVar(index); 
	_colorBar1->configure(*_maps1[index]);
}

///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////

void CP2PPI::changeDir() 
{
	_angleInc = -1.0*_angleInc;
}

///////////////////////////////////////////////////////////////////////

void CP2PPI::clearVarSlot(int index)
{
	_ppi1->clearVar(index);
}

///////////////////////////////////////////////////////////////////////

void CP2PPI::panSlot(int panIndex)
{
	switch (panIndex) {
	case 0:
		_ppi1->setZoom(0.0);
		break;
	case 1:
		_ppi1->pan(0.0, 0.10);
		break;
	case 2:
		_ppi1->pan(-0.10, 0.10);
		break;
	case 3:
		_ppi1->pan(-0.10, 0.0);
		break;
	case 4:
		_ppi1->pan(-0.10, -0.10);
		break;
	case 5:
		_ppi1->pan(00, -0.10);
		break;
	case 6:
		_ppi1->pan(0.10, -0.10);
		break;
	case 7:
		_ppi1->pan(0.10, 0.0);
		break;
	case 8:
		_ppi1->pan(0.10, 0.10);
		break;
	default:
		break;
	}
}
#endif	//	end base-class support conditional 
