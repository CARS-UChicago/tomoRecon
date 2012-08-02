/*
 * tomoRecon.h
 * 
 * C++ class for doing computed tomography reconstruction using Gridrec.
 *
 * This runs the reconstruction using multiple threads, each thread reconstructing a set of slices.
 *
 * It uses the EPICS libCom library for OS-independent functions for threads, mutexes, message queues, etc.
 *
 * Author: Mark Rivers
 *
 * Created: July 9, 2012
 */

#include <epicsMessageQueue.h>
#include <epicsThread.h>

#include "grid.h"

/** Structure that is passed from the constructor to the workerTasks in the toDoQueue */
typedef struct {
  int sliceNumber;  /**< Slice number of first slice */
  float center;     /**< Rotation center to use for these slices */
  float *pIn1;      /**< Pointer to first input slice */
  float *pIn2;      /**< Pointer to second input slice.  Can be NULL */
  float *pOut1;     /**< Pointer to first output slice */
  float *pOut2;     /**< Pointer to second output slice. Can be NULL */
} toDoMessage_t;

/** Structure that is passed from the workerTask to the supervisorTask in the doneQueue */
typedef struct {
  int sliceNumber;      /**< Slice number of first slice */
  int numSlices;        /**< Number of slices that we reconstructed. 1 or 2. */
  double sinogramTime;  /**< Time required to compute the sinograms */
  double reconTime;     /**< Time required to reconstruct */
} doneMessage_t;

/** Structure that is passed to the constructor to define the reconstruction */
typedef struct {
  int numThreads;           /**< Number of workerTask threads to create */
  int numPixels;            /**< Number of horizontal pixels in the input data */
  int maxSlices;            /**< Maximum number of slices that will be passed to tomoRecon::reconstruct */
  int numProjections;       /**< Number of projection angles in the input data */
  int paddedSinogramWidth;  /**< Number of pixels to pad the sinogram to;  must be power of 2 and >= numPixels */
  int airPixels;            /**< Number of pixels of air on each side of sinogram to use for secondary normalization */
  int ringWidth;            /**< Number of pixels in smoothing kernel when doing ring artifact reduction; 0 disables ring artifact reduction */
  int fluorescence;         /**< Set to 1 if the data are fluorescence data and should not have the log taken when computing sinogram */
  int debug;                /**< Debug output level; 0: only error messages, 1: debugging from tomoRecon, 2: debugging also from grid */
  char debugFileName[256];  /**< Name of file for debugging output;  use 0 length string ("") to send output to stdout */
  // These are gridRec parameters
  int geom;		              /**< 0 if array of angles provided; 1,2 if uniform in half, full circle */ 
  float pswfParam;	        /**< PSWF parameter */
  float sampl;	  	        /**< "Oversampling" ratio */
  float MaxPixSiz; 	        /**< Max pixel size for reconstruction */
  float R;		              /**< Region of interest (ROI) relative size */
  float X0;		              /**< Offset of ROI from rotation axis in units of center-to-edge distance */
  float Y0;		              /**< Offset of ROI from rotation axis in units of center-to-edge distance */
  char fname[16];	          /**< Name of filter function */		
  int ltbl;		              /**< Number of elements in convolvent lookup tables */
} tomoParams_t;

#ifdef __cplusplus
typedef struct {
  class tomoRecon *pTomoRecon;
  int taskNum;
} workerCreateStruct;

/** Class to do tomography reconstruction.
* Creates a supervisorTask that supervises the reconstruction process, and a set of workerTask threads
* that compute the sinograms and do the reconstruction.
* The reconstruction is done with the GridRec code, originally written at Brookhaven National Lab.
* Gridrec was modified to be thread-safe.
* When the class is created it can be used to reconstruct many slices in a single call, and does
* the reconstruction using multiple threads and cores.  The reconstruction function can be called 
* repeatedly to reconstruct more sets of slices.  Once the object is created it is restricted to
* reconstructing with the same set of parameters, with the exception of the rotation center, which
* can be specified on a slice-by-slice basis.  If the reconstruction parameters change (number of X pixels, 
* number of projections, Gridrec parameters, etc.) then the tomoRecon object must be deleted and a
* new one created.
* \param[in] pTomoParams A structure containing the tomography reconstruction parameters
* \param[in] pAngles Array of projection angles in degrees */

class tomoRecon {
public:
  tomoRecon(tomoParams_t *pTomoParams, float *pAngles);
  ~tomoRecon();
  virtual int reconstruct(int numSlices, float *center, float *pInput, float *pOutput);
  virtual void supervisorTask();
  virtual void workerTask(int taskNum);
  virtual void sinogram(float *pIn, float *pOut);
  virtual void poll(int *pReconComplete, int *pSlicesRemaining);
  virtual void logMsg(const char *pFormat, ...);

private:
  void shutDown();
  tomoParams_t *pTomoParams_;
  int numPixels_;
  int numSlices_;
  int numProjections_;
  int paddedWidth_;
  int numThreads_;
  float *pAngles_;
  float *pInput_;
  float *pOutput_;
  int queueElements_;
  int debug_;
  FILE *debugFile_;
  int reconComplete_;
  int slicesRemaining_;
  int shutDown_;
  epicsMessageQueueId toDoQueue_;
  epicsMessageQueueId doneQueue_;
  epicsEventId supervisorWakeEvent_;
  epicsEventId supervisorDoneEvent_;
  epicsEventId *workerWakeEvents_;
  epicsEventId *workerDoneEvents_;
  epicsMutexId fftwMutex_;
};
#endif
