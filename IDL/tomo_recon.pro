pro tomo_recon_abort
    common tomo_recon_common, tomo_recon_shareable_library
    t = call_external(tomo_recon_shareable_library, 'tomoReconAbortIDL')
end

pro tomo_recon_poll, reconComplete, slicesRemaining
    common tomo_recon_common, tomo_recon_shareable_library
    reconComplete = 0L
    slicesRemaining = 0L
    t = call_external(tomo_recon_shareable_library, 'tomoReconPollIDL', $
                      reconComplete, $
                      slicesRemaining)
end

;+
; NAME:
;   GRIDREC
;
; PURPOSE:
;   Performs tomographic reconstruction using the "gridrec" algorithm written
;   by Bob Marr and Graham Campbell (not sure about authors) at BNL in 1997.
;   The basic algorithm is based on FFTs.  It reconstructs 2 data sets at once, 
;   one in the real part of the FFT and one in the imaginary part.
;
;   This routine is 20-40 times faster than BACKPROJECT, and yields virtually
;   identical reconstructions.
;
;   This file uses CALL_EXTERNAL to call GridrecIDL.c which is a thin
;   wrapper to grid.c, 
;
; CATEGORY:
;   Tomography data processing
;
; CALLING SEQUENCE:
;   GRIDREC, Sinogram1, Sinogram2, Angles, Image1, Image2
;
; INPUTS:
;   Sinogram1:
;       The first input sinogram, dimensions NX x NANGLES.  
;   Sinogram2:
;       The second input sinogram, dimensions NX x NANGLES.  
;   Angles:
;       An array of dimensions NANGLES which contains the angle in degrees of 
;       each projection.
;
; KEYWORD PARAMETERS:
;   CENTER: The column containing the rotation axis.  The default is the center
;           of the sinogram.
;   SAMPL:  The "sampl" parameter used by grid.c.  Meaning not certain.  Default=1.2
;   C:      The "C" parameter used by grid.c.  Meaning not certain.  Default=6.0
;   R:      The "R" parameter used by grid.c.  Meaning not certain.  Default=1.0
;   MAXPIXSIZE: The "MaxPixSize" parameter used by grid.c.  Meaning not certain.  
;               Default=1.0
;   X0:     The "X0" parameter used by grid.c.  Meaning not certain.  Default=0.0
;   Y0:     The "Y0" parameter used by grid.c.  Meaning not certain.  Default=0.0
;   LTBL:   The "ltbl" parameter used by grid.c.  Meaning not certain.  Default=512.
;   FILTER_NAME: The "filter" parameter used by grid.c.  Character string. Default="shepp".
;   GEOM:   The "geom" parameter used by grid.c.
;               0 = The actual angles in degrees are passed to grid.c.  This is the
;                   default.
;               1 = Assume angles go from 0 to 180-delta, evenly spaced
;               1 = Assume angles go from 0 to 360-delta, evenly spaced
;   DEBUG:  Set this flag to enable debugging printing from the C code.
;
; OUTPUTS:
;   Image1:
;       The reconstructed image from Sinogram1.  
;   Image2:
;       The reconstructed image from Sinogram2.  
;
;   Note that the sizes of Image1 and Image2 are controlled by "grid" and will not 
;   be equal NX*NX.  RECONSTRUCT_SLICE uses the IDL routine CONGRID to resize the 
;   images to be NX*NX.
;
; PROCEDURE:
;   This function uses CALL_EXTERNAL to call the shareable library GridrecIDL, 
;   which is written in C.
;
; RESTRICTIONS:
;   GRIDREC locates the GridrecIDL shareable library via the environment variable
;   GRIDREC_SHARE.  This environment variable must be defined and must contain the
;   name (typically including the path) to a valid shareable library or DLL.
;
; EXAMPLE:
;   GRIDREC, s1, s2, angles, image1, image2, center=419
;
; MODIFICATION HISTORY:
;   Written by:     Mark Rivers, March 4, 2000
;   19-APR-2001 MLR Change units of ANGLES from radians to degrees.  Write
;                   documentation header.
;-

