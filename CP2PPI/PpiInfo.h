#ifndef PLOTINFOINC_
#define PLOTINFOINC_

#include <string>

class PpiInfo {
public:
	PpiInfo();
	PpiInfo(int id, std::string shortName, std::string longName,
		double scaleMin, double scaleMax, int ppiIndex);
	virtual ~PpiInfo();

	int getId();
	int getPpiIndex();

	void setScale(double min, double max);

	double getScaleMin();
	double getScaleMax();
	std::string getShortName();
	std::string getLongName();

protected:
	int _id;
	std::string _shortName;
	std::string _longName;
	double _scaleMin;
	double _scaleMax;
	int _ppiIndex;
};
#endif
