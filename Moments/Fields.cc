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
const int Fields::missingInt = -9999;

// constructor

Fields::Fields()

{

  initialize();

}

// Initialize to missing

void Fields::initialize()

{
  
  flags = missingInt;

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
//  kdp = missingDouble;

  snrhc = missingDouble;
  snrhx = missingDouble;
  snrvc = missingDouble;
  snrvx = missingDouble;

  dbmhc = missingDouble;
  dbmhx = missingDouble;
  dbmvc = missingDouble;
  dbmvx = missingDouble;

  dbzhc = missingDouble;
  dbzhx = missingDouble;
  dbzvc = missingDouble;
  dbzvx = missingDouble;

}

