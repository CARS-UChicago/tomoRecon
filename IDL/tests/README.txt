This folder contains IDL programs to demonstrate and test the tomoRecon code

The tests require input data which can be found in the following location:
   http://cars.uchicago.edu/gsecars/data/tomoRecon/
   
The tests require the .raw files from this folder, which are in little-endian raw format.
There are also .volume file, which contain the same data in netCDF format.

In each test the number of threads can be changed.  The default is 8.

The tests were done on the following:
- Windows 7 machine with dual quad-core (8 cores total) Xeon X5647 processors running at 2.93 GHz, 48 GB of RAM.
  Two 500 GB 15K RPM SAS disks arranged in Raid 0 configuration.
  
- Fedora Core 15 Linux system with 16 cores, Xeon E5630 running at 2.53GHz, 12 GB of RAM.
  I am not sure about the disk speed or Raid configuration on this machine.


                   small_test.pro
The small_test.pro program reads md_6_30min_A_.raw.  
This is a 16-bit integer dataset, [696, 520, 720] [X, Y, Projections]

The following is the output when run on my Windows 7 machine, using 8 threads:
Fri Jul 27 16:43:11 2012 small_test: Reading normalized input file in raw format
Fri Jul 27 16:43:11 2012 small_test: Calling tomo_recon
Fri Jul 27 16:43:11 2012 tomoRecon: Converting input to float
Fri Jul 27 16:43:11 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 16:43:16 2012 small_test: numTheads =        8 Elapsed time =        4.6799998

So it took a total of 4.7 seconds to reconstruct the entire dataset.


The following is the output when run on my Linux machine using 12 threads:
IDL> .run ~/devel/tomoRecon/IDL/tests/small_test   
Fri Jul 27 17:19:49 2012 small_test: Reading normalized input file in raw format
Fri Jul 27 17:19:49 2012 small_test: Calling tomo_recon
Fri Jul 27 17:19:49 2012 tomoRecon: Converting input to float
Fri Jul 27 17:19:49 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 17:19:54 2012 small_test: numTheads =       12 Elapsed time =        4.6249471

So it took a total of 4.6 seconds to reconstruct the entire dataset.


                   medium_test.pro
The medium_test.pro program reads md_6_30min_A_.raw.  
This is a 16-bit integer dataset, [1392, 1040, 900] [X, Y, Projections]

The following is the output when run on my Windows 7 machine, using 8 threads:
Fri Jul 27 17:35:52 2012 medium_test: Reading normalized input file in raw format
Fri Jul 27 17:35:54 2012 medium_test: Calling tomo_recon
Fri Jul 27 17:35:54 2012 tomoRecon: Converting input to float
Fri Jul 27 17:35:56 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 17:36:22 2012 medium_test: numTheads =        8 Elapsed time =        27.831000

So it took a total of 27.8 seconds to reconstruct the entire dataset.

The following is the output when run on my Linux machine, using 12 threads:
Fri Jul 27 18:00:13 2012 medium_test: Reading normalized input file in raw format
Fri Jul 27 18:00:15 2012 medium_test: Calling tomo_recon
Fri Jul 27 18:00:15 2012 tomoRecon: Converting input to float
Fri Jul 27 18:00:15 2012 tomo_recon: Calling tomoReconStartIDL C code 
RECON           FLOAT     = Array[1392, 1392, 520]
Fri Jul 27 18:00:29 2012 medium_test: numTheads =       12 Elapsed time =        13.980863

The Linux machine does not have enough memory to reconstruct the entire dataset in memory, so I just
reconstructed the first 520 slice. It took 14.0 seconds for this, so it would take 28 seconds for the
entire dataset.


                   large_test.pro
The large_test.pro program reads L62_4D_pt_13p8E_30mm_w5D_1_.raw, or L62_4D_pt_13p8E_30mm_w5D_1_.volume,
which is in netCDF format and requires less memory when only reading some of the slices.
This is a 16-bit integer dataset, [2048, 2048, 1500] [X, Y, Projections]

The following is the output when run on my Windows 7 machine, using 8 threads:
Fri Jul 27 18:31:27 2012 large_test: Reading normalized input file in netCDF format
Fri Jul 27 18:31:35 2012 large_test: Calling tomo_recon
Fri Jul 27 18:31:35 2012 tomoRecon: Converting input to float
Fri Jul 27 18:31:41 2012 tomo_recon: Calling tomoReconStartIDL C code 
RECON           FLOAT     = Array[2048, 2048, 1024]
Fri Jul 27 18:32:20 2012 large_test: numTheads =        8 Elapsed time =        45.179000

