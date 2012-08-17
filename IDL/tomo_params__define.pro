;+
; NAME:
;   TOMO_PARAMS__DEFINE
;
; PURPOSE:
;   Defines a structure which controls tomography reconstruction parameters for tomo_recon.
;   This structure is passed directly to the C++ code in the shareable library.
; The structure is
; long numThreads;           ; Number of workerTask threads to create
; long numPixels;            ; Number of horizontal pixels in the input data
; long maxSlices;            ; Maximum number of slices that will be passed to tomoRecon::reconstruct
; long numProjections;       ; Number of projection angles in the input data
; long paddedSinogramWidth;  ; Number of pixels to pad the sinogram to;  must be power of 2 and >= numPixels
; long airPixels;            ; Number of pixels of air on each side of sinogram to use for secondary normalization
; long ringWidth;            ; Number of pixels in smoothing kernel when doing ring artifact reduction; 0 disables ring artifact reduction
; long fluorescence;         ; Set to 1 if the data are fluorescence data and should not have the log taken when computing sinogram
; long debug;                ; Debug output level; 0: only error messages, 1: debugging from tomoRecon, 2: debugging also from grid
; char debugFile[256];       ; Name of file for debugging output;  use 0 length string ("") to send output to stdout
; These are gridRec parameters
; long geom;                 ; 0 if array of angles provided; 1,2 if uniform in half, full circle 
; float pswfParam;           ; PSWF parameter
; float sampl;               ; "Oversampling" ratio
; float MaxPixSiz;           ; Max pixel size for reconstruction
; float R;                   ; Region of interest (ROI) relative size
; float X0;                  ; Offset of ROI from rotation axis in units of center-to-edge distance
; float Y0;                  ; Offset of ROI from rotation axis in units of center-to-edge distance
; char fname[16];            ; Name of filter function
; long ltbl;                 ; Number of elements in convolvent lookup tables
;
; MODIFICATION HISTORY:
;   Written by:     Mark Rivers, August 1, 2012
;-
pro tomo_params__define 
  t = {tomo_params, $
    numThreads: 0L, $
    numPixels: 0L, $
    maxSlices: 0L, $
    numProjections: 0L, $
    paddedSinogramWidth: 0L, $
    airPixels: 0L, $
    ringWidth: 0L, $
    fluorescence: 0L, $
    debug: 0L, $
    debugFile: bytarr(256), $
    ; These are gridRec parameters
    geom: 0L,           $; 0 if array of angles provided;
                  ; 1,2 if uniform in half,full circle
    pswfParam: 0.,      $ ; PSWF parameter
    sampl: 0.,          $ ; "Oversampling" ratio
    MaxPixSiz: 0.,      $ ; Max pixel size for reconstruction
    R: 0.,              $ ; Region of interest (ROI) relative size
    X0: 0.,             $ ; (X0,Y0)=Offset of ROI from rotation axis
    Y0: 0.,             $ ; in units of center-to-edge distance.
    fname: bytarr(16),  $ ; Name of filter function
    ltbl: 0L            $ ; No. elements in convolvent lookup tables
  }
end
