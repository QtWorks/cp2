// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=* 
// ** Copyright UCAR (c) 2006 
// ** University Corporation for Atmospheric Research(UCAR) 
// ** National Center for Atmospheric Research(NCAR) 
// ** Research Applications Laboratory(RAL) 
// ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA 
// ** 2006/9/5 14:33:50 
// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=* 
/////////////////////////////////////////////////////////////
// Fields.cc
//
// Mike Dixon, RAP, NCAR
// P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// Sept 2006
//
///////////////////////////////////////////////////////////////

#include "Fields.hh"
#include <cmath>

const double Fields::missingDouble = -9999;

// constructor

Fields::Fields()

{

  initialize();

}

// Initialize to missing

void Fields::initialize()

{
  
  snr = missingDouble;
  dbm = missingDouble;
  dbz = missingDouble;
  vel = missingDouble;
  width = missingDouble;

  // clutter-filtered fields

  clut = missingDouble;
  dbzf = missingDouble;
  velf = missingDouble;
  widthf = missingDouble;

  // dual polarization fields

  zdr = missingDouble;
  zdrm = missingDouble;
  
  ldrh = missingDouble;
  ldrv = missingDouble;

  rhohv = missingDouble;
  phidp = missingDouble;
  kdp = missingDouble;

  snrhc = missingDouble;
  snrhx = missingDouble;
  snrvc = missingDouble;
  snrvx = missingDouble;

  dbmhc = missingDouble;
  dbmhx = missingDouble;
  dbmvc = missingDouble;
  dbmvx = missingDouble;

}

// Print

void Fields::print(ostream &out) const

{
  
  out << "snr: " << snr << endl;
  out << "dbm: " << dbm << endl;
  out << "dbz: " << dbz << endl;
  out << "vel: " << vel << endl;
  out << "width: " << width << endl;
  out << "clut: " << clut << endl;
  out << "dbzf: " << dbzf << endl;
  out << "velf: " << velf << endl;
  out << "widthf: " << widthf << endl;
  out << "zdr: " << zdr << endl;
  out << "zdrm: " << zdrm << endl;
  out << "ldrh: " << ldrh << endl;
  out << "ldrv: " << ldrv << endl;
  out << "rhohv: " << rhohv << endl;
  out << "phidp: " << phidp << endl;
  out << "kdp: " << kdp << endl;
  out << "snrhc: " << snrhc << endl;
  out << "snrhx: " << snrhx << endl;
  out << "snrvc: " << snrvc << endl;
  out << "snrvx: " << snrvx << endl;
  out << "dbmhc: " << dbmhc << endl;
  out << "dbmhx: " << dbmhx << endl;
  out << "dbmvc: " << dbmvc << endl;
  out << "dbmvx: " << dbmvx << endl;

}