The Windows machine does not have enough memory to reconstruct the entire dataset in memory, so I just
reconstructed the first 1024 slices. I read the netCDF file, though the Windows machine has enough memory to 
read the .raw file and truncate the vol array after reading. It took 44.5 seconds for this, 
so it would take 90.4 seconds for the entire dataset if the machine had enough memory (64GB would be enough).

The following is the output when run on my Linux machine, using 12 threads:
Fri Jul 27 18:40:39 2012 large_test: Reading normalized input file in netCDF format
Fri Jul 27 18:40:46 2012 large_test: Calling tomo_recon
Fri Jul 27 18:40:46 2012 tomoRecon: Converting input to float
Fri Jul 27 18:40:46 2012 tomo_recon: Calling tomoReconStartIDL C code 
RECON           FLOAT     = Array[2048, 2048, 257]
Fri Jul 27 18:40:57 2012 large_test: numTheads =       12 Elapsed time =        11.325219

The Windows machine does not have enough memory to reconstruct the entire dataset in memory, so I just
reconstructed the first 256 slices. I read the netCDF file, because the Linux machine does not have enough memory
to read the entire .raw file. It took 11.3 seconds for this, so it would take 11.3*8=90.4 seconds for the 
entire dataset if the machine had enough memory.

                   large_test_chunk.pro
The large_test.pro program reads the same dataset, L62_4D_pt_13p8E_30mm_w5D_1_.volume, as large_test.pro.
It processes the entire dataset in "chunks", where the chunk size is selected to fit the available memory.
Unlike the tests above, this test also writes out the reconstructed data in a netCDF file.  Thus, it is
an example of a complete processing application. The file reading/writing and reconstruction are overlapped
to the maximum degree possible, so it is doing file I/O at the same time that the reconstruction is
processing.

The following is the output when run on my Windows 7 machine, using 7 threads and a chunk size of 685 slices.
This chunk size requires 3 chunks to process the entire dataset, and uses about 38 GB of memory.

Fri Jul 27 18:46:52 2012 large_test_chunk: calling tomo_recon_netcdf
Fri Jul 27 18:46:52 2012 tomo_recon_netcdf: reading v1, nextslice =        0
Fri Jul 27 18:47:03 2012 tomo_recon_netcdf: starting v1 reconstruction v1Offset =        0
Fri Jul 27 18:47:03 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 18:47:03 2012 tomo_recon_netcdf: reading v2, nextslice =      685
Fri Jul 27 18:47:33 2012 tomo_recon_netcdf: waiting for v1 reconstruction, v1Offset =        0
Fri Jul 27 18:47:37 2012 tomo_recon_netcdf:  v1 reconstruction complete 
Fri Jul 27 18:47:37 2012 tomo_recon_netcdf: starting v2 reconstruction v2Offset =      685
Fri Jul 27 18:47:37 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 18:47:37 2012 tomo_recon_netcdf: writing v1, offset =        0
Fri Jul 27 18:48:07 2012 tomo_recon_netcdf: reading v1, nextslice =     1370
Fri Jul 27 18:48:53 2012 tomo_recon_netcdf: waiting for v2 reconstruction, v2Offset =      685
Fri Jul 27 18:48:54 2012 tomo_recon_netcdf:  v2 reconstruction complete 
Fri Jul 27 18:48:54 2012 tomo_recon_netcdf: starting v1 reconstruction v1Offset =     1370
Fri Jul 27 18:48:54 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 18:48:54 2012 tomo_recon_netcdf: writing v2, offset =      685
Fri Jul 27 18:49:26 2012 tomo_recon_netcdf: waiting for v1 reconstruction, v1Offset =     1370
Fri Jul 27 18:49:31 2012 tomo_recon_netcdf:  v1 reconstruction complete 
Fri Jul 27 18:49:31 2012 tomo_recon_netcdf: writing v1, offset =     1370
Fri Jul 27 18:50:02 2012 large_test_chunk: numTheads =        7 Elapsed time =        190.21300

