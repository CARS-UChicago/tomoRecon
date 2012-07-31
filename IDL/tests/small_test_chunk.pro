; Program to test tomo_recon_netcdf with a medium dataset

; The environment variable TOMO_RECON_SHARE must be set to point to tomoRecon.dll (Windows) or libtomoRecon.so (Linux)
; For example SET TOMO_RECON_SHARE=J:\epics\devel\tomoRecon\bin\windows-x64\tomoRecon.dll
; You must set your default directory to the location with the data files

numThreads=12

print, systime(0), ' small_test_chunk: calling tomo_recon_netcdf'
t0 = systime(1)
tomo_recon_netcdf, 'md_6_30min_A_.volume', 'md_6_30min_A_recon.volume', $
    maxSlices=500, $
    numThreads=numThreads, $
    debug=1, $
    dbgFile='small_test_chunk_debug.txt', $
    airPixels=0, $
    center=349

print, systime(0), ' small_chunk: numTheads = ', numThreads, ' Elapsed time = ', systime(1) - t0

vol = read_tomo_volume('md_6_30min_A_recon.volume', yrange=[350, 350])
image_display, vol
tomo_recon_delete
end
