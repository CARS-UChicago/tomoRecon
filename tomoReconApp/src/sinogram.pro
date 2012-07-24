pro sine_wave, x, a, f, pder

; Used to evaluate a sine wave with unit frequency. This routine is used by
; sinogram to fit the center-of-gravity data
; a(0) = rotation center
; a(1) = amplitude
; a(2) = phase
f = a(0) + a(1)*sin(x + a(2))
pder = fltarr(n_elements(x), n_elements(a))
pder(*,0) = 1.
pder(*,1) = sin(x + a(2))
pder(*,2) = a(1)*cos(x + a(2))
return
end


function sinogram, input, angles, $
                    acc_values = acc_values, $
                    air_values = air_values, $
                    backlash = backlash, $
                    auto_center = auto_center, $
                    center = center, $
                    cog = cog, $
                    fluorescence = fluorescence, $
                    debug = debug

;+
; NAME:
;   SINOGRAM
;
; PURPOSE:
;   To convert raw tomography data into a sinogram.
;
; CATEGORY:
;   Tomography
;
; CALLING SEQUENCE:
;   result = SINOGRAM(Input, Angles)
;
; INPUTS:
;   Input
;       An array of raw tomography data. INPUT(I, J) is the intensity at
;       position I for view angle J. Each row is assumed to contain at least
;       one air value at each end for normalization.
;   Angles
;       An array of the angles of each row of the input.  Units are degrees.
;
; KEYWORD PARAMETERS:
;   ACC_VALUES=acc_values
;       The number of values to be discarded at the beginning and end of each
;       row. These values are typically discarded because the stage was in its
;       acceleration/deceleration phase or because there are simply an
;       unecessarily large number of air values at the ends of each row.
;       The default value is 0.
;   AIR_VALUES=air_values
;       The number of air values to be averaged together at the beginning and
;       and of each row, after discarding the ACC_VALUES. This averaging is
;       done to decrease the statistical uncertainty in the air values.
;       The default value is 10.
;   COG=cog
;       This keyword is used to return the measured and fitted
;       center-of-gravity data for the sinogram. The center-of-gravity data are
;       very useful for diagnosing problems such as backlash, beam hardening,
;       detector saturation, etc. COG is dimensioned (n_angles, 2), where
;       n_angles is the number of angles in the input array. COG(*,0) is the
;       measured center-of-gravity data. COG(*,1) is the fitted data. The
;       following command can then be given after the SINOGRAM command
;       IDL> PLOT, COG(*,0)
;       IDL> OPLOT, COG(*,1)
;       to see is the sinogram data are reasonable.
;   /AUTO_CENTER
;       Used to specify that the rotation axis is to be automatically
;       determined by fitting the center of gravity data for the sinogram.
;       This operation is used to correct for the fact that the rotation axis 
;       may not have been perfectly centered when the data were collected.
;       The default is not to auto center the rotation axis.
;   CENTER=center
;       If /AUTO_CENTER is specified then CENTER is an output value containing
;       the calculated column number of the rotation axis.  If /AUTO_CENTER is 
;       not specified then CENTER is an input value containing the column
;       number of the rotation axis.  The units of CENTER are columns in the
;       input image, i.e. before additional air values are added to shift the
;       rotation axis position.  If /AUTOCENTER is not specified and CENTER < 0 
;       then centering is not done, i.e. it is treated as if the CENTER keyword was
;       absent.
;   /BACKLASH
;       Used to specify that even rows of the image are to be shifted left or
;       right so that the fitted rotation axis of the even rows is the same
;       as that for the odd rows. This is used to correct for backlash on
;       images done with first generation CT scans, when the scanning is
;       done bidirectionally.
;       The default is not to correct backlash.
;   /DEBUG
;       Used to turn on debugging output.  The default is not to print 
;       debugging output.
;
;   /FLUORESCENCE
;       Used to indicate that the data are fluoresence data rather than
;       absorption data.  For fluoresence data the data are not normalized to
;       air on each side of the image, and the logarithm is not taken.
;
; RETURN:
;       The output array containing the corrected sinogram. It is always of
;       type FLOAT.
;
; PROCEDURE:
;       This routine creates a sinogram from raw tomography data. It does the
;       following:
;       -   Converts to an odd number of columns (if necessary) by discarding 
;           the last column
;       -   Discards unwanted or uneeded pixels from the left and right edges 
;           of the image. These could be values collected during motor 
;           acceleration or extra air values which will simply slow down the 
;           reconstruction.
;           "ACC_VALUES" pixels are discarded from both the left and right 
;           edges of the array.
;       -   Averages the air values for "air_values" pixels on the left and 
;           right hand sides of the input.
;       -   Logarithmation. output = -log(input/air). The air values are
;           interpolated between the averaged values for the left and right 
;           hand edges of the image for each row.  This step is not performed
;           if the /FLUORESCENCE keyword is set.
;       -   Backlash correction (optional) If /BACKLASH is specified then 
;           motor backlash is corrected for. This is done by fitting the
;           center-of-gravity separately for the even and odd rows of the 
;           image and then shifting the even rows so the the rotation axes are 
;           the same. 
;           The measured center-of-gravity is fitted to a sinusoidal curve
;           of the form Y = A(0) + A(1)*SIN(X + A(2)).
;               A(0) is the rotation axis
;               A(1) is the amplitude
;               A(2) is the phase
;           The fitting is done using routine CURVE_FIT in the User Library.
;           The shifting is done using routine POLY_2D which can shift by 
;           fractional pixels.
;       -   Centering of the rotation axis (optional). If /AUTO_CENTER 
;           is specified then the image is shifted so that the rotation axis 
;           obtained by fitting the center-of-gravity data for all rows in the
;           image coincides exactly with the center column.  If CENTER is 
;           specified without /AUTO_CENTER then the image is shifted by this 
;           value. If CENTER is specified with /AUTO_CENTER then the CENTER
;           value is an output containing the actual position of the rotation
;           axis.
;           The shifting is actually done by adding extra air pixels to the 
;           sinogram so that the rotation axis is in the center of the 
;           enlarged sinogram array.
; MODIFICATION HISTORY:
;   Created 21-OCT-1991 by Mark Rivers.
;   MLR 25-NOV-1994:
;       Removed BSIF_COMMON
;       Made TOMO_HEADER a structure passed to the routine
;       This structure must contain the fields .ANGLE, .WIDTH, .STEP_SIZE
;   MLR 13-MAY-1998
;       Converted from a procedure to a function, added ANGLES parameter, 
;       removed TOMO_HEADER parameter.  Added TWEAK_CENTER and DEBUG keywords.
;   MLR 9-MAR-1999
;       Added some more DEBUG output.
;       Changed the CENTER keyword to AUTO_CENTER, made TWEAK_CENTER work even
;       if /AUTO_CENTER is not used.
;   MLR 9-APR-1999
;       Changed the way shifting is done.  Previously it was done with POLY2D
;       but this could shift the object out of the field of view.  Changed the
;       logic so the shifting is now done by adding extra air pixels on one
;       side of the sinogram.  This can't do fractional pixels, but it does
;       fix the problem of shifting out of the window.
;   MLR 9-APR-1999
;       Removed the TWEAK_CENTER keyword, replaced with the CENTER keyword.
;   MLR 18-MAY-1999
;       Fixed bug when CENTER was specified, but no shift was required
;   MLR 23-JUN-1999
;       Added FLUORESCENCE keyword
;   MLR 22-JAN-2001
;       Changed logic so that if CENTER<0 it is ignorred
;   MLR 19-APR-2001
;       Changed units of ANGLES from radians to degrees
;-

