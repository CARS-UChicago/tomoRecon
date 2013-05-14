pro fft_test2f, data, isign

   ; We use a common block just to store info through calls
   common tomo_recon_common, tomo_recon_shareable_library

   s = size(data, /dimensions)
   nx = s[0]
   ny = s[1]
   t = call_external(library, 'fftw_2d', complex(data), nx, ny, long(isign))
end
