slice = reform(read_tomo_volume('L62_4D_pt_13p8E_30mm_w5D_1_.volume', yrange=[100, 100]))
t0 = systime(1)
best1 = optimize_center(slice, 1014, .25, 40, entropy=entropy1, recon=recon, air=10, center=center, histMin=-.1, histMax=.1, numThreads=12)
print, 'Best index=', best1, ' best center=', center[best1], ' time=', systime(1)-t0 
plot, center, entropy1, ystyle=1, psym=-1

slice = reform(read_tomo_volume('L62_4D_pt_13p8E_30mm_w5D_1_.volume', yrange=[1900,1900]))
t0 = systime(1)
best2 = optimize_center(slice, 1014, .25, 40, entropy=entropy2, recon=recon, air=10, center=center, histMin=-.1, histMax=.1, numThreads=12)
print, 'Best index=', best2, ' best center=', center[best2], ' time=', systime(1)-t0 
offset = entropy2[best2]-entropy1[best1]
oplot, center, entropy2-offset, psym=-2

end
       
