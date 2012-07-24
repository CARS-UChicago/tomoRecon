/*
 * tomoRecon.cpp
 * 
 * C++ class for doing computed tomographdeby reconstruction using Gridrec.
 *
 * This runs the reconstruction using multiple threads, each thread reconstructing a set of slices.
 *
 * It uses the EPICS libCom library for OS-independent functions for threads, mutexes, message queues, etc.
 *
 * Author: Mark Rivers
 *
 * Created: July 9, 2012
 */
#include <stdlib.h>
#include <math.h>

#include <epicsTime.h>

#include "tomoRecon.h"

static tomoSupervisor *pTomoSupervisor = NULL;
static int reconComplete;
static int slicesRemaining;
static int tomoDebug;
static FILE *debugFile;
static int tomoAbort;

extern "C" {
int tomoReconStart(tomoParams_t *pTomoParams, float *pAngles, float *pInput, float *pOutput)
{
  if (pTomoSupervisor) {
    fprintf(debugFile, "tomoReconStart: tomoSupervisor object already exists\r\n");
    fflush(debugFile);
    return -1;
  }
  pTomoSupervisor = new tomoSupervisor(pTomoParams, pAngles, pInput, pOutput);
  return 0;
}

int tomoReconPoll(int *pReconComplete, int *pSlicesRemaining)
{
  *pReconComplete = reconComplete;
  *pSlicesRemaining = slicesRemaining;
  return 0;
}

int tomoReconAbort()
{
  if (!pTomoSupervisor) {
    fprintf(debugFile, "tomoReconAbort: tomoSupervisor object does not exist\r\n"); 
    fflush(debugFile);
    return -1;
  }
  pTomoSupervisor->abort();
  return 0;
}

static void supervisorTask(void *drvPvt)
{
  tomoSupervisor *pTomoSupervisor = (tomoSupervisor *)drvPvt;
  pTomoSupervisor->supervisorTask();
}

static void workerTask(void *drvPvt)
{
  tomoWorker *pTomoWorker = (tomoWorker *)drvPvt;
  pTomoWorker->workerTask();
}

}  // extern "C"

/** Constructor for the tomoSupervisor class.
* \param[in] pTomoParams A structure containing the tomography reconstruction parameters
* \param[in] pTomoParams Pointer to angles
* \param[in] pInput Pointer to input data
* \param[out] pOutput Pointer to output data */
tomoSupervisor::tomoSupervisor(tomoParams_t *pTomoParams, float *pAngles, float *pInput, float *pOutput)
  : 
    pTomoParams_(pTomoParams),
    numPixels_(pTomoParams_->numPixels),
    numSlices_(pTomoParams_->numSlices),
    numProjections_(pTomoParams_->numProjections),
    paddedWidth_(pTomoParams_->paddedSinogramWidth),
    numThreads_(pTomoParams_->numThreads),
    pAngles_(pAngles),
    pInput_(pInput),
    pOutput_(pOutput)

{
  epicsThreadId supervisorTaskId;
  float *pIn=pInput_, *pOut=pOutput_;
  toDoMessage_t toDoMessage;
  int reconSize = numPixels_ * numPixels_;
  int nextSlice=0;
  int i;
  int status;
  
  debugFile = stdout;
  #ifdef _WIN32
  if (debug_) {
    debugFile = fopen("tomoReconDebug.out", "w");
  }
  #endif
  
  // Set static variables that need to outlive the object
  tomoAbort = 0;
  tomoDebug = pTomoParams_->debug;
  pTomoSupervisor = this;
  reconComplete = 0;
  slicesRemaining = pTomoParams_->numSlices;
  
  queueElements_ = (pTomoParams_->numSlices+1)/2;
  toDoQueue_ = epicsMessageQueueCreate(queueElements_, sizeof(toDoMessage_t));
  doneQueue_ = epicsMessageQueueCreate(queueElements_, sizeof(doneMessage_t));
  workerDoneEventIds_ = (epicsEventId *) malloc(numThreads_ * sizeof(epicsEventId));
  fftwMutexId_ = epicsMutexCreate();

  // Fill up the toDoQueue with slices to be reconstructed
  for (i=0; i<queueElements_; i++) {
    toDoMessage.sliceNumber = nextSlice;
    toDoMessage.pIn1 = pIn;
    toDoMessage.pOut1 = pOut;
    pIn += numPixels_;
    pOut += reconSize;
    nextSlice++;
    if (nextSlice < pTomoParams_->numSlices) {
      toDoMessage.pIn2 = pIn;
      toDoMessage.pOut2 = pOut;
      toDoMessage.center = pTomoParams_->centerOffset + i*pTomoParams_->centerSlope;
      pIn += numPixels_;
      pOut += reconSize;
      nextSlice++;
    } else {
      toDoMessage.pIn2 = NULL;
      toDoMessage.pOut2 = NULL;
    }
    status = epicsMessageQueueTrySend(toDoQueue_, &toDoMessage, sizeof(toDoMessage));
    if (status) {
      fprintf(debugFile, "tomoRecon::supervisorTask, error calling epicsMessageQueueTrySend, status=%d\r\n", status);
      fflush(debugFile);
    }
  }

  /* Create the thread for the supervisor task */
  supervisorTaskId = epicsThreadCreate("supervisorTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC) ::supervisorTask,
                                this);
  if (supervisorTaskId == 0) {
    fprintf(debugFile, "tomoRecon::tomoRecon epicsThreadCreate failure for supervisorTask\r\n"); 
    fflush(debugFile);
    return;
  }
}

