; Program to test tomo_recon with a small dataset

; The environment variable TOMO_RECON_SHARE must be set to point to tomoRecon.dll (Windows) or libtomoRecon.so (Linux)
; For example SET TOMO_RECON_SHARE=J:\epics\devel\tomoRecon\bin\windows-x64\tomoRecon.dll
; You must set your default directory to the location with the data files

; Delete any existing 3-D arrays from this program
vol = 0
recon = 0

print, systime(0), ' small_test: Reading normalized input file in raw format'
openr, lun, /get, 'md_6_30min_A_.raw'
vol = intarr(696, 520, 720, /nozero)
readu, lun, vol
close, lun
numThreads = 12
print, systime(0), ' small_test: Calling tomo_recon'
t0 = systime(1)
tomo_recon, vol, recon, debug=0, airPixels=10, centerOffset=347.5, numThreads=numThreads, paddedSinogramWidth=1024
while (1) do begin
  tomo_recon_poll, reconComplete, slicesRemaining
  if (reconComplete) then break
  wait, 0.01
endwhile
print, systime(0), ' small_test: numTheads = ', numThreads, ' Elapsed time = ', systime(1) - t0

end
