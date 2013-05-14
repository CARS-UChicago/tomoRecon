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
;   tomo_recon, tomoParams, Input, Output
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

pro tomo_recon, tomoParams, $
                input, $
                output, $
                create = create, $
                angles = angles, $
                center=center, $
                wait=wait
                          
    ; We use a common block just to store info through calls
    common tomo_recon_common, tomo_recon_shareable_library
    
    t0 = systime(1)

    ; Make the array dimensions in tomoParams agree with actual size
    tomo_params_set_dimensions, tomoParams, input

    if (n_elements(angles) ne 0) then begin
        if (n_elements(angles) ne tomoParams.numProjections) then message, 'Incorrect number of angles'
    endif else begin
        ; Assume evenly spaced angles 0 to 180-angle_step degrees
        angles = findgen(tomoParams.numProjections)/(tomoParams.numProjections) * 180.
    endelse
    angles = float(angles)
    
    if (n_elements(wait) eq 0) then wait = 1
    
    output = 0 ; Deallocate any existing array
    output = fltarr(tomoParams.numPixels, tomoParams.numPixels, tomoParams.numSlices, /nozero)
    
    ; Make sure input is a float array
    tname = size(input, /tname)
    if (tname ne 'FLOAT') then begin
        input = float(input)
    endif
    t1 = systime(1)

    if (n_elements(create) eq 0) then create = 1
    if (n_elements(center) eq 0) then begin
        centerArr = fltarr(tomoParams.numSlices) + (tomoParams.numPixels)/2.
    endif else if (n_elements(center) eq 1) then begin
        centerArr = fltarr(tomoParams.numSlices) + float(center)
    endif else  begin
        if (n_elements(center) lt tomoParams.numSlices) then $
            message, 'Center size ='+ string(n_elements(center)) + ' must be '+ string(tomoParams.numSlices)
        centerArr = float(center)
    endelse    

    locate_tomo_recon_shareable_library
    if (create) then begin
        t = call_external(tomo_recon_shareable_library, 'tomoReconCreateIDL', $
                      tomoParams, $
                      angles)
    endif
    t = call_external(tomo_recon_shareable_library, 'tomoReconRunIDL', $
                      tomoParams.numSlices, $
                      centerArr, $
                      input, $
                      output)

   if (wait) then begin
        repeat begin
            tomo_recon_poll, reconComplete, slicesRemaining
            wait, .01
        endrep until (reconComplete)
        t2 = systime(1)
        print, 'tomo_recon: time to convert to float:', t1-t0
        print, '                 time to reconstruct:', t2-t1
        print, '                          total time:', t2-t0
  endif               
end
