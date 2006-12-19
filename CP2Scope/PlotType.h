#ifndef PLOTTYPEINC_
#define PLOTTYPEINC_

class PlotType {
public:
	PlotType(int id, 
		double gainMin, double gainMax, double gainCurrent, 
		double offsetMin, double offsetMax, double offsetCurrent);
	virtual ~PlotType();

	void setGain(double min, double max, double current);
	void setOffset(double min, double Max, double current);

	double getGainMin();
	double getGainMax();
	double getGainCurrent();

	double getOffsetMin();
	double getOffsetMax();
	double getOffsetCurrent();

protected:
	int _id;
	double _gainMin;
	double _gainMax;
	double _gainCurrent;
	double _offsetMin;
	double _offsetMax;
	double _offsetCurrent;
};
#endif
