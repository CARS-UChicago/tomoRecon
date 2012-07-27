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
#include <stdarg.h>

#include <epicsTime.h>

#include "tomoRecon.h"


extern "C" {
static void supervisorTask(void *pPvt)
{
  tomoRecon *pTomoRecon = (tomoRecon *)pPvt;
  pTomoRecon->supervisorTask();
  pTomoRecon->logMsg("Exiting supervisor C task.");
}

static void workerTask(workerCreateStruct *pWCS)
{
  pWCS->pTomoRecon->workerTask(pWCS->doneEventId);
  pWCS->pTomoRecon->logMsg("Freeing pWCS %s.", epicsThreadGetNameSelf());
  free(pWCS);
}
} // extern "C"


/** Constructor for the tomoRecon class.
* \param[in] pTomoParams A structure containing the tomography reconstruction parameters
* \param[in] pTomoParams Pointer to angles
* \param[in] pInput Pointer to input data
* \param[out] pOutput Pointer to output data */
tomoRecon::tomoRecon(tomoParams_t *pTomoParams, float *pAngles, float *pInput, float *pOutput)
  : reconComplete_(0),
    slicesRemaining_(pTomoParams->numSlices),
    pTomoParams_(pTomoParams),
    numPixels_(pTomoParams_->numPixels),
    numSlices_(pTomoParams_->numSlices),
    numProjections_(pTomoParams_->numProjections),
    paddedWidth_(pTomoParams_->paddedSinogramWidth),
    numThreads_(pTomoParams_->numThreads),
    pAngles_(pAngles),
    pInput_(pInput),
    pOutput_(pOutput),
    queueElements_((pTomoParams_->numSlices+1)/2),
    debug_(pTomoParams_->debug),
    shutDown_(0)

{
  epicsThreadId supervisorTaskId;
  float *pIn=pInput_, *pOut=pOutput_;
  toDoMessage_t toDoMessage;
  int reconSize = numPixels_ * numPixels_;
  int nextSlice=0;
  int i;
  int status;
  static const char *functionName="tomoRecon::tomoRecon";

  debugFile_ = stdout;
  #ifdef _WIN32
  if (debug_) {
    debugFile_ = fopen("tomoReconDebug.out", "w");
  }
  #endif
 
  toDoQueue_ = epicsMessageQueueCreate(queueElements_, sizeof(toDoMessage_t));
  doneQueue_ = epicsMessageQueueCreate(queueElements_, sizeof(doneMessage_t));
  workerDoneEventIds_ = (epicsEventId *) malloc(numThreads_ * sizeof(epicsEventId));
  fftwMutexId_ = epicsMutexCreate();
  logMsgMutexId_ = epicsMutexCreate();

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
      logMsg("%s:, error calling epicsMessageQueueTrySend, status=%d", 
          functionName, status);
    }
  }

  /* Create the thread for the supervisor task */
  supervisorTaskId = epicsThreadCreate("supervisorTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC) ::supervisorTask,
                                this);
  if (supervisorTaskId == 0) {
    logMsg("%s: epicsThreadCreate failure for supervisorTask", functionName); 
  }
}

/** Destructor for the tomoRecon class. */
tomoRecon::~tomoRecon() 
{
  int i;
  
  epicsMessageQueueDestroy(toDoQueue_);
  epicsMessageQueueDestroy(doneQueue_);
  for (i=0; i<numThreads_; i++) epicsEventDestroy(workerDoneEventIds_[i]);
  epicsMutexDestroy(fftwMutexId_);
  epicsMutexDestroy(logMsgMutexId_);
  free(workerDoneEventIds_);
  if (debugFile_ != stdout) fclose(debugFile_);
}

void tomoRecon::poll(int *pReconComplete_, int *pSlicesRemaining_)
{
  *pReconComplete_ = reconComplete_;
  *pSlicesRemaining_ = slicesRemaining_;
}

void tomoRecon::abort()
{
  // Set the shutDown flag, which will stop the supervisorTask and workerTask threads
  shutDown_ = 1;
}

