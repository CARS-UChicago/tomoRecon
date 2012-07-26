/* File tomoReconIDL.c
   This file is a thin wrapper layer which is called from IDL
   It calls tomoRecon
   
   Mark Rivers
   July, 2012
*/

#include "tomoRecon.h"

#include <epicsExport.h>

// We use static variables because IDL does not really handle passing pointers (though it can be faked with LongLong type)
// and because IDL variable for tomoParams can disappear, it does not have the required lifetime.
static tomoRecon *pTomoRecon = 0;
static tomoParams_t tomoParams;

extern "C" {
epicsShareFunc void epicsShareAPI tomoReconStartIDL(int argc, char *argv[])
{
    // Make a local copy of tomoParams because the IDL variable could be deleted
    memcpy(&tomoParams, (tomoParams_t *)argv[0], sizeof(tomoParams));
    float *pAngles = (float *)argv[1];
    float *pIn     = (float *)argv[2];
    float *pOut    = (float *)argv[3];
    if (pTomoRecon) delete pTomoRecon;
    pTomoRecon = new tomoRecon(&tomoParams, pAngles, pIn, pOut);
}

epicsShareFunc void epicsShareAPI tomoReconPollIDL(int argc, char *argv[])
{
    int *pReconComplete   = (int *)argv[0];
    int *pSlicesRemaining = (int *)argv[1];
    
    if (pTomoRecon == 0) return;
    pTomoRecon->poll(pReconComplete, pSlicesRemaining);
}

epicsShareFunc void epicsShareAPI tomoReconAbortIDL(int argc, char *argv[])
{
    if (pTomoRecon == 0) return;
    pTomoRecon->abort();
}

} // extern "C"
