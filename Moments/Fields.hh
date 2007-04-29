// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=* 
// ** Copyright UCAR (c) 2006 
// ** University Corporation for Atmospheric Research(UCAR) 
// ** National Center for Atmospheric Research(NCAR) 
// ** Research Applications Laboratory(RAL) 
// ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA 
// ** 2006/9/5 14:33:50 
// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=* 
/////////////////////////////////////////////////////////////
// Fields.hh
//
// Mike Dixon, RAP, NCAR
// P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// Sept 2006
//
///////////////////////////////////////////////////////////////

#ifndef Fields_HH
#define Fields_HH

#include <iostream>
using namespace std;

////////////////////////
// This class

class Fields {
  
public:
  
  Fields();

  // set values to missing

  void initialize();

  // public data

  double snr;
  double dbm;
  double dbz;
  double vel;
  double width;

  // clutter-filtered fields

  double clut;
  double dbzf;
  double velf;
  double widthf;

  // dual polarization fields

  double zdr;  // calibrated
  double zdrm; // measured - uncalibrated
  
  double ldrh;
  double ldrv;

  double rhohv;
  double phidp;
  double kdp;

  double snrhc;
  double snrhx;
  double snrvc;
  double snrvx;

  double dbmhc;
  double dbmhx;
  double dbmvc;
  double dbmvx;

  static const double missingDouble;

  void print(ostream &out) const;

protected:
private:

};

#endif

