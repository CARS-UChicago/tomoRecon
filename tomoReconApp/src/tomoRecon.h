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

typedef struct {
  int sliceNumber;
  float center;
  float *pIn1;
  float *pIn2;
  float *pOut1;
  float *pOut2;
} toDoMessage_t;

typedef struct {
  int sliceNumber;
  int numSlices;
  double sinogramTime;
  double reconTime;
} doneMessage_t;

typedef struct {
  int numThreads;
  int numPixels;
  int numSlices;
  int numProjections;
  int paddedSinogramWidth;
  float centerOffset;
  float centerSlope;
  int airPixels;
  int ringWidth;
  int fluorescence;
  int debug;
  char debugFileName[256];
  // These are gridRec parameters
  int geom;		      /* 0 if array of angles provided;
				             * 1,2 if uniform in half,full circle */ 
  float pswfParam;	/* PSWF parameter */
  float sampl;	  	/* "Oversampling" ratio */
  float MaxPixSiz; 	/* Max pixel size for reconstruction */
  float R;		      /* Region of interest (ROI) relative size */
  float X0;		      /* (X0,Y0)=Offset of ROI from rotation axis, */
  float Y0;		      /* in units of center-to-edge distance.  */
  char fname[16];	  /* Name of filter function   */		
  int ltbl;		      /* No. elements in convolvent lookup tables. */
} tomoParams_t;

#ifdef __cplusplus
typedef struct {
  class tomoRecon *pTomoRecon;
  int taskNum;
} workerCreateStruct;

class tomoRecon {
public:
  tomoRecon(tomoParams_t *pTomoParams, float *pAngles);
  ~tomoRecon();
  virtual int reconstruct(float *pInput, float *pOutput);
  virtual void supervisorTask();
  virtual void workerTask(int taskNum);
  virtual void sinogram(float *pIn, float *pOut);
  virtual void poll(int *pReconComplete, int *pSlicesRemaining);
  virtual void abort();
  virtual void logMsg(const char *pFormat, ...);

private:
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
  epicsMutexId logMsgMutex_;
};
#endif
