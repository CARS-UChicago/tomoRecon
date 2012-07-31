/* File tomoReconIDL.c
   This file is a thin wrapper layer which is called from IDL
   It calls tomoRecon
   
   Mark Rivers
   July, 2012
*/

#include "tomoRecon.h"

#include <epicsExport.h>

// We use static variables because IDL does not really handle passing pointers (though it can be faked with LongLong type)
// and because the IDL variable for tomoParams can disappear, it does not have the required lifetime.
static tomoRecon *pTomoRecon = 0;
static tomoParams_t tomoParams;
static float *angles = 0;

extern "C" {
epicsShareFunc void epicsShareAPI tomoReconCreateIDL(int argc, char *argv[])
{
    // Make a local copy of tomoParams and angles because the IDL variables could be deleted
    memcpy(&tomoParams, (tomoParams_t *)argv[0], sizeof(tomoParams));
    float *pAngles = (float *)argv[1];
    if (angles) free(angles);
    angles = (float *)malloc(tomoParams.numProjections*sizeof(float));
    memcpy(angles, pAngles, tomoParams.numProjections*sizeof(float));
    if (pTomoRecon) delete pTomoRecon;
    pTomoRecon = new tomoRecon(&tomoParams, angles);
}

epicsShareFunc void epicsShareAPI tomoReconDeleteIDL(int argc, char *argv[])
{
    if (pTomoRecon == 0) return;
    delete pTomoRecon;
    if (angles) free(angles);
    pTomoRecon = 0;
}

epicsShareFunc void epicsShareAPI tomoReconRunIDL(int argc, char *argv[])
{
    int *numSlices =   (int *)argv[0];
    float *pCenter = (float *)argv[1];
    float *pIn     = (float *)argv[2];
    float *pOut    = (float *)argv[3];

    if (pTomoRecon == 0) return;
    pTomoRecon->reconstruct(*numSlices, pCenter, pIn, pOut);
}

epicsShareFunc void epicsShareAPI tomoReconPollIDL(int argc, char *argv[])
{
    int *pReconComplete   = (int *)argv[0];
    int *pSlicesRemaining = (int *)argv[1];
    
    if (pTomoRecon == 0) return;
    pTomoRecon->poll(pReconComplete, pSlicesRemaining);
}

} // extern "C"
