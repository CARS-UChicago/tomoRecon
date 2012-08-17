;+
; NAME:
;   TOMO_RECON_DELETE
;
; PURPOSE:
;   Deletes the tomoRecon object created by tomo_recon.  This function can be called to explicitly delete the tomoRecon
;   C++ object.  It typically does not need to be called, because object is automatically deleted when tomo_recon is
;   called again with the create=1 (default) option, or when IDL exits.  The overhead of leaving the object in existence is small.
;
;   This file uses CALL_EXTERNAL to call tomoReconIDL.cpp which is a thin
;   wrapper to tomoRecon.cpp. 
;
; CATEGORY:
;   Tomography data processing
;
; CALLING SEQUENCE:
;   tomo_recon_delete
;
; COMMON BLOCKS:
;	  TOMO_RECON_COMMON:
;       This common block is used to hold the name of the shareable library that is called from IDL.	
;
; PROCEDURE:
;   This function uses CALL_EXTERNAL to call the shareable library.
;   libtomoRecon.so (Linux)  or tomoRecon.dll (Windows), which is written in C++.
;
; EXAMPLE:
;   TOMO_RECON_DELETE
;
; MODIFICATION HISTORY:
;   Written by:     Mark Rivers, August 1, 2012
;-
pro tomo_recon_delete
    common tomo_recon_common, tomo_recon_shareable_library

    t = call_external(tomo_recon_shareable_library, 'tomoReconDeleteIDL')
end

;+
; NAME:
;   TOMO_RECON_POLL
;
; PURPOSE:
;   Polls the tomoRecon object created by tomo_recon to read the reconstruction status and the
;   number of slices remaining.
;
;   This file uses CALL_EXTERNAL to call tomoReconIDL.cpp which is a thin
;   wrapper to tomoRecon.cpp. 
;
; CATEGORY:
;   Tomography data processing
;
; CALLING SEQUENCE:
;   tomo_recon_poll, reconComplete, slicesRemaining
;
; OUTPUTS:
;   reconComplete:
;       reconComplete=1 if the reconstruction is complete, 0 if it is not yet complete.
;
;   slicesRemaining
;       slicesRemaining is the number of slices remaining to be reconstructed.
;
; COMMON BLOCKS:
;	  TOMO_RECON_COMMON:
;       This common block is used to hold the name of the shareable library that is called from IDL.	
;
; PROCEDURE:
;   This function uses CALL_EXTERNAL to call the shareable library.
;   libtomoRecon.so (Linux)  or tomoRecon.dll (Windows), which is written in C++.
;
; EXAMPLE:
;   TOMO_RECON_POLL, reconComplete, slicesRemaining
;
; MODIFICATION HISTORY:
;   Written by:     Mark Rivers, August 1, 2012
;-
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
;   TOMO_RECON
;
; PURPOSE:
;   Performs tomographic reconstruction using the tomoRecon object from
;   tomoRecon.cpp and tomoReconIDL.cpp.
;
;   tomoRecon uses the "gridrec" algorithm written
;   by Bob Marr and Graham Campbell (not sure about authors) at BNL in 1997.
;   The basic algorithm is based on FFTs.  

