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
/** Function to create a tomoRecon object from IDL. 
 * \param[in] argc Number of parameters = 2
 * \param[in] argv Array of pointers.<br/>
 *            argv[0] = Pointer to a tomoParams_t structure, which defines the reconstruction parameters <br/>
 *            argv[1] = Pointer to float array of angles in degrees <br/>
 * These arguments are copied to static variables in this file, because the IDL variables could be deleted
 * and returned to the heap while the tomoRecon object still exists. */
epicsShareFunc void epicsShareAPI tomoReconCreateIDL(int argc, char *argv[])
{
  tomoParams_t *pTomoParams = (tomoParams_t *)argv[0];
  float *pAngles            =        (float *)argv[1];
  
  // Make a local copy of tomoParams and angles because the IDL variables could be deleted
  memcpy(&tomoParams, pTomoParams, sizeof(tomoParams));
  if (angles) free(angles);
  angles = (float *)malloc(tomoParams.numProjections*sizeof(float));
  memcpy(angles, pAngles, tomoParams.numProjections*sizeof(float));
  if (pTomoRecon) delete pTomoRecon;
  pTomoRecon = new tomoRecon(&tomoParams, angles);
}

/** Function to delete the tomoRecon object created with tomoReconCreateIDL.
* Note that any existing tomoRecon object is automatically deleted the next time
* that tomoReconCreateIDL is called, so it is often not necessary to call this function. */
epicsShareFunc void epicsShareAPI tomoReconDeleteIDL(int argc, char *argv[])
{
  if (pTomoRecon == 0) return;
  delete pTomoRecon;
  if (angles) free(angles);
  angles = 0;
  pTomoRecon = 0;
}

/** Function to run a reconstruction using the tomoRecon object created with tomoReconCreateIDL.
 * \param[in] argc Number of parameters = 4
 * \param[in] argv Array of pointers. <br/>
 *            argv[0] = Pointer to a number of slices to reconstruct <br/>
 *            argv[1] = Pointer to float array of rotation centers in pixels <br/>
 *            argv[2] = Pointer to float array of input slices [numPixels, numSlices, numProjections] <br/>
 *            argv[3] = Pointer to float array of output reconstructed slices [numPixels, numPixels, numSlices]
 */
epicsShareFunc void epicsShareAPI tomoReconRunIDL(int argc, char *argv[])
{
  int *numSlices =   (int *)argv[0];
  float *pCenter = (float *)argv[1];
  char *pIn      =  (char *)argv[2];
  char *pOut     =  (char *)argv[3];

  if (pTomoRecon == 0) return;
  pTomoRecon->reconstruct(*numSlices, pCenter, pIn, pOut);
}

/** Function to poll the status of a reconstruction started with tomoReconRunIDL.
 * \param[in] argc Number of parameters = 2
 * \param[in] argv Array of pointers. <br/>
 *            argv[0] = Pointer to int reconComplete; 0 if reconstruction still running, 1 if complete <br/>
 *            argv[1] = Pointer to int slicesRemaining, which gives the number of slices remaining to be reconstructed. */
epicsShareFunc void epicsShareAPI tomoReconPollIDL(int argc, char *argv[])
{
  int *pReconComplete   = (int *)argv[0];
  int *pSlicesRemaining = (int *)argv[1];

  if (pTomoRecon == 0) return;
  pTomoRecon->poll(pReconComplete, pSlicesRemaining);
}

} // extern "C"
