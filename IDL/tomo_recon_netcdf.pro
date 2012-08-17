pro tomo_recon_netcdf, input_file, output_file, maxSlices=maxSlices, _REF_EXTRA=extra

  ; Open the input netCDF file to get the dimensions
  file_id = ncdf_open(input_file, /nowrite)
  if (n_elements(file_id) eq 0) then message, 'Not a netCDF file'
  vol_id = ncdf_varid (file_id, 'VOLUME')
  if (vol_id eq -1) then begin
    ncdf_close, file_id
    message, 'No VOLUME variable in netCDF file'
  endif
  ; Get information about the volume variable
  vol_info = ncdf_varinq(file_id, vol_id)
  if (vol_info.ndims ne 3) then begin
      ncdf_close, file_id
      message, 'VOLUME variable does not have 3 dimensions in netCDF file'
  endif
  ncdf_diminq, file_id, vol_info.dim[0], name, numPixels
  ncdf_diminq, file_id, vol_info.dim[1], name, numSlices
  ncdf_diminq, file_id, vol_info.dim[2], name, numProjections
  ncdf_close, file_id
  
  if (n_elements(maxSlices) eq 0) then maxSlices = numSlices
  
  ; Create the output file
  dummy = intarr(2,2,2)
  write_tomo_volume, output_file, dummy, xmax=numPixels, ymax=numPixels, zmax=numSlices
  nextSlice = 0
  v1Exists = 0
  v2Exists = 0
  create = 1
  readTime = 0
  writeTime = 0
  reconWaitTime = 0
  inputConvertTime = 0
  outputConvertTime = 0
  tStart = systime(1)
  
  repeat begin
    if (nextSlice lt numSlices) then begin
        print, systime(0), ' tomo_recon_netcdf: reading v1, nextslice = ', nextSlice
        t0 = systime(1)
        v1 = read_tomo_volume(input_file, yrange=[nextSlice, (nextSlice+maxSlices-1)])
        readTime = readTime + systime(1) - t0
        t0 = systime(1)
        v1 = v1/1.e4
        inputConvertTime = inputConvertTime + systime(1) - t0
        v1Exists = 1
        v1Offset = nextSlice
        nextSlice = nextSlice + maxSlices
    endif else begin
        v1Exists = 0
    endelse
    if (v2Exists) then begin
        ; v2 is reconstructing; wait for it to get done
        print, systime(0), ' tomo_recon_netcdf: waiting for v2 reconstruction, v2Offset = ', v2Offset
        t0 = systime(1)
        repeat begin
            tomo_recon_poll, reconComplete, slicesRemaining
            wait, .01
        endrep until reconComplete
        reconWaitTime = reconWaitTime + systime(1) - t0
        v2 = 0
        print, systime(0), ' tomo_recon_netcdf:  v2 reconstruction complete '
    endif
    if (v1Exists) then begin
        ; Reconstruct v1
        print, systime(0), ' tomo_recon_netcdf: starting v1 reconstruction v1Offset = ', v1Offset
        tomo_recon, v1, r1, wait=0, create=create, _EXTRA=extra
        create = 0
    endif
    if (v2Exists) then begin
        print, systime(0), ' tomo_recon_netcdf: writing v2, offset = ', v2Offset
        t0 = systime(1)
        r2 *= 1.e6
        r2 = fix(r2)
        outputConvertTime = outputConvertTime + systime(1) - t0
        t0 = systime(1)
        write_tomo_volume, output_file, r2, zoffset=v2Offset, /append
        writeTime = writeTime + systime(1) - t0
    endif
    if (nextSlice lt numSlices) then begin
        print, systime(0), ' tomo_recon_netcdf: reading v2, nextslice = ', nextSlice
        t0 = systime(1)
        v2 = read_tomo_volume(input_file, yrange=[nextSlice, (nextSlice+maxSlices-1)])
        readTime = readTime + systime(1) - t0
        t0 = systime(1)
        v2 = v2/1.e4
        inputConvertTime = inputConvertTime + systime(1) - t0
        v2Exists = 1
        v2Offset = nextSlice
        nextSlice = nextSlice + maxSlices
    endif else begin
        v2Exists = 0
    endelse
    if (v1Exists) then begin
        ; v1 is reconstructing; wait for it to get done
        print, systime(0), ' tomo_recon_netcdf: waiting for v1 reconstruction, v1Offset = ', v1Offset
        t0 = systime(1)
        repeat begin
            tomo_recon_poll, reconComplete, slicesRemaining 
            wait, 0.01
        endrep until reconComplete
        reconWaitTime = reconWaitTime + systime(1) - t0
        v1 = 0
        print, systime(0), ' tomo_recon_netcdf:  v1 reconstruction complete '
    endif
    if (v2Exists) then begin
        ; Reconstruct v2
        print, systime(0), ' tomo_recon_netcdf: starting v2 reconstruction v2Offset = ', v2Offset
        tomo_recon, v2, r2, wait=0, create=create, _EXTRA=extra
        create = 0
    endif
    if (v1Exists) then begin
        print, systime(0), ' tomo_recon_netcdf: writing v1, offset = ', v1Offset
        t0 = systime(1)
        r1 *= 1.e6
        r1 = fix(r1)
        outputConvertTime = outputConvertTime + systime(1) - t0
        t0 = systime(1)
        write_tomo_volume, output_file, r1, zoffset=v1Offset, /append
        writeTime = writeTime + systime(1) - t0
    endif
  endrep until ((v1Exists eq 0) and (v2Exists eq 0))
  
  tEnd = systime(1)
  print, 'read_tomo_netcdf: times, read=', readTime, ' write=', writeTime, $
         ' input convert=', inputConvertTime, ' output convert=', outputConvertTime, $
         ' recon wait=', reconWaitTime, ' total=', tEnd-tStart

end