if n_elements(air_values) eq 0 then air_values = 10
if n_elements(acc_values) eq 0 then acc_values = 0

ncols = n_elements(input(*,0))
nrows = n_elements(input(0,*))
; If there are an even number of columns in the image throw out the last one
if (ncols mod 2) eq 0 then ncols=ncols-1

; Throw out the acceleration values, convert data to floating point
output = float(input(acc_values:ncols-acc_values-1, *))
ncols = n_elements(output(*,0))

cog = fltarr(nrows)                    ; Center-of-gravity array
linear = findgen(ncols) / (ncols-1)
no_air = fltarr(ncols) + 1e4
lin2 = findgen(ncols) + 1.
weight = fltarr(nrows) + 1.
for i=0, nrows-1 do begin
    if (air_values gt 0) then begin
       air_left = total(output(0:air_values-1,i)) / air_values
       air_right = total(output(ncols-air_values:ncols-1,i)) / air_values
       air = air_left + linear*(air_right-air_left)
    endif else begin
       air = no_air
    endelse
    if (not keyword_set(fluorescence)) then $
        output(0,i) = -alog(output(*,i)/air > 1.e-5)
    cog(i) = total(output(*,i) * lin2) / total(output(*,i))
endfor
odds = where((indgen(nrows) mod 2) ne 0)
evens = where((indgen(nrows) mod 2) eq 0)
x = angles*!dtor
a = [(ncols-1)/2., $             ; Initial estimate of rotation axis
        (max(cog) - min(cog))/2., $ ; Initial estimate of amplitude
         0.]                        ; Initial estimate of phase
