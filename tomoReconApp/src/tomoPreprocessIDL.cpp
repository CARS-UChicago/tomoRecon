/* File tomoPreprocessIDL.c
   This file is a thin wrapper layer which is called from IDL
   It calls tomoPreprocess
   
   Mark Rivers
   December, 2022
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tomoPreprocess.h"

#include <epicsExport.h>

// We use static variables because IDL does not really handle passing pointers (though it can be faked with LongLong type)
// and because the IDL variable for preprocessParams can disappear, it does not have the required lifetime.
static tomoPreprocess *pTomoPreprocess = 0;
static preprocessParams_t preprocessParams;

extern "C" {
/** Function to create a tomoPreprocess object from IDL. 
 * \param[in] argc Number of parameters = 1
 * \param[in] argv Array of pointers.<br/>
 *            argv[0] = Pointer to a preprocessParams_t structure, which defines the preprocessing parameters <br/>
 * These arguments are copied to static variables in this file, because the IDL variables could be deleted
 * and returned to the heap while the tomoPreprocess object still exists. */
epicsShareFunc void epicsShareAPI tomoPreprocessCreateIDL(int argc, char *argv[])
{
  preprocessParams_t *pPreprocessParams = (preprocessParams_t *)argv[0];
  
  // Make a local copy of preprocessParams and angles because the IDL variables could be deleted
  memcpy(&preprocessParams, pPreprocessParams, sizeof(preprocessParams));
  if (pTomoPreprocess) delete pTomoPreprocess;
  pTomoPreprocess = new tomoPreprocess(&preprocessParams);
}

/** Function to delete the tomoPreprocess object created with tomoPreprocessCreateIDL.
* Note that any existing tomoPreprocess object is automatically deleted the next time
* that tomoPreprocessCreateIDL is called, so it is often not necessary to call this function. */
epicsShareFunc void epicsShareAPI tomoPreprocessDeleteIDL(int argc, char *argv[])
{
  if (pTomoPreprocess == 0) return;
  delete pTomoPreprocess;
  pTomoPreprocess = 0;
}

/** Function to run preprocessing using the tomoPreprocess object created with tomoPreprocessCreateIDL.
 * \param[in] argc Number of parameters = 5
 * \param[in] argv Array of pointers. <br/>
 *            argv[0] = Pointer to a number of projections to preprocess <br/>
 *            argv[1] = Pointer to float array of dark field <br/>
 *            argv[2] = Pointer to float array of flat field <br/>
 *            argv[3] = Pointer to unsigned 16-bit array of input projections [numPixels, numSlices, numProjections] <br/>
 *            argv[4] = Pointer to float array of normalized output [numPixels, numSlices, numProjections]
 */
epicsShareFunc void epicsShareAPI tomoPreprocessRunIDL(int argc, char *argv[])
{
  int *numProjections =   (int *)argv[0];
  float *pDark   = (float *)argv[1];
  float *pFlat   = (float *)argv[1];
  epicsUInt16 *pIn = (epicsUInt16 *)argv[2];
  float *pOut    = (float *)argv[3];

  if (pTomoPreprocess == 0) return;
  pTomoPreprocess->preprocess(*numProjections, pDark, pFlat, pIn, pOut);
}

/** Function to poll the status of a preprocessing started with tomoPreprocessRunIDL.
 * \param[in] argc Number of parameters = 2
 * \param[in] argv Array of pointers. <br/>
 *            argv[0] = Pointer to int preprocessComplete; 0 if preprocessing still running, 1 if complete <br/>
 *            argv[1] = Pointer to int projectionsRemaining, which gives the number of projections remaining to be processed. */
epicsShareFunc void epicsShareAPI tomoPreprocessPollIDL(int argc, char *argv[])
{
  int *pPreprocessComplete   = (int *)argv[0];
  int *pProjectionsRemaining = (int *)argv[1];

  if (pTomoPreprocess == 0) return;
  pTomoPreprocess->poll(pPreprocessComplete, pProjectionsRemaining);
}

} // extern "C"
