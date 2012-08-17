;+
; NAME:
;   OPTIMIZE_CENTER
;
; PURPOSE:
;   Finds the best rotation center for tomographic reconstruction using the tomo_recon reconstruction code.
;   This is done by reconstructing a single slice with a set of rotation centers and measuring the image entropy
;   for each reconstruction.  The optimum rotation center is the one that produces the minimum image entropy.
;
; CATEGORY:
;   Tomography data processing
;
; CALLING SEQUENCE:
;   MinIndex = optimize_center(Slice, InCenter, Step, NumCenter)
;
; INPUTS:
;   Slice:
;       A slice to be reconstructed, [NumPixels, NumProjections]
;
;   InCenter:
;       The rotation center to seach about for the optimum.  
;       This can be thought of as the initial guess of the optimum rotation center
;
;   Step:
;       The step size in pixels for the rotation center array.  This can be a floating
;       point number, i.e. fractional pixels are allowed.
;
;   NumCenter:
;       The number of rotation center positions to use.  This should be an odd number,
;       in which case the InCenter value will be in the middle of the CENTER array.
;
; OUTPUTS:
;   This function returns the index of the center position that results in the minimum entropy 
;   in the reconstructed image.  This index can be used to find the optimum center position,
;   CENTER[MinIndex], the mimimum entropy value, ENTROPY[MinIndex], and the optimum reconstructed
;   slice, RECON[*,*,MinIndex].
;
; KEYWORD PARAMETERS:
;   CENTER:
;       An output array of dimensions NumCenter.  It contains the center positions
;       used in the optimization.  It is computed as:
;       center = inCenter + (findgen(numCenter) - numCenter/2)*step
;   ENTROPY:
;       An output array of dimensions NumCenter.  It contains the entropy
;       of the reconstruction at each center position.
;       The entropy of a slice is computed as follows:
;         - H, the histogram of the slice is computed, using 10000 bins from HISTMIN to HISTMAX.
;         - H is scaled (divided) by the number of pixels in the slice
;         - The entropy is then computed as the sum of H*log(H), summing over the 10000 elements in H
;   RECON:
;       An output float array of reconstructed slices [numPixels, numPixels, numCenter].
;   HISTMIN:
;       The minimum reconstructed value to be used when computing the histogram to compute
;       the entropy.
;       The default is the half minimum value of the center slice in RECON, i.e. min(RECON[*,*,numCenter/2]) * 0.5
;   HISTMAX:
;       The maximum reconstructed value to be used when computing the histogram to compute
;       the entropy.
;       The default is the twice maximum value of the center slice in RECON, i.e. max(RECON[*,*,numCenter/2]) * 2.0
;       
;
; PROCEDURE:
;   This function constructs the CENTER array of rotation centers.  It then replicates Slice into a
;   3-D array [NumPixels, NumCenter, NumProjections].
;   libtomoRecon.so (Linux)  or tomoRecon.dll (Windows), which is written in C++.
;;
; EXAMPLE:
;   TOMO_RECON, input, output, center=419
;
; MODIFICATION HISTORY:
;   Written by:     Mark Rivers, August 1, 2012
;-

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
   center = inCenter + (findgen(numCenter) - numCenter/2)*step
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
