pro fft_test1f, data, isign

   ; We use a common block just to store info through calls
   common tomo_recon_common, tomo_recon_shareable_library

   s = size(data, /dimensions)
   nx = s[0]
   t = call_external(library, 'fftw_1d', complex(data), nx, long(isign))
end