pro tomo_recon, input, $
                output, $
                angles = angles, $
                numThreads = numThreads, $
                paddedSinogramWidth=paddedSinogramWidth, $
                centerOffset=centerOffset, $
                centerSlope=centerSlope, $
                airPixels = airPixels, $
                fluorescence = fluorescence, $
                debug = debug, $
                sampl=sampl, $
                pswfParam=pswfParam, $
                R=R, $
                MaxPixSiz=MaxPixSiz, $
                X0=X0, $
                Y0=Y0, $
                ltbl=ltbl, $
                filter_name=filter_name, $
                geom=geom
                          
    ; We use a common block just to store info through calls
    common tomo_recon_common, tomo_recon_shareable_library

    s = size(input, /dimensions)
    numPixels = s[0]
    numSlices = s[1]
    numProjections = s[2]
    if (n_elements(angles) ne 0) then begin
        if (n_elements(angles) ne numProjections) then message, 'Incorrect number of angles'
    endif else begin
        ; Assume evenly spaced angles 0 to 180-angle_step degrees
        angles = findgen(numProjections)/(numProjections) * 180.
    endelse
    angles = float(angles)
    
    output = 0 ; Deallocate any existing array
    output = fltarr(numPixels, numPixels, numSlices, /nozero)
    
    ; Make sure input is a float array
    tname = size(input, /tname)
    if (tname ne 'FLOAT') then begin
        print, systime(0), ' tomoRecon: Converting input to float' 
        input = float(input)
    endif

    if (n_elements(numThreads) eq 0) then numThreads=1 
    if (n_elements(paddedSinogramWidth) eq 0) then paddedSinogramWidth=1024 
    if (n_elements(centerOffset) eq 0) then centerOffset = paddedSinogramWidth/2. 
    if (n_elements(centerSlope) eq 0) then centerSlope = 0. 
    if (n_elements(airPixels) eq 0) then airPixels = 10 
    if (n_elements(ringWidth) eq 0) then ringWidth = 9
    if (n_elements(fluorescence) eq 0) then fluorescence = 0
    if (n_elements(debug) eq 0) then debug = 0
    ; *** Set default Gridrec parameters, may want to reset these based on experience **
    if (n_elements(pswfParam)  eq 0) then pswfParam = 6.0
    if (n_elements(sampl) eq 0) then sampl = 1.0
    if (n_elements(R) eq 0) then R = 1.0
    if (n_elements(MaxPixSiz) eq 0) then MaxPixSiz = 1.0
    if (n_elements(X0) eq 0) then X0 = 0.
    if (n_elements(Y0) eq 0) then Y0 = 0.
    if (n_elements(ltbl) eq 0) then ltbl=512
    if (n_elements(filter_name) eq 0) then filter_name="shepp"
    if (n_elements(geom) eq 0) then geom=0

    tomoParams = {tomo_params}

    tomoParams.numThreads = numThreads
    tomoParams.numPixels = numPixels
    tomoParams.numSlices = numSlices
    tomoParams.numProjections = numProjections
    tomoParams.paddedSinogramWidth = paddedSinogramWidth
    tomoParams.centerOffset = centerOffset
    tomoParams.centerSlope = centerSlope
    tomoParams.airPixels = airPixels
    tomoParams.ringWidth = ringWidth
    tomoParams.fluorescence = fluorescence
    tomoParams.debug = debug
    tomoParams.pswfParam = pswfParam
    tomoParams.sampl = sampl
    tomoParams.R = R
    tomoParams.MaxPixSiz = MaxPixSiz
    tomoParams.X0 = X0
    tomoParams.Y0 = Y0
    tomoParams.ltbl = ltbl
    tomoParams.fname = [byte(filter_name), 0B]
    tomoParams.geom = geom

    if (n_elements(tomo_recond_shareable_library) eq 0) then begin
        tomo_recon_shareable_library = getenv('TOMO_RECON_SHARE')
        if (tomo_recon_shareable_library eq "") then begin
            file = 'tomoReconIDL_' + !version.os + '_' + !version.arch
            if (!version.os eq 'Win32') then file=file+'.dll' else file=file+'.so'
            tomo_recon_shareable_library = file_which(file)
        endif
    endif
    if (tomo_recon_shareable_library eq '') then message, 'tomoRecon shareable library not defined'
    print, systime(0), ' tomo_recon: Calling tomoReconStartIDL C code '
    t = call_external(tomo_recon_shareable_library, 'tomoReconStartIDL', $
                      tomoParams, $
                      angles, $
                      input, $
                      output)
                      
end
