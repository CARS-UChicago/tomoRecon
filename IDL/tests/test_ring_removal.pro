; Program to test right artifact removal

; The environment variable TOMO_RECON_SHARE must be set to point to tomoRecon.dll (Windows) or libtomoRecon.so (Linux)
; For example SET TOMO_RECON_SHARE=J:\epics\devel\tomoRecon\bin\windows-x64\tomoRecon.dll
; You must set your default directory to the location with the data files

; Delete any existing 3-D arrays from this program
vol = 0
recon = 0

print, systime(0), ' test_ring_removal: Reading normalized input file in netCDF format'
vol = read_tomo_volume('L62_4D_pt_13p8E_30mm_w5D_1_.volume', yrange=[500, 520])
numThreads = 12
ringWidth = 21
print, systime(0), ' test_ring_removal: Calling tomo_recon'
t0 = systime(1)
tomo_recon, vol, recon, debug=0, airPixels=10, ringWidth=ringWidth, center=1014, numThreads=numThreads
print, systime(0), ' test_ring_removal: numTheads = ', numThreads, ' ringWidth = ', ringWidth, ' Elapsed time = ', systime(1) - t0

slice = recon[*,*,0]
slice = rebin(slice, 512, 512)
window, 0, xsize=512, ysize=512
tv, bytscl(slice, min=-.001, max=.005)

end