/** Destructor for the tomoSupervisor class. */
tomoSupervisor::~tomoSupervisor() 
{
  int i;
  
  pTomoSupervisor = NULL;  
  epicsMessageQueueDestroy(toDoQueue_);
  epicsMessageQueueDestroy(doneQueue_);
  for (i=0; i<numThreads_; i++) epicsEventDestroy(workerDoneEventIds_[i]);
  epicsMutexDestroy(fftwMutexId_);
  free(workerDoneEventIds_);
  if (debugFile != stdout) fclose(debugFile);
}

void tomoSupervisor::abort()
{
  // Set the abort flag, which will stop the supervisorTask and workerTask threads
  tomoAbort = 1;
}

/** Supervisor control task that runs as a separate thread. */
void tomoSupervisor::supervisorTask(void)
{
  int i;
  int status;
  doneMessage_t doneMessage;

  /* Create the worker objects */
  for (i=0; i<numThreads_; i++) {
    workerDoneEventIds_[i] = epicsEventCreate(epicsEventEmpty);
    new tomoWorker(this, i, workerDoneEventIds_[i]);
  }
  
  // Wait for all the messages to come back, indicating that reconstruction is complete
  while (slicesRemaining > 0) {
    if (tomoAbort) break;
    status = epicsMessageQueueReceiveWithTimeout(doneQueue_, &doneMessage, sizeof(doneMessage), 1.0);
    if (status != sizeof(doneMessage)) {
      fprintf(debugFile, "tomoRecon::supervisorTask, timeout waiting for worker thread message\r\n");
      fflush(debugFile);
      continue;
    }
    slicesRemaining -= doneMessage.numSlices;
  }
  reconComplete = 1;
  // Wait for the worker threads to exit before deleting the message queues, etc.
  // They will exit because the toDoQueue is now empty
  for (i=0; i<numThreads_; i++) {
    status = epicsEventWaitWithTimeout(workerDoneEventIds_[i], 1.0);
    if (status != epicsEventWaitOK) {
      fprintf(debugFile, "Error waiting for worker task %d to complete ...\r\n", i);
      fflush(debugFile);
    }
  }
      
  delete this;
}



/** Constructor for the tomoWorker class.
* \param[in] pTomoSupervisor - pointer to tomoSupervisor object
* \param[in] taskNum - index number of this worker task
*/
  tomoWorker::tomoWorker(tomoSupervisor *pTomoSupervisor, int taskNum, epicsEventId doneEventId)
  : 
    pTS_(pTomoSupervisor),
    taskNum_(taskNum),
    doneEventId_(doneEventId)
{
  epicsThreadId workerTaskId;
  char workerTaskName[20];
  static const char *functionName = "tomoWorker::tomoWorker";
  
  sprintf(workerTaskName, "workerTask%d", taskNum_+1);
  workerTaskId = epicsThreadCreate(workerTaskName,
                     epicsThreadPriorityMedium,
                     epicsThreadGetStackSize(epicsThreadStackMedium),
                     (EPICSTHREADFUNC) ::workerTask,
                     this);
  if (workerTaskId == 0) {
    fprintf(debugFile, "%s epicsThreadCreate failure for workerTask %d\r\n", functionName, taskNum_+1);
    fflush(debugFile);
    return;
  }
}

