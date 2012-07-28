; Program to test tomo_recon_netcdf with a large dataset

; The environment variable TOMO_RECON_SHARE must be set to point to tomoRecon.dll (Windows) or libtomoRecon.so (Linux)
; For example SET TOMO_RECON_SHARE=J:\epics\devel\tomoRecon\bin\windows-x64\tomoRecon.dll
; You must set your default directory to the location with the data files

numThreads=8

print, systime(0), ' large_test_chunk: calling tomo_recon_netcdf'
t0 = systime(1)
tomo_recon_netcdf, 'L62_4D_pt_13p8E_30mm_w5D_1_.volume', 'L62_4D_pt_13p8E_30mm_w5D_1_recon.volume', $
    maxSlices=685, $
    numThreads=numThreads, $
    debug=0, $
    airPixels=0, $
    centerOffset=1012, $
    paddedSinogramWidth=2048

print, systime(0), ' large_test_chunk: numTheads = ', numThreads, ' Elapsed time = ', systime(1) - t0

end
