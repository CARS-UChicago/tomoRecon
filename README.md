## tomoRecon: Tomography preprocessing and reconstruction with multiple threads

tomoRecon is software for performing tomography preprocessing and parallel beam reconstruction using
multiple threads. It uses the [EPICS libCom](http://www.aps.anl.gov/epics)
library for operating system independent support for threads, mutexes, message queues,
etc. It uses a C++ version of the Gridrec software for the actual reconstruction.
The library is provided in both source code form, and as 64-bit shareable libraries
for Linux and Windows. The shareable libraries can be called from C++, IDL, GDL, or other languages.

tomoRecon is described in this 
[paper from the 2012 SPIE X-Ray Tomography VII conference](https://github.com/CARS-UChicago/tomoRecon/blob/master/documentation/SPIE_tomoRecon.pdf).

IDL and GDL can be used as the "front-end" to tomoRecon, with IDL or GDL reading and writing the files and displaying images. 
There are [IDL routines](http://github.com/CARS-UChicago/IDL_Tomography) that call tomoRecon, including a GUI.

To use tomoRecon from IDL or GDL with tomo_recon.pro, the shareable library needs
to be found. This can be done in two different ways.
  - Define the environment variable TOMO_RECON_SHARE to point to the shareable library.
    For example on the Linux bash shell "export TOMO_RECON_SHARE=/usr/local/lib/libtomoRecon.so".
    On Windows this might be "set TOMO_RECON_SHARE=C:\tomoRecon\tomoRecon.dll".
  - If the environment variables does not exist, then it looks for the shareable library
    in the IDL "path", and the shareable library must be named: 
    'tomoRecon_' + !version.os + '_' + !version.arch + '.so' or '.dll' For example, tomoRecon_Win32_x86_64.dll
    or tomoRecon_linux_x86_64.so. This can be done either by renaming the shareable
    library or by using soft-links to the actual shareable library file.

### Additional information:
* [Release notes](RELEASE.md)
