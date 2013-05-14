pro fft_test2d, nx=nx, ny=ny, isign=isign, nloop=nloop, f0, f1, f2, f3

   if (n_elements(nx) eq 0)    then nx = 1024
   if (n_elements(ny) eq 0)    then ny = 1024
   if (n_elements(nloop) eq 0) then nloop = 1
   if (n_elements(isign) eq 0) then isign=1

   data = complexarr(nx, ny)
   data[200:300, 300:320] = complex(1.5, .5)

   t0 = systime(1)
   for i=0, nloop-1 do begin
      f0 = fft(data, isign)
   endfor
   ; Renormalize
   if (isign eq -1) then f0 = f0 * nx * ny
   print, 'Time with IDL internal FFT', systime(1)-t0

   t0 = systime(1)
   for i=0, nloop-1 do begin 
      f3 = data
      fftw_2d, f3, isign
   endfor
   print, 'Time with FFTW', systime(1)-t0

   print, 'Differences:'
   diff30 = f3 - f0;
   print, ' FFTW and IDL:               max= ', $
           max(abs(diff30)), ' RMS = ', sqrt(total(abs(diff30)^2)/nx)
end

