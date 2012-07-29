t0 = systime(1)
slice = reform(read_tomo_volume('Ag_22_A_.volume', yrange=[100, 100]))
best1 = optimize_center(slice, 694.55, .1, 40, entropy=entropy1, recon=recon, air=10, center=center, histMin=-.1, histMax=.1, numThreads=12)
plot, center, entropy1, ystyle=1, psym=-1
print, 'Best index=', best1, ' best center=', center[best1], ' time=', systime(1)-t0 
slice = reform(read_tomo_volume('Ag_22_A_.volume', yrange=[900,900]))
best2 = optimize_center(slice, 694.55, .1, 40, entropy=entropy2, recon=recon, air=10, center=center, histMin=-.1, histMax=.1, numThreads=12)
offset = entropy2[best2]-entropy1[best1]
oplot, center, entropy2-offset, psym=-2
print, 'Best index=', best2, ' best center=', center[best2], ' time=', systime(1)-t0 
end
       