/** Supervisor control task that runs as a separate thread. */
void tomoRecon::supervisorTask(void)
{
  int i;
  int status;
  doneMessage_t doneMessage;
  char workerTaskName[20];
  epicsThreadId workerTaskId;
  workerCreateStruct *pWCS;
  static const char *functionName="tomoRecon::supervisorTask";

  /* Create the worker tasks */
  for (i=0; i<numThreads_; i++) {
    workerDoneEventIds_[i] = epicsEventCreate(epicsEventEmpty);
    sprintf(workerTaskName, "workerTask%d", i);
    pWCS = (workerCreateStruct *)malloc(sizeof(workerCreateStruct));
    pWCS->pTomoRecon = this;
    pWCS->doneEventId = workerDoneEventIds_[i];
    workerTaskId = epicsThreadCreate(workerTaskName,
                       epicsThreadPriorityMedium,
                       epicsThreadGetStackSize(epicsThreadStackMedium),
                       (EPICSTHREADFUNC) ::workerTask,
                       pWCS);
    if (workerTaskId == 0) {
      logMsg("%s epicsThreadCreate failure for workerTask %d", functionName, i);
      return;
    }
  }
  
  // Wait for all the messages to come back, indicating that reconstruction is complete
  while (slicesRemaining_ > 0) {
    if (shutDown_) break;
    status = epicsMessageQueueReceive(doneQueue_, &doneMessage, sizeof(doneMessage));
    if (status != sizeof(doneMessage)) {
      logMsg("%s, error reading worker thread message", functionName);
      continue;
    }
    slicesRemaining_ -= doneMessage.numSlices;
  }
  if (debug_) logMsg("%s: All slices complete!", functionName);
  // Wait for the worker threads to exit before setting the reconstruction complete flag.
  // They will exit because the toDoQueue is now empty
  for (i=0; i<numThreads_; i++) {
    if (debug_) logMsg("%s: Beginning wait for worker task %d to complete, eventId=%p", 
          functionName, i, workerDoneEventIds_[i]);
    status = epicsEventWaitWithTimeout(workerDoneEventIds_[i], 1.0);
    if (debug_) logMsg("%s: Done wait for worker task %d to complete, eventId=%p, status=%d", 
          functionName, i, workerDoneEventIds_[i], status);
    if (status != epicsEventWaitOK) {
      logMsg("%s: Error waiting for worker task %d to complete, eventId=%p, status=%d", 
          functionName, i, workerDoneEventIds_[i], status);
    }
  }
  reconComplete_ = 1;
  if (debug_) logMsg("%s: Reconstruction complete! Exiting supervisor task.", functionName);
}

/** Worker task that runs as a separate thread. Multiple worker tasks can be running simultaneously.
 * Each worker task reconstructs slices that it get from the toDoQueue. */
