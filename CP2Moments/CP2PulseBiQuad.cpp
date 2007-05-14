#include "CP2PulseBiQuad.h"

CP2PulseBiQuad::CP2PulseBiQuad(
							   int gates,           
							   float a1,           
							   float a2,           
							   float b0,           
							   float b1,           
							   float b2            
							   ):
_a1(a1),
_a2(a2),
_b0(b0),
_b1(b1),
_b2(b2)
{
	for (unsigned int i = 0; i < (unsigned int)2*gates; i++) {
		_filters.push_back(new BiQuad);
		_filters[i]->setA1(_a1);
		_filters[i]->setA2(_a2);
		_filters[i]->setB0(_b0);
		_filters[i]->setB1(_b1);
		_filters[i]->setB2(_b2);
	}
}

/////////////////////////////////////////////////////////////////////

CP2PulseBiQuad::~CP2PulseBiQuad()
{
	for (unsigned int i = 0; i < _filters.size(); i++) {
		delete _filters[i];
	}
}

/////////////////////////////////////////////////////////////////////

void CP2PulseBiQuad::tick(CP2Pulse& pulse)
{
	for (int i = 0; i < pulse.header.gates*2; i++) {
		pulse.data[(unsigned int)i] = _filters[i]->tick(pulse.data[(unsigned int)i]);
	}
}
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////

