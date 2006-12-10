###########################################################################
#
# Makefile for TsArchive2Dsr program
#
# Mike Dixon, RAP, NCAR
# POBox 3000, Boulder, CO, 80307, USA
#
# May 2006
#
###########################################################################

include $(RAP_MAKE_INC_DIR)/rap_make_macros

TARGET_FILE = TsArchive2Dsr

LOC_INCLUDES =
LOC_CPPC_CFLAGS = -ansi
LOC_LDFLAGS =
LOC_LIBS = -lSpdb -lFmq -ldsserver -ldidss -lrapformats -lrapmath -ltoolsa \
	-ldataport -ltdrp -lfftw3

HDRS = \
	$(PARAMS_HH) \
	Args.hh \
	Beam.hh \
	ClutFilter.hh \
	ClutProb.hh \
	Fft.hh \
	Fields.hh \
	GateSpectra.hh \
	InterestMap.hh \
	Moments.hh \
	MomentsMgr.hh \
	OpsInfo.hh \
	OutputFmq.hh \
	Pulse.hh \
	TsArchive2Dsr.hh

CPPC_SRCS = \
	$(PARAMS_CC) \
	Args.cc \
	Beam.cc \
	ClutFilter.cc \
	ClutProb.cc \
	Fft.cc \
	Fields.cc \
	GateSpectra.cc \
	InterestMap.cc \
	Moments.cc \
	MomentsMgr.cc \
	Main.cc \
	OpsInfo.cc \
	OutputFmq.cc \
	Pulse.cc \
	TsArchive2Dsr.cc

#
# tdrp macros
#

include $(RAP_MAKE_INC_DIR)/rap_make_tdrp_macros

#
# standard C++ targets
#

include $(RAP_MAKE_INC_DIR)/rap_make_c++_targets

#
# tdrp targets
#

include $(RAP_MAKE_INC_DIR)/rap_make_tdrp_c++_targets

#
# local targets
#

# DO NOT DELETE THIS LINE -- make depend depends on it.

