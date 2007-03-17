#ifndef COLORBARSETTINGSINC_
#define COLORBARSETTINGSINC_

#include "ui_ColorBarSettings.h"

/// Use Ui::ColorBarSettings to present a dialog
/// for setting the color bar properties
class ColorBarSettings: public QDialog, private Ui::ColorBarSettings 
{
public:
	/// Constructor
	ColorBarSettings(double min, double max, QWidget* parent=0);
	/// Destructor
	virtual ~ColorBarSettings();
	/// @returns The minimum value
	double getMinimum();
	/// @returns
	double getMaximum();

protected:
};

#endif
