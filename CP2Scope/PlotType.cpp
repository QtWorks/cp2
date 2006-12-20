#include "PlotInfo.h"

////////////////////////////////////////////////////////
PlotInfo::PlotInfo(int id, int displayType, std::string shortName, std::string longName,
		double gainMin, double gainMax, double gainCurrent, 
		double offsetMin, double offsetMax, double offsetCurrent):
_id(id),
_displayType(displayType),
_shortName(shortName),
_longName(longName),
_gainMin(gainMin),
_gainMax(gainMax),
_gainCurrent(gainCurrent),
_offsetMin(offsetMin),
_offsetMax(offsetMax),
_offsetCurrent(offsetCurrent)
{
}

////////////////////////////////////////////////////////

PlotInfo::~PlotInfo()
{
}

////////////////////////////////////////////////////////
void
PlotInfo::setGain(double min, double max, double current)
{
	_gainMin = min;
	_gainMax = max;
	_gainCurrent = current;
}

////////////////////////////////////////////////////////
void
PlotInfo::setOffset(double min, double max, double current)
{
	_offsetMin = min;
	_offsetMax = max;
	_offsetCurrent = current;
}

////////////////////////////////////////////////////////
double 
PlotInfo::getGainMin()
{
	return _gainMin;
}

////////////////////////////////////////////////////////
double 
PlotInfo::getGainMax()
{
	return _gainMax;
}

////////////////////////////////////////////////////////
double 
PlotInfo::getGainCurrent()
{
	return _gainCurrent;
}

////////////////////////////////////////////////////////
double 
PlotInfo::getOffsetMin()
{
	return _offsetMin;
}

////////////////////////////////////////////////////////
double 
PlotInfo::getOffsetMax()
{
	return _offsetMax;
}

////////////////////////////////////////////////////////
double 
PlotInfo::getOffsetCurrent()
{
	return _offsetCurrent;
}

////////////////////////////////////////////////////////
std::string 
PlotInfo::getShortName()
{
	return _shortName;
}

////////////////////////////////////////////////////////
std::string 
PlotInfo::getLongName()
{
	return _longName;
}