/** Destructor for the tomoWorker class. */
tomoWorker::~tomoWorker() 
{
}


/** Worker task that runs as a separate thread. Multiple worker tasks can be running simultaneously.
 * Each worker task reconstructs slices that it get from the toDoQueue. */
void tomoWorker::workerTask(void)
{
  toDoMessage_t toDoMessage;
  doneMessage_t doneMessage;
  epicsTimeStamp tStart, tStop;
  long imageSize;
  int numProjections = pTS_->numProjections_;
  int paddedWidth = pTS_->paddedWidth_;
  int numPixels = pTS_->numPixels_;
  int status;
  float *pOut;
  int i;
  int sinOffset;
  float *sin1, *sin2, *air, *recon1, *recon2, *pRecon;
  sg_struct sgStruct;
  grid_struct gridStruct;
  float **S1, **S2, **R1, **R2;
  grid *pGrid;
  
  sgStruct.n_ang    = numProjections;
  sgStruct.n_det    = paddedWidth;
  sgStruct.geom     = pTS_->pTomoParams_->geom;
  sgStruct.angles   = pTS_->pAngles_;
  sgStruct.center   = pTS_->pTomoParams_->centerOffset;
  get_pswf(pTS_->pTomoParams_->pswfParam, &gridStruct.pswf);
  gridStruct.sampl     = pTS_->pTomoParams_->sampl;
  gridStruct.R         = pTS_->pTomoParams_->R;
  gridStruct.MaxPixSiz = pTS_->pTomoParams_->MaxPixSiz;
  gridStruct.X0        = pTS_->pTomoParams_->X0;
  gridStruct.Y0        = pTS_->pTomoParams_->Y0;
  gridStruct.ltbl      = pTS_->pTomoParams_->ltbl;
  gridStruct.filter    = get_filter(pTS_->pTomoParams_->fname);
  gridStruct.verbose   = tomoDebug;

  // Must take a mutex when creating grid object, because it creates fftw plans, which is not thread safe
  epicsMutexLock(pTS_->fftwMutexId_);
  pGrid = new grid(&gridStruct, &sgStruct, &imageSize);
  epicsMutexUnlock(pTS_->fftwMutexId_);

  sinOffset = (imageSize - numPixels)/2;
  if (sinOffset < 0) sinOffset = 0;

  sin1   = (float *) calloc(paddedWidth * numProjections, sizeof(float));
  sin2   = (float *) calloc(paddedWidth * numProjections, sizeof(float));
  air    = (float *) malloc(paddedWidth*sizeof(float));
  recon1 = (float *) calloc(imageSize * imageSize, sizeof(float));
  recon2 = (float *) calloc(imageSize * imageSize, sizeof(float));  
  S1     = (float **) malloc(numProjections * sizeof(float *));
  S2     = (float **) malloc(numProjections * sizeof(float *));
  R1     = (float **) malloc(imageSize * sizeof(float *));
  R2     = (float **) malloc(imageSize * sizeof(float *));

  /* We are passed addresses of arrays (float *), while Gridrec
     wants a pointer to a table of the starting address of each row.
     Need to build those tables */
  S1[0] = sin1;
  S2[0] = sin2;
  for (i=1; i<numProjections; i++) {
    S1[i] = S1[i-1] + paddedWidth;
    S2[i] = S2[i-1] + paddedWidth;
  }
  R1[0] = recon1;
  R2[0] = recon2;
  for (i=1; i<imageSize; i++) {
      R1[i] = R1[i-1] + imageSize;
      R2[i] = R2[i-1] + imageSize;
  }

  while(1) {
    status = epicsMessageQueueTryReceive(pTS_->toDoQueue_, &toDoMessage, sizeof(toDoMessage));
    if (status == -1) break;
    if (status != sizeof(toDoMessage)) {
      fprintf(debugFile, "tomoRecon::workerTask, error calling epicsMessageQueueReceive, status=%d\r\n", status);
      fflush(debugFile);
      break;
    }
    epicsTimeGetCurrent(&tStart);
    
    sinogram(toDoMessage.pIn1, sin1, air);
    doneMessage.numSlices = 1;
    if (toDoMessage.pIn2) {
      sinogram(toDoMessage.pIn2, sin2, air);
      doneMessage.numSlices = 2;
    }
    epicsTimeGetCurrent(&tStop);
    doneMessage.sinogramTime = epicsTimeDiffInSeconds(&tStop, &tStart);
    epicsTimeGetCurrent(&tStart);
    pGrid->recon(S1, S2, &R1, &R2);
    // Copy to output array, discard padding
    for (i=0, pOut=toDoMessage.pOut1, pRecon=recon1+sinOffset*imageSize; 
         i<imageSize;
         i++, pOut+=numPixels, pRecon+=imageSize) {
      memcpy(pOut, pRecon+sinOffset, imageSize*sizeof(float));
    }
    if (doneMessage.numSlices == 2) {
      for (i=0, pOut=toDoMessage.pOut2, pRecon=recon2+sinOffset*imageSize; 
           i<imageSize;
           i++, pOut+=numPixels, pRecon+=imageSize) {
      memcpy(pOut, pRecon+sinOffset, imageSize*sizeof(float));
      }
    }
    epicsTimeGetCurrent(&tStop);
    doneMessage.reconTime = epicsTimeDiffInSeconds(&tStop, &tStart);
    doneMessage.sliceNumber = toDoMessage.sliceNumber;
    status = epicsMessageQueueTrySend(pTS_->doneQueue_, &doneMessage, sizeof(doneMessage));
    if (status) {
      printf("tomoRecon::workerTask, error calling epicsMessageQueueTrySend, status=%d\r\n", status);
    }
    if (tomoDebug > 0) { 
      fprintf(debugFile, "tomoRecon::workerTask, thread=%s, slice=%d, sinogram time=%f, recon time=%f\r\n", 
          epicsThreadGetNameSelf(), doneMessage.sliceNumber, 
          doneMessage.sinogramTime, doneMessage.reconTime);
      fflush(debugFile);
    }
    if (tomoAbort) break;
  }
  free(sin1);
  free(sin2);
  free(air);
  free(recon1);
  free(recon2);
  free(S1);
  free(S2);
  free(R1);
  free(R2);
  if (tomoDebug > 0) {
    fprintf(debugFile, "tomoRecon::workerTask %s exiting\r\n", epicsThreadGetNameSelf());
    fflush(debugFile);
  }
  // Send an event so the supervisor knows this thread is done
  epicsEventSignal(doneEventId_);
  delete this;
}

