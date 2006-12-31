#include "PpiInfo.h"

////////////////////////////////////////////////////////
PpiInfo::PpiInfo():
_id(0),
_shortName(""),
_longName(""),
_gainMin(-1),
_gainMax(1),
_gainCurrent(0),
_offsetMin(-1),
_offsetMax(1),
_offsetCurrent(0)
{
}
////////////////////////////////////////////////////////
PpiInfo::PpiInfo(int id, std::string shortName, std::string longName,
		double gainMin, double gainMax, double gainCurrent, 
		double offsetMin, double offsetMax, double offsetCurrent):
_id(id),
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

PpiInfo::~PpiInfo()
{
}

////////////////////////////////////////////////////////
void
PpiInfo::setGain(double min, double max, double current)
{
	_gainMin = min;
	_gainMax = max;
	_gainCurrent = current;
}

////////////////////////////////////////////////////////
void
PpiInfo::setOffset(double min, double max, double current)
{
	_offsetMin = min;
	_offsetMax = max;
	_offsetCurrent = current;
}

////////////////////////////////////////////////////////
double 
PpiInfo::getGainMin()
{
	return _gainMin;
}

////////////////////////////////////////////////////////
double 
PpiInfo::getGainMax()
{
	return _gainMax;
}

////////////////////////////////////////////////////////
double 
PpiInfo::getGainCurrent()
{
	return _gainCurrent;
}

////////////////////////////////////////////////////////
double 
PpiInfo::getOffsetMin()
{
	return _offsetMin;
}

////////////////////////////////////////////////////////
double 
PpiInfo::getOffsetMax()
{
	return _offsetMax;
}

////////////////////////////////////////////////////////
double 
PpiInfo::getOffsetCurrent()
{
	return _offsetCurrent;
}

////////////////////////////////////////////////////////
std::string 
PpiInfo::getShortName()
{
	return _shortName;
}

////////////////////////////////////////////////////////
std::string 
PpiInfo::getLongName()
{
	return _longName;
}
////////////////////////////////////////////////////////
int 
PpiInfo::getId()
{
	return _id;
}





