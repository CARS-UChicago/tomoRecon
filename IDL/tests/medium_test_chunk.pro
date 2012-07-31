; Program to test tomo_recon_netcdf with a medium dataset

; The environment variable TOMO_RECON_SHARE must be set to point to tomoRecon.dll (Windows) or libtomoRecon.so (Linux)
; For example SET TOMO_RECON_SHARE=J:\epics\devel\tomoRecon\bin\windows-x64\tomoRecon.dll
; You must set your default directory to the location with the data files

numThreads=12

print, systime(0), ' medium_test_chunk: calling tomo_recon_netcdf'
t0 = systime(1)
tomo_recon_netcdf, 'Ag_22_A_.volume', 'Ag_22_A_recon.volume', $
    maxSlices=260, $
    numThreads=numThreads, $
    debug=0, $
    airPixels=0, $
    centerOffset=694
    
print, systime(0), ' medium_chunk: numTheads = ', numThreads, ' Elapsed time = ', systime(1) - t0

end
