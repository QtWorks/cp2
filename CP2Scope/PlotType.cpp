#include "PlotType.h"

////////////////////////////////////////////////////////
PlotType::PlotType(int id, 
		double gainMin, double gainMax, double gainCurrent, 
		double offsetMin, double offsetMax, double offsetCurrent):
_id(id),
_gainMin(gainMin),
_gainMax(gainMax),
_gainCurrent(gainCurrent),
_offsetMin(offsetMin),
_offsetMax(offsetMax),
_offsetCurrent(offsetCurrent)
{
}

////////////////////////////////////////////////////////

PlotType::~PlotType()
{
}

////////////////////////////////////////////////////////
void
PlotType::setGain(double min, double max, double current)
{
	_gainMin = min;
	_gainMax = max;
	_gainCurrent = current;
}

////////////////////////////////////////////////////////
void
PlotType::setOffset(double min, double max, double current)
{
	_offsetMin = min;
	_offsetMax = max;
	_offsetCurrent = current;
}

////////////////////////////////////////////////////////
double 
PlotType::getGainMin()
{
	return _gainMin;
}

////////////////////////////////////////////////////////
double 
PlotType::getGainMax()
{
	return _gainMax;
}

////////////////////////////////////////////////////////
double 
PlotType::getGainCurrent()
{
	return _gainCurrent;
}

////////////////////////////////////////////////////////
double 
PlotType::getOffsetMin()
{
	return _offsetMin;
}

////////////////////////////////////////////////////////
double 
PlotType::getOffsetMax()
{
	return _offsetMax;
}

////////////////////////////////////////////////////////
double 
PlotType::getOffsetCurrent()
{
	return _offsetCurrent;
}