;   This file uses CALL_EXTERNAL to call tomoReconIDL.cpp which is a thin
;   wrapper to tomoRecon.cpp. 
;
; CATEGORY:
;   Tomography data processing
;
; CALLING SEQUENCE:
;   tomo_recon, Input, Output
;
; INPUTS:
;   Input:
;       An array of normalized projections, dimensions [numPixels, numSlices, numProjections].
;       This array will be converted to type FLOAT if it is another data type.
;
; OUTPUTS:
;   Output:
;       A FLOAT array of reconstructed slices, dimensions [numPixels, numPixels, numSlices].
;
; KEYWORD PARAMETERS:
;   CREATE:
;       Set this keyword to 1 to create a new tomoRecon object.  This is the default.
;       Set this keyword to 0 to use an existing tomoRecon object created by a previous
;       call to this function.  Note that all reconstruction parameters except numSlices
;       and CENTER must be the same when using an existing tomoRecon object.
;   ANGLES:
;       An array of dimensions numProjections which contains the angle in degrees of 
;       each projection.  The default is numProjections spaced evenly from 0 to
;       180-angleStep.
;   NUMTHREADS:
;       The number of threads that tomoRecon will use when reconstructing.
;       The default is 8.
;   PADDEDSINOGRAMWIDTH:
;       The number of pixels in the padded sinograms created by tomoRecon. 
;       The default is the smallest power of 2 which is equal or greater than
;       numPixels.  Larger padding can produce slightly more accurate reconstructions
;       at the expense of execution time.
;   CENTER: 
;       The column of Input containing the rotation axis.  This can be specified
;       either as scalar or as a 1-D array of dimension numSlices.  If a scalar
;       is specified then it will be converted to a 1-D array with all values
;       the same.  If a 1-D array is entered then the rotation center can
;       be different for each slice. The default is the center of Input, i.e. numPixels/2.
;   AIRPIXELS:
;       The number of air values to be averaged together at the beginning and
;       and of each row of the sinogram. This is used for secondary normalization in the 
;       tomoRecon::sinogram function.  The air value at each pixel is linearly interpolated 
;       between the average of AIRPIXELS values on the left and right of each row.
;       The averaging is done to decrease the statistical uncertainty in the air values.
;       Set AIRPIXELS=0 to disable this secondary normalization.
;       The default value is 10.
;   RINGWIDTH:
;       The size of the low-pass filtering kernel to be used when smoothing the average
;       sinogram row for ring artifact reduction.  Ring artifact reduction is done by
;       computing the difference between the average row of the sinogram and the low-pass
;       filtered version of the average row.  The difference is used to correct each column
;       of the sinogram.
;       The default is 9.  Set RINGWIDTH=0 to disabled ring artifact reduction.
;   FLUORESCENCE:
;       Used to indicate that the data are fluoresence data rather than
;       absorption data.  For fluoresence data the data are not normalized to
;       air on each side of the image, and the logarithm is not taken.
;   DEBUG:
;       Debug output level; 0=only error messages; 1=debugging from tomoRecon.cpp; 2=debugging also from grid.cpp
;   DGBFILE:
;       Name of a file for debugging output from tomoRecon.cpp.  The default is zero-length string ("") which
;       directs the output to stdout.  Note that on Linux output to stdout can be seen in the command line console window.
;       However, on Windows it is not possible to see the stdout output, even when running the IDL Command Line program.
;       Thus, setting DGBFILE is necessary to obtain debugging output on Windows.      
;   PSWFPARAM:      
;       The pswf (Prolated Spherical Waveform Function) parameter used by grid.c. Default=6.0
;   SAMPL:  
;       The "sampl" parameter used by grid.cpp. Default=1.0
;   R:      
;       The ROI parameter used by grid.c.  Default=1.0
;   MAXPIXSIZE: 
;       The MaxPixSize parameter used by grid.c.  Default=1.0
;   X0:     
;       The X0 ROI parameter used by grid.c.  Default=0.0
;   Y0:     
;       The Y0 ROI parameter used by grid.c.  Default=0.0
;   LTBL:   
;       The lookup table size parameter used by grid.c.  Default=512.
;   FILTER_NAME: 
;       The "filter" parameter used by grid.c.  Character string. Choices are
;       "shepp" (or "shlo"); "hann"; "hamm" (or "hamming"); "ramp" (or "ramlak")
;       Default="shepp".
;   GEOM:   
;       The "geom" parameter used by grid.c.
;               0 = The actual angles in degrees are passed to grid.cpp.  This is the
;                   default.
;               1 = Assume angles go from 0 to 180-delta, evenly spaced
;               1 = Assume angles go from 0 to 360-delta, evenly spaced
;   WAIT:
;       Controls whether this procedure waits for the reconstruction to complete (WAIT=1),
;       or returns immediately to the calling function while the reconstruction continues to run
;       in its own threads (WAIT=0).  Default is WAIT=1.
;
; COMMON BLOCKS:
;	  TOMO_RECON_COMMON:
;       This common block is used to hold the name of the shareable library that is called from IDL.	
;
; PROCEDURE:
;   This function uses CALL_EXTERNAL to call the shareable library.
;   libtomoRecon.so (Linux)  or tomoRecon.dll (Windows), which is written in C++.
;
; RESTRICTIONS:
;   TOMO_RECON locates the tomoRecon IDL shareable library via the environment variable
;   first by seeing if the environment variable TOMO_RECON_SHARE exists.  If it does then this must point to the
;   complete path to the shareable library, e.g. /usr/local/lib/libtomoRecon.so.  
;   If the environment variables does not exist, then it looks for the shareable library in the IDL "path", and the
;   shareable library must be named: 'tomoRecon_' + !version.os + '_' + !version.arch + '.so' or '.dll'
;   For example, tomoRecon_Win32_x86_64.dll or tomoRecon_linux_x86_64.so.  This can be done with soft-links to the
;   actual shareable library file.
;
; EXAMPLE:
;   TOMO_RECON, input, output, center=419
;
; MODIFICATION HISTORY:
;   Written by:     Mark Rivers, August 1, 2012
;-

