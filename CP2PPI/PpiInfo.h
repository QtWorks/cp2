#ifndef PLOTINFOINC_
#define PLOTINFOINC_

#include <string>

class PpiInfo {
public:
	PpiInfo();
	PpiInfo(int id, std::string shortName, std::string longName,
		double gainMin, double gainMax, double gainCurrent, 
		double offsetMin, double offsetMax, double offsetCurrent,
		int ppiIndex);
	virtual ~PpiInfo();

	int getId();
	int getPpiIndex();

	void setGain(double min, double max, double current);
	void setOffset(double min, double Max, double current);

	double getGainMin();
	double getGainMax();
	double getGainCurrent();

	double getOffsetMin();
	double getOffsetMax();
	double getOffsetCurrent();

	std::string getShortName();
	std::string getLongName();

protected:
	int _id;
	std::string _shortName;
	std::string _longName;
	double _gainMin;
	double _gainMax;
	double _gainCurrent;
	double _offsetMin;
	double _offsetMax;
	double _offsetCurrent;
	int _ppiIndex;
};
#endif
