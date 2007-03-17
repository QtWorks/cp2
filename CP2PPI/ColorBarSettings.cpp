#include "ColorBarSettings.h"

//////////////////////////////////////////////////////////////////////////////
ColorBarSettings::ColorBarSettings(double min, double max, QWidget* parent):
QDialog(parent)
{
	setupUi(this);	

	// set the spin box min and max ranges to accept virtually everything
	_minSpin->setMinimum(-1.0e10);
	_minSpin->setMaximum( 1.0e10);
	_maxSpin->setMinimum(-1.0e10);
	_maxSpin->setMaximum( 1.0e10);

	// set the spin box values
	_minSpin->setValue(min);
	_maxSpin->setValue(max);
}

//////////////////////////////////////////////////////////////////////////////
ColorBarSettings::~ColorBarSettings()
{
}
//////////////////////////////////////////////////////////////////////////////
double
ColorBarSettings::getMinimum()
{
	return _minSpin->value();
}
//////////////////////////////////////////////////////////////////////////////
double
ColorBarSettings::getMaximum()
{
	return _maxSpin->value();
}
//////////////////////////////////////////////////////////////////////////////
