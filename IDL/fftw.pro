function fftw, data, direction, inverse=inverse, overwrite=overwrite

; This function is very similar to the built-in IDL function "fft", but it uses the
; fftw library.  It only works with 1-D or 2-D data, and only single-precision complex
; type, not double precision.

  ; We use a common block just to store info through calls
  common tomo_recon_common, tomo_recon_shareable_library

  if (n_elements(direction) eq 0) then direction = -1
  if (keyword_set(inverse)) then direction = 1
  if (keyword_set(overwrite)) then overwrite = 1 else overwrite = 0

  locate_tomo_recon_shareable_library

  n_dimensions = size(data, /n_dimensions)
  dims = size(data, /dimensions)
  type = size(data, /type)
  if (type ne 6) then data = complex(data)
  if (overwrite eq 0) then saveData = data;

  case n_dimensions of
    1: t = call_external(tomo_recon_shareable_library, 'fftw_1d', data, long(dims[0]), long(direction))
    2: t = call_external(tomo_recon_shareable_library, 'fftw_2d', data, long(dims[0]), long(dims[1]), long(direction))
    else: message, 'Error, fftw only accepts 1-D or 2-D data'
  endcase
  if (overwrite eq 1) then return, data
  temp = data
  data = saveData
  return, temp
     
end
