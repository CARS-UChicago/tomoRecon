slice = reform(read_tomo_volume('L62_4D_pt_13p8E_30mm_w5D_1_.volume', yrange=[250, 250]))
t0 = systime(1)
best1 = optimize_center(slice, 1011.5, .1, 41, entropy=entropy1, recon=recon1, air=10, center=center, histMin=-.1, histMax=.1, numThreads=10)
print, 'Best index=', best1, ' best center=', center[best1], ' time=', systime(1)-t0 
plot, center, entropy1, ystyle=1, psym=-1

slice = reform(read_tomo_volume('L62_4D_pt_13p8E_30mm_w5D_1_.volume', yrange=[1800,1800]))
t0 = systime(1)
best2 = optimize_center(slice, 1011.5, .1, 41, entropy=entropy2, recon=recon2, air=10, center=center, histMin=-.1, histMax=.1, numThreads=10)
print, 'Best index=', best2, ' best center=', center[best2], ' time=', systime(1)-t0 
offset = entropy2[best2]-entropy1[best1]
oplot, center, entropy2-offset, psym=-2

end
       