cog_fit = curvefit(x(odds), cog(odds), weight(odds), a, sigmaa, $
                                        function_name='sine_wave')
cog_odd = a(0) - 1.
if (keyword_set(debug)) then print, format='(a, f8.2, a, f8.2)', $
  'Fitted center of gravity for odd rows  = ', cog_odd, ' +-', sigmaa(0)
cog_fit = curvefit(x(evens), cog(evens), weight(evens), a, sigmaa, $
                                        function_name='sine_wave')
cog_even = a(0) - 1.
if (keyword_set(debug)) then print, format='(a, f8.2, a, f8.2)', $
        'Fitted center of gravity for even rows  = ', cog_even, ' +-', sigmaa(0)
if (n_elements(backlash) ne 0) then begin
    back = cog_even - cog_odd
    if (keyword_set(debug)) then print, format='(a,f8.2)', $
        'Backlash (even rows shifted right relative to odd rows) = ', back
    P = [[back, 0.],[1., 0.]]
    Q = [[0., 1.],[0., 0.]]
    temp = poly_2d(output(*, evens), P, Q, 1)
    for i=0, nrows-1, 2 do begin
        output(0, i) = temp(*, i/2)
        cog(i) = total(output(*,i) * lin2) / total(output(*,i))
    endfor
endif
cog_fit = curvefit(x, cog, weight, a, sigmaa, $
                                        function_name='sine_wave')
cog_mean = a(0) - 1.
error_before = cog_mean - (ncols-1)/2.

shift_amount = 0.
if (keyword_set(debug)) then print, format='(a, f8.2, a, f8.2)', $
        'Fitted center of gravity = ', cog_mean, ' +-', sigmaa(0)
if (keyword_set(debug)) then print, format='(a,f8.2)', $
                'Error before correction (offset from center of image) = ', $
                error_before
do_shift = 0
if (keyword_set(auto_center)) then do_shift=1
if (n_elements(center) ne 0) then begin
   if (center ge 0) then do_shift=1
endif
if (do_shift) then begin
    if keyword_set(auto_center) then center = cog_mean
    shift_amount = round(center - (ncols-1)/2.)
    npad = 2 * abs(shift_amount)
    if (air_values gt 0) then begin
        pad_values = air_values 
    endif else begin
        pad_values = 1
    endelse
    if (shift_amount lt 0) then begin
        pad_left = fltarr(npad, nrows)
        temp = total(output(0:pad_values-1,*),1) / pad_values
        for i=0, npad-1 do begin
            pad_left[i,*] = temp
        endfor
        output = [pad_left, output]
        ncols = n_elements(output(*,0))
    endif else if (shift_amount gt 0) then begin
        pad_right = fltarr(npad, nrows)
        temp = total(output(ncols-pad_values:ncols-1,*),1) / pad_values
        for i=0, npad-1 do begin
            pad_right[i,*] =  temp
        endfor
        output = [output, pad_right]
        ncols = n_elements(output(*,0))
    endif
    for i=0, nrows-1 do begin
        cog(i) = total(output(*,i) * lin2) / total(output(*,i))
    endfor
    cog_fit = curvefit(x, cog, weight, a, sigmaa, $
                                                function_name='sine_wave')
    cog_mean = a(0) - 1.
    error_after = cog_mean - (ncols-1)/2.
    if (keyword_set(debug)) then print, format='(a, f8.2, a, f8.2)', $
        'Fitted center of gravity after correction= ', cog_mean, $
                ' +-', sigmaa(0)
    if (keyword_set(debug)) then print, format='(a,f8.2)', $
        'Error after correction (offset from center of image) = ', $
                    error_after
endif

cog = [[cog], [cog_fit]]

if (keyword_set(debug)) then print, $
    "Sinogram used average of "+string(air_values)+" pixels for air"
if (keyword_set(debug)) then print, $
    "Skipped "+string(acc_values)+" acceleration pixels"
if (n_elements(backlash) ne 0) then begin
        if (keyword_set(debug)) then print, $
            "Backlash corrected "+string(back)+" pixels"
endif
if (keyword_set(auto_center) and keyword_set(debug)) then begin
        print, "Center corrected "+string(shift_amount)+" pixels"+$
        " Absolute center = " + string(center) +  " pixels"
endif

return, output
end