void tomoWorker::sinogram(float *pIn, float *pOut, float *air)
{
  int i, j;
  int numAir = pTS_->pTomoParams_->airPixels;
  int numProjections = pTS_->numProjections_;
  int paddedWidth = pTS_->paddedWidth_;
  int numPixels = pTS_->numPixels_;
  int numSlices = pTS_->numSlices_;
  int sinOffset = (paddedWidth - numPixels)/2;
  float airLeft, airRight, airSlope;
  float *pInData;
  float *pOutData;
  
  for (i=0, pInData=pIn, pOutData=pOut; 
       i<numProjections;
       i++, pInData+=numPixels*numSlices, pOutData+=paddedWidth) {
    if (numAir > 0) {
      for (j=0, airLeft=0, airRight=0; j<numAir; j++) {
        airLeft += pInData[j];
        airRight += pInData[numPixels - 1 - j];
      }
      airLeft /= numAir;
      airRight /= numAir;
      airSlope = (airRight - airLeft)/(numPixels - 1);
    }
    else {
      // Assume data has been normalized to air=10000
      airLeft = 1e4;
      airSlope = 0;
    }
    for (j=0; j<numPixels; j++) {
       air[j] = airLeft + airSlope*i;
    }
    if (pTS_->pTomoParams_->fluorescence) {
      for (j=0; j<numPixels; j++) {
        pOutData[sinOffset + j] = pInData[j];
      }
    }
    else {
      for (j=0; j<numPixels; j++) {
        pOutData[sinOffset + j] = -log(pInData[j]/air[j]);
      }
    }
  }
}


