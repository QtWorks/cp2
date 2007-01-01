#include "PpiInfo.h"

////////////////////////////////////////////////////////
PpiInfo::PpiInfo():
_id(0),
_shortName(""),
_longName(""),
_scaleMin(1),
_scaleMax(10)
{
}

////////////////////////////////////////////////////////
PpiInfo::PpiInfo(int id, std::string shortName, std::string longName,
		double scaleMin, double scaleMax,
		int ppiIndex):
_id(id),
_shortName(shortName),
_longName(longName),
_scaleMin(scaleMin),
_scaleMax(scaleMax),
_ppiIndex(ppiIndex)
{
}

////////////////////////////////////////////////////////

PpiInfo::~PpiInfo()
{
}

////////////////////////////////////////////////////////
void
PpiInfo::setScale(double min, double max)
{
	_scaleMin = min;
	_scaleMax = max;
}

////////////////////////////////////////////////////////
double 
PpiInfo::getScaleMin()
{
	return _scaleMin;
}

////////////////////////////////////////////////////////
double 
PpiInfo::getScaleMax()
{
	return _scaleMax;
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

////////////////////////////////////////////////////////
int 
PpiInfo::getPpiIndex()
{
	return _ppiIndex;
}