So it took 190.2 seconds, (00:3:10.20) to reconstruct the entire dataset, including writing the output to disk.
I used 7 threads for the reconstruction, rather than 8, so that there would be one core available to handle the I/O being done from 
the main IDL process.  One can see that the above chunking was close to optimum, because when the file I/O completed the
reconstruction was almost complete, but not quite.  It waited 4 seconds, 1 second, and 5 seconds for the
reconstruction to complete after writing the results of the previous reconstruction and reading the next input chunk.


The following is the output when run on my Linux machine, using 12 threads and a chunk size of 171 slices.
This chunk size requires 12 chunks to process the entire dataset, and uses about 8 GB of memory.

IDL> .run ~/devel/tomoRecon/IDL/tests/large_test_chunk 
Fri Jul 27 19:07:15 2012 large_test_chunk: calling tomo_recon_netcdf
Fri Jul 27 19:07:15 2012 tomo_recon_netcdf: reading v1, nextslice =        0
Fri Jul 27 19:07:19 2012 tomo_recon_netcdf: starting v1 reconstruction v1Offset =        0
Fri Jul 27 19:07:19 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 19:07:19 2012 tomo_recon_netcdf: reading v2, nextslice =      171
Fri Jul 27 19:07:33 2012 tomo_recon_netcdf: waiting for v1 reconstruction, v1Offset =        0
Fri Jul 27 19:07:33 2012 tomo_recon_netcdf:  v1 reconstruction complete 
Fri Jul 27 19:07:33 2012 tomo_recon_netcdf: starting v2 reconstruction v2Offset =      171
Fri Jul 27 19:07:33 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 19:07:33 2012 tomo_recon_netcdf: writing v1, offset =        0
Fri Jul 27 19:07:45 2012 tomo_recon_netcdf: reading v1, nextslice =      342
Fri Jul 27 19:08:02 2012 tomo_recon_netcdf: waiting for v2 reconstruction, v2Offset =      171
Fri Jul 27 19:08:02 2012 tomo_recon_netcdf:  v2 reconstruction complete 
Fri Jul 27 19:08:02 2012 tomo_recon_netcdf: starting v1 reconstruction v1Offset =      342
Fri Jul 27 19:08:03 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 19:08:03 2012 tomo_recon_netcdf: writing v2, offset =      171
Fri Jul 27 19:08:21 2012 tomo_recon_netcdf: reading v2, nextslice =      513
Fri Jul 27 19:08:38 2012 tomo_recon_netcdf: waiting for v1 reconstruction, v1Offset =      342
Fri Jul 27 19:08:38 2012 tomo_recon_netcdf:  v1 reconstruction complete 
Fri Jul 27 19:08:38 2012 tomo_recon_netcdf: starting v2 reconstruction v2Offset =      513
Fri Jul 27 19:08:38 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 19:08:38 2012 tomo_recon_netcdf: writing v1, offset =      342
Fri Jul 27 19:09:07 2012 tomo_recon_netcdf: reading v1, nextslice =      684
Fri Jul 27 19:09:24 2012 tomo_recon_netcdf: waiting for v2 reconstruction, v2Offset =      513
Fri Jul 27 19:09:24 2012 tomo_recon_netcdf:  v2 reconstruction complete 
Fri Jul 27 19:09:24 2012 tomo_recon_netcdf: starting v1 reconstruction v1Offset =      684
Fri Jul 27 19:09:24 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 19:09:24 2012 tomo_recon_netcdf: writing v2, offset =      513
Fri Jul 27 19:09:49 2012 tomo_recon_netcdf: reading v2, nextslice =      855
Fri Jul 27 19:10:09 2012 tomo_recon_netcdf: waiting for v1 reconstruction, v1Offset =      684
Fri Jul 27 19:10:09 2012 tomo_recon_netcdf:  v1 reconstruction complete 
Fri Jul 27 19:10:09 2012 tomo_recon_netcdf: starting v2 reconstruction v2Offset =      855
Fri Jul 27 19:10:09 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 19:10:09 2012 tomo_recon_netcdf: writing v1, offset =      684
Fri Jul 27 19:10:27 2012 tomo_recon_netcdf: reading v1, nextslice =     1026
Fri Jul 27 19:10:46 2012 tomo_recon_netcdf: waiting for v2 reconstruction, v2Offset =      855
Fri Jul 27 19:10:46 2012 tomo_recon_netcdf:  v2 reconstruction complete 
Fri Jul 27 19:10:46 2012 tomo_recon_netcdf: starting v1 reconstruction v1Offset =     1026
Fri Jul 27 19:10:47 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 19:10:47 2012 tomo_recon_netcdf: writing v2, offset =      855
Fri Jul 27 19:11:04 2012 tomo_recon_netcdf: reading v2, nextslice =     1197
Fri Jul 27 19:11:23 2012 tomo_recon_netcdf: waiting for v1 reconstruction, v1Offset =     1026
Fri Jul 27 19:11:23 2012 tomo_recon_netcdf:  v1 reconstruction complete 
Fri Jul 27 19:11:23 2012 tomo_recon_netcdf: starting v2 reconstruction v2Offset =     1197
Fri Jul 27 19:11:24 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 19:11:24 2012 tomo_recon_netcdf: writing v1, offset =     1026
Fri Jul 27 19:11:47 2012 tomo_recon_netcdf: reading v1, nextslice =     1368
Fri Jul 27 19:12:07 2012 tomo_recon_netcdf: waiting for v2 reconstruction, v2Offset =     1197
Fri Jul 27 19:12:07 2012 tomo_recon_netcdf:  v2 reconstruction complete 
Fri Jul 27 19:12:07 2012 tomo_recon_netcdf: starting v1 reconstruction v1Offset =     1368
Fri Jul 27 19:12:07 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 19:12:07 2012 tomo_recon_netcdf: writing v2, offset =     1197
Fri Jul 27 19:12:27 2012 tomo_recon_netcdf: reading v2, nextslice =     1539
Fri Jul 27 19:12:46 2012 tomo_recon_netcdf: waiting for v1 reconstruction, v1Offset =     1368
Fri Jul 27 19:12:46 2012 tomo_recon_netcdf:  v1 reconstruction complete 
Fri Jul 27 19:12:46 2012 tomo_recon_netcdf: starting v2 reconstruction v2Offset =     1539
Fri Jul 27 19:12:46 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 19:12:46 2012 tomo_recon_netcdf: writing v1, offset =     1368
Fri Jul 27 19:13:03 2012 tomo_recon_netcdf: reading v1, nextslice =     1710
Fri Jul 27 19:13:23 2012 tomo_recon_netcdf: waiting for v2 reconstruction, v2Offset =     1539
Fri Jul 27 19:13:23 2012 tomo_recon_netcdf:  v2 reconstruction complete 
Fri Jul 27 19:13:23 2012 tomo_recon_netcdf: starting v1 reconstruction v1Offset =     1710
Fri Jul 27 19:13:23 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 19:13:23 2012 tomo_recon_netcdf: writing v2, offset =     1539
Fri Jul 27 19:13:44 2012 tomo_recon_netcdf: reading v2, nextslice =     1881
Fri Jul 27 19:14:04 2012 tomo_recon_netcdf: waiting for v1 reconstruction, v1Offset =     1710
Fri Jul 27 19:14:04 2012 tomo_recon_netcdf:  v1 reconstruction complete 
Fri Jul 27 19:14:04 2012 tomo_recon_netcdf: starting v2 reconstruction v2Offset =     1881
Fri Jul 27 19:14:04 2012 tomo_recon: Calling tomoReconStartIDL C code 
Fri Jul 27 19:14:04 2012 tomo_recon_netcdf: writing v1, offset =     1710
Fri Jul 27 19:14:21 2012 tomo_recon_netcdf: waiting for v2 reconstruction, v2Offset =     1881
Fri Jul 27 19:14:21 2012 tomo_recon_netcdf:  v2 reconstruction complete 
Fri Jul 27 19:14:21 2012 tomo_recon_netcdf: writing v2, offset =     1881
Fri Jul 27 19:14:28 2012 large_test_chunk: numTheads =       12 Elapsed time =        433.77212

So it took 433.77 seconds, (00:7:13.77) to reconstruct the entire dataset, including writing the output to disk.
One can see that the above chunking was not working as well as one would like, because the reconstruction was
always complete before the file I/O completed.  The times to read and write the files are considerably slower
on this Linux machine than on the Windows machine.  The time per byte might improve if we were able to use
a larger chunk size, which would require more memory.