void tomoRecon::workerTask(epicsEventId doneEventId)
{
  toDoMessage_t toDoMessage;
  doneMessage_t doneMessage;
  epicsTimeStamp tStart, tStop;
  long imageSize;
  int status;
  float *pOut;
  int i;
  int sinOffset;
  float *sin1, *sin2, *air, *recon1, *recon2, *pRecon;
  sg_struct sgStruct;
  grid_struct gridStruct;
  float **S1, **S2, **R1, **R2;
  grid *pGrid;
  static const char *functionName="tomoRecon::workerTask";
  
  sgStruct.n_ang    = numProjections_;
  sgStruct.n_det    = paddedWidth_;
  sgStruct.geom     = pTomoParams_->geom;
  sgStruct.angles   = pAngles_;
  sgStruct.center   = pTomoParams_->centerOffset + (paddedWidth_ - numPixels_)/2;
  get_pswf(pTomoParams_->pswfParam, &gridStruct.pswf);
  gridStruct.sampl     = pTomoParams_->sampl;
  gridStruct.R         = pTomoParams_->R;
  gridStruct.MaxPixSiz = pTomoParams_->MaxPixSiz;
  gridStruct.X0        = pTomoParams_->X0;
  gridStruct.Y0        = pTomoParams_->Y0;
  gridStruct.ltbl      = pTomoParams_->ltbl;
  gridStruct.filter    = get_filter(pTomoParams_->fname);
  gridStruct.verbose   = debug_;

  // Must take a mutex when creating grid object, because it creates fftw plans, which is not thread safe
  epicsMutexLock(fftwMutexId_);
  pGrid = new grid(&gridStruct, &sgStruct, &imageSize);
  epicsMutexUnlock(fftwMutexId_);

  sinOffset = (imageSize - numPixels_)/2;
  if (sinOffset < 0) sinOffset = 0;

  sin1   = (float *) calloc(paddedWidth_ * numProjections_, sizeof(float));
  sin2   = (float *) calloc(paddedWidth_ * numProjections_, sizeof(float));
  air    = (float *) malloc(paddedWidth_*sizeof(float));
  recon1 = (float *) calloc(imageSize * imageSize, sizeof(float));
  recon2 = (float *) calloc(imageSize * imageSize, sizeof(float));  
  S1     = (float **) malloc(numProjections_ * sizeof(float *));
  S2     = (float **) malloc(numProjections_ * sizeof(float *));
  R1     = (float **) malloc(imageSize * sizeof(float *));
  R2     = (float **) malloc(imageSize * sizeof(float *));

  /* We are passed addresses of arrays (float *), while Gridrec
     wants a pointer to a table of the starting address of each row.
     Need to build those tables */
  S1[0] = sin1;
  S2[0] = sin2;
  for (i=1; i<numProjections_; i++) {
    S1[i] = S1[i-1] + paddedWidth_;
    S2[i] = S2[i-1] + paddedWidth_;
  }
  R1[0] = recon1;
  R2[0] = recon2;
  for (i=1; i<imageSize; i++) {
      R1[i] = R1[i-1] + imageSize;
      R2[i] = R2[i-1] + imageSize;
  }

  while(1) {
    status = epicsMessageQueueTryReceive(toDoQueue_, &toDoMessage, sizeof(toDoMessage));
    if (status == -1) break;
    if (status != sizeof(toDoMessage)) {
      logMsg("%s:, error calling epicsMessageQueueReceive, status=%d", functionName, status);
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
         i<numPixels_;
         i++, pOut+=numPixels_, pRecon+=imageSize) {
      memcpy(pOut, pRecon+sinOffset, numPixels_*sizeof(float));
    }
    if (doneMessage.numSlices == 2) {
      for (i=0, pOut=toDoMessage.pOut2, pRecon=recon2+sinOffset*imageSize; 
           i<numPixels_;
           i++, pOut+=numPixels_, pRecon+=imageSize) {
        memcpy(pOut, pRecon+sinOffset, numPixels_*sizeof(float));
      }
    }
    epicsTimeGetCurrent(&tStop);
    doneMessage.reconTime = epicsTimeDiffInSeconds(&tStop, &tStart);
    doneMessage.sliceNumber = toDoMessage.sliceNumber;
    status = epicsMessageQueueTrySend(doneQueue_, &doneMessage, sizeof(doneMessage));
    if (status) {
      printf("%s, error calling epicsMessageQueueTrySend, status=%d", functionName, status);
    }
    if (debug_ > 0) { 
      logMsg("%s:, thread=%s, slice=%d, sinogram time=%f, recon time=%f", 
          functionName, epicsThreadGetNameSelf(), doneMessage.sliceNumber, 
          doneMessage.sinogramTime, doneMessage.reconTime);
    }
    if (shutDown_) break;
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
  delete pGrid;
  // Send an event so the supervisor knows this thread is done
  epicsEventSignal(doneEventId);
  if (debug_ > 0) {
    logMsg("tomoRecon::workerTask %s exiting, eventId=%p", 
        epicsThreadGetNameSelf(), doneEventId);
  }
}

void tomoRecon::sinogram(float *pIn, float *pOut, float *air)
{
  int i, j;
  int numAir = pTomoParams_->airPixels;
  int sinOffset = (paddedWidth_ - numPixels_)/2;
  float airLeft, airRight, airSlope, ratio;
  float *pInData;
  float *pOutData;
  
  for (i=0, pInData=pIn, pOutData=pOut; 
       i<numProjections_;
       i++, pInData+=numPixels_*numSlices_, pOutData+=paddedWidth_) {
    if (numAir > 0) {
      for (j=0, airLeft=0, airRight=0; j<numAir; j++) {
        airLeft += pInData[j];
        airRight += pInData[numPixels_ - 1 - j];
      }
      airLeft /= numAir;
      airRight /= numAir;
      if (airLeft <= 0.) airLeft = 1.;
      if (airRight <= 0.) airRight = 1.;
      airSlope = (airRight - airLeft)/(numPixels_ - 1);
    }
    else {
      // Assume data has been normalized to air=10000
      airLeft = 1e4;
      airSlope = 0;
    }
    for (j=0; j<numPixels_; j++) {
       air[j] = airLeft + airSlope*i;
    }
    if (pTomoParams_->fluorescence) {
      for (j=0; j<numPixels_; j++) {
        pOutData[sinOffset + j] = pInData[j];
      }
    }
    else {
      for (j=0; j<numPixels_; j++) {
        ratio = pInData[j]/air[j];
        if (ratio <= 0.) ratio = 1.;
        pOutData[sinOffset + j] = -log(ratio);
      }
    }
  }
}

void tomoRecon::logMsg(const char *pFormat, ...)
{
    va_list     pvar;
    epicsTimeStamp now;
    char nowText[40];

    epicsMutexLock(logMsgMutexId_);
    epicsTimeGetCurrent(&now);
    nowText[0] = 0;
    epicsTimeToStrftime(nowText,sizeof(nowText),
         "%Y/%m/%d %H:%M:%S.%03f",&now);
    fprintf(debugFile_,"%s ",nowText);
    va_start(pvar,pFormat);
    vfprintf(debugFile_, pFormat, pvar);
    va_end(pvar);
    if (debugFile_ == stdout)
        fprintf(debugFile_, "\r\n");
    else
        fprintf(debugFile_, "\n");
    fflush(debugFile_);
    epicsMutexUnlock(logMsgMutexId_);
}

