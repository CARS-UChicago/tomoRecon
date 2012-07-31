function optimize_center, slice, inCenter, step, numCenter, $
            center=center, $
            entropy=entropy, $
            recon=recon, $
            histMin=histMin, $
            histMax=histMax, $
            _REF_EXTRA=extra

   ; This function uses tomoRecon to reconstruct a single slice at different rotation centers.
   ; It calculates an array "entropy" containing a figure of merit as the rotation center is varied
   ; center is an array of rotation centers
   ; entropy is the entropy at each point
   entropy = dblarr(numCenter)
   center = inCenter + (findgen(numCenter) - numCenter/2.)*step
   s = size(slice, /dimensions)
   if (n_elements(s) ne 2) then message, 'Must pass a 2-D slice'
   nx = s[0]
   nr = s[1]
   input = fltarr(nx, numCenter, nr)
   for i=0, numCenter-1 do begin
      input[0:nx-1,i,  0:nr-1] = float(slice)
   endfor
   
   tomo_recon, input, recon, center=center, _EXTRA = extra
   ; Use the slice in center of range to get min/max of reconstruction for histogram
   r = recon[*,*,numCenter/2]
   if (n_elements(histMin) eq 0) then begin
        histMin = min(r)
        if ((histMin) lt 0) then histMin=2*histMin else histMin=0.5*histMin
   endif
   if (n_elements(histMax) eq 0) then begin
        histMax = max(r)
        if ((histMax) gt 0) then histMax=2*histMax else histMax=0.5*histMax
   endif
   binsize=(histMax-histMin)/1.e4
   npixels = n_elements(r)
   for i=0, numCenter-1 do begin
      r = recon[*,*,i]
      h = histogram(r, min=histMin, max=histMax, bin=binsize) > 1
      h = float(h) / npixels
      entropy[i] = -total(h*alog(h))
   endfor
   minEntropy = min(entropy, minIndex)
   return, minIndex
end
