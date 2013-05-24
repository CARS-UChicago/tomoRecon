pro fft_test1d, nx=nx, isign=isign, nloop=nloop, f0, f1   

   if (n_elements(nx) eq 0)    then nx = 2048
   if (n_elements(nloop) eq 0) then nloop = 2048
   if (n_elements(isign) eq 0) then isign=1
   data = complexarr(nx)
   data[200:300]=complex(1.5,.5)
   t0 = systime(1)
   for i=0, nloop-1 do begin
      f0 = fft(data, isign)
   endfor
   ; Renormalize
   if (isign eq -1) then f0 = f0 * nx
   print, 'Time with IDL internal FFT', systime(1)-t0

   t0 = systime(1)
   for i=0, nloop-1 do begin
      f1 = fftw(data, isign)
   endfor
   print, 'Time with FFTW', systime(1)-t0

   print, 'Differences:'
   diff10 = f1 - f0;
   print, ' FFTW and IDL:               max= ', $
           max(abs(diff10)), ' RMS = ', sqrt(total(abs(diff10)^2)/nx)
end
