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
  kdp = missingDouble;

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

  // SZ8-64 phase coding

  sz_trip_flag = missingInt;
  sz_leakage = missingDouble;
  sz_dbzt = missingDouble;

  // infilling after applying SZ

  sz_zinfill = missingInt;
  sz_itexture = missingDouble;
  sz_dbzi = missingDouble;
  sz_veli = missingDouble;
  sz_widthi = missingDouble;

  // CMD - Clutter Mitigation Decision

  cmd = missingDouble;
  cmd_flag = 0;

  cmd_dbz_diff_sq = missingDouble;
  cmd_spin_change = missingDouble;

  cmd_tdbz = missingDouble;
  cmd_sqrt_tdbz = missingDouble;
  cmd_spin = missingDouble;
  cmd_vel = missingDouble;
  cmd_vel_sdev = missingDouble;
  cmd_width = missingDouble;

  cmd_dbz_narrow = missingDouble;
  cmd_ratio_narrow = missingDouble;
  cmd_ratio_wide = missingDouble;
  cmd_wx2peak_sep = missingDouble;
  cmd_cvar_db = missingDouble;

  cmd_zdr_sdev = missingDouble;
  cmd_rhohv_sdev = missingDouble;
  cmd_phidp_sdev = missingDouble;

  cpa = missingDouble;
  aiq = missingDouble;
  niq = missingDouble;
  meani = missingDouble;
  meanq = missingDouble;

  test = missingDouble;
  
}

