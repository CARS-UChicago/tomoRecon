; Program to test tomo_recon with a medium sized dataset

; The environment variable TOMO_RECON_SHARE must be set to point to tomoRecon.dll (Windows) or libtomoRecon.so (Linux)
; For example SET TOMO_RECON_SHARE=J:\epics\devel\tomoRecon\bin\windows-x64\tomoRecon.dll
; You must set your default directory to the location with the data files

; Delete any existing 3-D arrays from this program
vol = 0
recon = 0

print, systime(0), ' medium_test: Reading normalized input file in raw format'
openr, lun, /get, 'Ag_22_A_.raw'
vol = intarr(1392, 1040, 900, /nozero)
readu, lun, vol
; Use the following line to only reconstruct the first half of the dataset because of memory limitations
;vol = vol[*,0:519,*]
close, lun
numThreads = 8
print, systime(0), ' medium_test: Calling tomo_recon'
t0 = systime(1)
tomo_recon, vol, recon, debug=0, airPixels=10, centerOffset=694, numThreads=numThreads, paddedSinogramWidth=2048
while (1) do begin
  tomo_recon_poll, reconComplete, slicesRemaining
  if (reconComplete) then break
  wait, 0.01
endwhile
help, recon
print, systime(0), ' medium_test: numTheads = ', numThreads, ' Elapsed time = ', systime(1) - t0

end