TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# NOTE: When building on Windows use windows-x64-static.
# That way the DLL won't depend on Com.dll, though it will still depend on fftw3f-3.dll.

SHARED_LIBRARIES = YES
STATIC_BUILD = YES
LOADABLE_LIBRARY_HOST += tomoRecon
tomoRecon_LDFLAGS_Linux += $(STATIC_LDFLAGS)

USR_CXXFLAGS_Linux += -std=c++11

INC += fftw3.h

# Note, the following files were manually copied from the fftw Windows distribution directories
ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
LIB_INSTALLS    += ../os/win32-x86/libfftw3f-3.lib
BIN_INSTALLS    += ../os/win32-x86/libfftw3f-3.dll

else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
LIB_INSTALLS    += ../os/windows-x64/libfftw3f-3.lib
BIN_INSTALLS    += ../os/windows-x64/libfftw3f-3.dll

else ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
#LIB_INSTALLS    += ../os/linux-x86_64/libfftw3f.a

else ifeq (linux-x86, $(findstring linux-x86, $(T_A)))
LIB_INSTALLS    += ../os/linux-x86/libfftw3f.a

endif

tomoRecon_SRCS += tomoPreprocess.cpp
tomoRecon_SRCS += tomoPreprocessIDL.cpp
tomoRecon_SRCS += tomoRecon.cpp
tomoRecon_SRCS += grid.cpp pswf.c filters.c
tomoRecon_SRCS += tomoReconIDL.cpp fftwIDL.cpp

tomoRecon_LIBS_WIN32 += libfftw3f-3
tomoRecon_SYS_LIBS_WIN32 += dbghelp
# To use the version of fftw3f included with tomoRecon use this line
#tomoRecon_LIBS_Linux += fftw3f
# To use the system version of fftw3f use this line
tomoRecon_SYS_LIBS_Linux += fftw3f
tomoRecon_SYS_LIBS_Darwin += fftw3f
tomoRecon_LIBS += Com

#=============================


#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
