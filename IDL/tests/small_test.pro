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
free_lun, lun
vol = vol/1.e4
numThreads = 8

; Reconstruct once creating a new tomoRecon object
print, systime(0), ' small_test: Calling tomo_recon with create=1'
t0 = systime(1)
tomo_recon, vol, recon, debug=0, dbgFile='', airPixels=10, center=349, numThreads=numThreads, create=1
print, systime(0), ' small_test: numTheads = ', numThreads, ' Elapsed time = ', systime(1) - t0

; Reconstruct again using the same tomoRecon object.  Will use the same reconstruction parameters from above
print, systime(0), ' small_test: Calling tomo_recon with create=0'
t0 = systime(1)
tomo_recon, vol, recon, create=0
print, systime(0), ' small_test: numTheads = ', numThreads, ' Elapsed time = ', systime(1) - t0

end
