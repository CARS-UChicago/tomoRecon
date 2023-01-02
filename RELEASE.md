# tomoRecon Release Notes

## R2-0 (January XXX, 2023)
- Added tomoPreprocess.cpp which does tomography preprocessing in C++ code.
  This is about 10-15X faster than tomopy or IDL for these operations.
  The proprocessing does dark field, flat field, and zinger corrections.
- Added support for building with windows-x64-static architecture,
  so DLL does not depend on EPICS Com.dll, that code is included in tomoRecon.dll.
- Add logMsg function so debugging it written to a file.
  This is needed for IDL on Windows, where stdout is not available.
- Use the system version of fftw3f on Linux, not version in tomoRecon repository.
- Added code to make fftw directly callable from IDL.
- Fixed errors in FFT directions.
- Fixed major error in air normalization.
- Fixed minor error in ring artifact removal.

## R1-2 (March 11, 2013)
- Added libfftw3f.a in tomoReconApp/src/os/linux-x86 and linux-x86_64 to the SVN
  repository and source code distribution. Previous source-code releases were missing
  these files, so it would not build.
- Modified tomoReconApp/src/Makefile to show how to build with the system version
  of libfftw3f if that is desired.
- Minor change to IDL/tomo_recon.pro so that it will compile with GDL (GNU Data Language) as well as IDL.
- Tested the IDL/tests programs with GDL, and they work, including the netCDF and
  call_external code. This is good news, since it means that an IDL license is not
  needed to use the tomo_recon.pro front-end to tomoRecon.

## R1-1 (March 7, 2013)
 - Improvements to the Linux pre-built version
   - Changed the Makefile so it links with the static version of the libCom library.
     This means that libtomoRecon.so no longer depends on libCom.so. Previously libCom.so
     needed to be available at run-time.
   - Changed the Makefile so it links with the static version of the libfftw3f library.
     This means that libtomorecon.so no longer depends on libfftw3f.so. Previously libfftw3f.so
     needed to be available at run-time.
   - Added 2 new architectures, linux-x86-gcc43 and linux-x86_64-gcc42. The linux-x86_64
     architecture is built using gcc 4.6.3. It will not run on systems with older versions
     of gcc because of incompatible changes to the GCC and C run-time libraries. linux-x86-gcc43
     was built with gcc 4.3.0, and linux-x86_64-gcc42 was built with gcc 4.2.1. These
     builds will run on older Linux systems.

## R1-0 (August 17, 2012)
- Initial release of tomoRecon module.