pro tomo_recon, input, $
                output, $
                create = create, $
                angles = angles, $
                numThreads = numThreads, $
                paddedSinogramWidth=paddedSinogramWidth, $
                center=center, $
                airPixels = airPixels, $
                ringWidth = ringWidth, $
                fluorescence = fluorescence, $
                debug = debug, $
                dbgFile = dbgFile, $
                sampl=sampl, $
                pswfParam=pswfParam, $
                R=R, $
                MaxPixSiz=MaxPixSiz, $
                X0=X0, $
                Y0=Y0, $
                ltbl=ltbl, $
                filter_name=filter_name, $
                geom=geom, $
                wait=wait
                          
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

    if (n_elements(create) eq 0) then create = 1
    if (n_elements(numThreads) eq 0) then numThreads=8
    if (n_elements(paddedSinogramWidth) eq 0) then begin
        ; Use the next largest power of 2 by default
        paddedSinogramWidth = 128
        repeat begin
            paddedSinogramWidth=paddedSinogramWidth * 2
        endrep until (paddedSinogramWidth ge numPixels)
    endif
    if (n_elements(center) eq 0) then begin
        centerArr = fltarr(numSlices) + (numPixels)/2.
    endif else if (n_elements(center) eq 1) then begin
        centerArr = fltarr(numSlices) + float(center)
    endif else  begin
        if (n_elements(center) ne numSlices) then $
            message, 'Center size ='+ string(n_elements(center)) + ' must be '+ string(numSlices)
        centerArr = float(center)
    endelse    
    if (n_elements(airPixels) eq 0) then airPixels = 10 
    if (n_elements(ringWidth) eq 0) then ringWidth = 9
    if (n_elements(fluorescence) eq 0) then fluorescence = 0
    if (n_elements(debug) eq 0) then debug = 0
    if (n_elements(dbgFile) eq 0) then dbgFile = ""
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
    if (n_elements(wait) eq 0) then wait=1

    tomoParams = {tomo_params}

    tomoParams.numThreads = numThreads
    tomoParams.numPixels = numPixels
    tomoParams.maxSlices = numSlices
    tomoParams.numProjections = numProjections
    tomoParams.paddedSinogramWidth = paddedSinogramWidth
    tomoParams.airPixels = airPixels
    tomoParams.ringWidth = ringWidth
    tomoParams.fluorescence = fluorescence
    tomoParams.debug = debug
    tomoParams.debugFile = [byte(dbgFile), 0B]
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
            file = 'tomoRecon_' + !version.os + '_' + !version.arch
            if (!version.os eq 'Win32') then file=file+'.dll' else file=file+'.so'
            tomo_recon_shareable_library = file_which(file)
        endif
    endif
    if (tomo_recon_shareable_library eq '') then message, 'tomoRecon shareable library not defined'
    if (create) then begin
        print, systime(0), ' tomo_recon: Calling tomoReconCreateIDL C code '
        t = call_external(tomo_recon_shareable_library, 'tomoReconCreateIDL', $
                      tomoParams, $
                      angles)
    endif
    print, systime(0), ' tomo_recon: Calling tomoReconRunIDL C code '
    t = call_external(tomo_recon_shareable_library, 'tomoReconRunIDL', $
                      numSlices, $
                      centerArr, $
                      input, $
                      output)

   if (wait) then begin
        repeat begin
            tomo_recon_poll, reconComplete, slicesRemaining
            wait, .01
        endrep until (reconComplete)
   endif
                      
end
