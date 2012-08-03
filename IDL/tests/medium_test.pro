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
free_lun, lun
print, systime(0), ' medium_test: Converting to float'
vol = vol / 1.e4
numThreads = 8
print, systime(0), ' medium_test: Calling tomo_recon'
t0 = systime(1)
tomo_recon, vol, recon, debug=0, airPixels=10, center=694, numThreads=numThreads
print, systime(0), ' medium_test: numTheads = ', numThreads, ' Elapsed time = ', systime(1) - t0

end
