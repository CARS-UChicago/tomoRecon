;+
; NAME:
;   TOMO_PARAMS__DEFINE
;
; PURPOSE:
;   Defines a structure which controls tomography reconstruction parameters for tomo_recon.
;   This structure is passed directly to the C++ code in the shareable library.
;
; MODIFICATION HISTORY:
;   Written by:     Mark Rivers, August 1, 2012
;-

pro tomo_params__define 
  t = {tomo_params, $

    ; Sinogram parameters
    numPixels: 0L,           $ ; Number of pixels in sinogram row before padding
    numProjections: 0L,      $ ; Number of angles
    numSlices: 0L,           $ ; Number of slices
    sinoScale: 10000.,       $ ; Scale factor to multiply sinogram when airPixels=0
    reconScale: 1.e6,        $ ; Scale factor to multiple reconstruction
    paddedSinogramWidth: 0L, $ ; Number of pixels in sinogram after padding
    paddingAverage: 0L,      $ ; Number of pixels to average on each side of sinogram to compute padding. 0 pixels pads with 0.0 
    airPixels: 0L,           $ ; Number of pixels of air to average at each end of sinogram row
    ringWidth: 0L,           $ ; Number of pixels to smooth by when removing ring artifacts
    fluorescence: 0L,        $ ; 0=absorption data, 1=fluorescence
    
    ; Reconstruction method
    reconMethod: 0L,         $ ; 0=tomoRecon, 1=Gridrec, 2=Backproject
    reconMethodTomoRecon:   0L, $
    reconMethodGridrec:     1L, $
    reconMethodBackproject: 2L, $
    
    ; tomoRecon parameters
    numThreads: 0L, $
    slicesPerChunk: 0L, $
    debug: 0L, $
    debugFile: bytarr(256), $
    
    ; gridRec/tomoRecon parameters
    geom: 0L,           $; 0 if array of angles provided;
                         ; 1,2 if uniform in half,full circle
    pswfParam: 0.,      $ ; PSWF parameter
    sampl: 0.,          $ ; "Oversampling" ratio
    maxPixSize: 0.,     $ ; Max pixel size for reconstruction
    ROI: 0.,            $ ; Region of interest (ROI) relative size
    X0: 0.,             $ ; (X0,Y0)=Offset of ROI from rotation axis
    Y0: 0.,             $ ; in units of center-to-edge distance.
    ltbl: 0L,           $ ; No. elements in convolvent lookup tables
    GR_filterName: bytarr(16),  $ ; Name of filter function
    
    ; Backproject parameters
    BP_Method: 0L,         $ ; 0=Riemann, 1=Radon
    BP_MethodRiemann: 0L,  $
    BP_MethodRadon:   1L,  $
    BP_filterName: bytarr(16),  $ ; Name of filter function
    BP_filterSize: 0L,          $ ; Length of filter
    RiemannInterpolation: 0L,  $ ; 0=none, 1=bilinear, 2=cubic
    RiemannInterpolationNone:     0L, $
    RiemannInterpolationBilinear: 1L, $
    RiemannInterpolationCubic:    2L, $
    RadonInterpolation: 0L,  $ ; 0=none, 1=linear
    RadonInterpolationNone:     0L, $
    RadonInterpolationLinear:   1L  $
  }
end
