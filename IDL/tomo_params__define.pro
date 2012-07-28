pro tomo_params__define 
  t = {tomo_params, $
    numThreads: 0L, $
    numPixels: 0L, $
    numSlices: 0L, $
    numProjections: 0L, $
    paddedSinogramWidth: 0L, $
    centerOffset: 0., $
    centerSlope: 0., $
    airPixels: 0L, $
    ringWidth: 0L, $
    fluorescence: 0L, $
    debug: 0L, $
    ; These are gridRec parameters
    geom: 0L,           $	; 0 if array of angles provided;
				                  ; 1,2 if uniform in half,full circle
    pswfParam: 0.,      $ ; PSWF parameter
    sampl: 0.,          $ ; "Oversampling" ratio */
    MaxPixSiz: 0.,      $ ; Max pixel size for reconstruction
    R: 0.,              $ ; Region of interest (ROI) relative size
    X0: 0.,             $ ; (X0,Y0)=Offset of ROI from rotation axis
    Y0: 0.,             $ ; in units of center-to-edge distance.
    fname: bytarr(16),  $ ; Name of filter function	
    ltbl: 0L            $ ; No. elements in convolvent lookup tables
  }
end
