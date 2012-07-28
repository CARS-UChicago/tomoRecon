; Program to test tomo_recon with a large sized dataset

; The environment variable TOMO_RECON_SHARE must be set to point to tomoRecon.dll (Windows) or libtomoRecon.so (Linux)
; For example SET TOMO_RECON_SHARE=J:\epics\devel\tomoRecon\bin\windows-x64\tomoRecon.dll
; You must set your default directory to the location with the data files

; Delete any existing 3-D arrays from this program
vol = 0
recon = 0

;print, systime(0), ' medium_test: Reading normalized input file in raw format'
;openr, lun, /get, 'L62_4D_pt_13p8E_30mm_w5D_1_.raw'
;vol = intarr(2048, 2048, 1500, /nozero)
;readu, lun, vol
; Use the following line to only reconstruct the first half of the dataset because of memory limitations
;vol = vol[*,0:1023,*]
;close, lun

; Use the following instead of the lines above to read the .volume file instead of the .raw file.  
; This takes less memory because limiting the number of slices is done in the file reading, 
; rather than after reading.
; It does require the CARS tomography software, which contains "read_tomo_volume"
print, systime(0), ' large_test: Reading normalized input file in netCDF format'
vol = read_tomo_volume('L62_4D_pt_13p8E_30mm_w5D_1_.volume', yrange=[0, 256])
numThreads = 12
print, systime(0), ' large_test: Calling tomo_recon'
t0 = systime(1)
tomo_recon, vol, recon, debug=0, airPixels=10, centerOffset=1012, numThreads=numThreads, paddedSinogramWidth=2048
while (1) do begin
  tomo_recon_poll, reconComplete, slicesRemaining
  if (reconComplete) then break
  wait, 0.01
endwhile
help, recon
print, systime(0), ' large_test: numTheads = ', numThreads, ' Elapsed time = ', systime(1) - t0

end
