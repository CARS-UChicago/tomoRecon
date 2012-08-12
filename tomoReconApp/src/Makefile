TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

SHARED_LIBRARIES = YES
STATIC_BUILD = YES
LIBRARY_HOST += tomoRecon

# Note, the following files were manually copied from the fftw Windows distribution directories
ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
LIB_INSTALLS    += ../os/win32-x86/libfftw3f-3.lib
BIN_INSTALLS    += ../os/win32-x86/libfftw3f-3.dll

else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
LIB_INSTALLS    += ../os/windows-x64/libfftw3f-3.lib
BIN_INSTALLS    += ../os/windows-x64/libfftw3f-3.dll
endif

tomoRecon_SRCS += tomoRecon.cpp
tomoRecon_SRCS += grid.cpp pswf.c filters.c
tomoRecon_SRCS += tomoReconIDL.cpp

tomoRecon_LIBS_WIN32 += libfftw3f-3
tomoRecon_SYS_LIBS_Linux += fftw3f
tomoRecon_SYS_LIBS_Darwin += fftw3f
tomoRecon_LIBS += Com

#=============================


#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
