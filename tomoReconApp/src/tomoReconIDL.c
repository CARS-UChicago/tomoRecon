/* File TomoReconIDL.c
   This file is a thin wrapper layer which is called from IDL
   It calls tomoRecon
   
   Mark Rivers
   July, 29012
*/

#include "tomoRecon.h"

#include <epicsExport.h>


epicsShareFunc void epicsShareAPI tomoReconStartIDL(int argc, char *argv[])
{
    tomoParams_t *pTomoParams = (tomoParams_t *)argv[0];
    float *pAngles = (float *)argv[1];
    float *pIn     = (float *)argv[2];
    float *pOut    = (float *)argv[3];
    
    tomoReconStart(pTomoParams, pAngles, pIn, pOut);
}

epicsShareFunc void epicsShareAPI tomoReconPollIDL(int argc, char *argv[])
{
    int *reconComplete   = (int *)argv[0];
    int *slicesRemaining = (int *)argv[1];
    
    tomoReconPoll(reconComplete, slicesRemaining);
}

epicsShareFunc void epicsShareAPI tomoReconAbortIDL(int argc, char *argv[])
{
    tomoReconAbort();
}

