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

////////////////////////
// This class

class Fields {
  
public:
  
  Fields();

  // set values to missing

  void initialize();

  // public data

  int flags;  // censoring flag

  double snr;
  double dbm; // uncalibrated power
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

  double dbmhc; // uncalibrated power
  double dbmhx;
  double dbmvc;
  double dbmvx;

  double dbzhc;
  double dbzhx;
  double dbzvc;
  double dbzvx;

  // SZ8-64 phase coding

  int sz_trip_flag;
  double sz_leakage;
  double sz_dbzt;

  // infilling after applying SZ

  int sz_zinfill;
  double sz_itexture;
  double sz_dbzi;   // infilled dbz
  double sz_veli;   // infilled velocity
  double sz_widthi; // infilled width

  // CMD - Clutter Mitigation Decision

  double cmd;
  int cmd_flag;
  
  double cmd_dbz_diff_sq;
  double cmd_spin_change;
  
  double cmd_tdbz;
  double cmd_sqrt_tdbz;
  double cmd_spin;
  double cmd_vel;
  double cmd_vel_sdev;
  double cmd_width;

  double cmd_dbz_narrow;
  double cmd_ratio_narrow;
  double cmd_ratio_wide;
  double cmd_wx2peak_sep;
  double cmd_cvar_db;

  double cmd_zdr_sdev;
  double cmd_rhohv_sdev;
  double cmd_phidp_sdev;

  // refractivity fields

  double cpa;  // clutter phase alignment
  double aiq;
  double niq;
  double meani;
  double meanq;

  // for testing

  double test;
  
  static const double missingDouble;
  static const int missingInt;

protected:
private:

};

#endif

