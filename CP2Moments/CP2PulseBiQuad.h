#ifndef CP2PULSEBIQUADINC_
#define CP2PULSEBIQUADINC_

#include "BiQuad.h"
#include "CP2Net.h"
using namespace CP2Net;
#include <vector>

/**
@brief A biquad IIR filter which will operate on CP2Pulses.

CP2Pulses are passed in, sent to the biqaud filter, and the
filtered data replace the incoming data. CP2PulseBiQuad is
running 2*ngates filters; i.e. one filter for each i and q
at each gate.
**/
class CP2PulseBiQuad: BiQuad {
public:
	CP2PulseBiQuad(int gates,///< The number of gates that the filter will operate on
		float a1,           ///< coeffcient a1
		float a2,           ///< coeffcient a2
		float b0,           ///< coeffcient b0
		float b1,           ///< coeffcient b1
		float b2            ///< coeffcient b2
		);
	virtual ~CP2PulseBiQuad();
	void tick(CP2Pulse& pulse ///< A pulse to be processed
		);

protected:

	float _a1;           ///< coeffcient a1
	float _a2;           ///< coeffcient a2
	float _b0;           ///< coeffcient b0
	float _b1;           ///< coeffcient b1
	float _b2;           ///< coeffcient b2

	/// One filter will be created for each i and q at each gate.
	std::vector<BiQuad*> _filters;
};

#endif
