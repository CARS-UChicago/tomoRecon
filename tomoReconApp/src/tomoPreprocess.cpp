/*
 * tomoPreprocess.cpp
 * 
 * C++ class for doing computed tomography preprocessing.
 *
 * This runs the preprocessing using multiple threads, each thread preprocessing a set of projections.
 *
 * It uses the EPICS libCom library for OS-independent functions for threads, mutexes, message queues, etc.
 *
 * Author: Mark Rivers
 *
 * Created: December 31, 2022
 */
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <algorithm>

#include <epicsTime.h>

#include "tomoPreprocess.h"


extern "C" {
static void supervisorTask(void *pPvt)
{
  tomoPreprocess *pTomoPreprocess = (tomoPreprocess *)pPvt;
  pTomoPreprocess->supervisorTask();
}

static void workerTask(workerCreateStruct *pWCS)
{
  pWCS->pTomoPreprocess->workerTask(pWCS->taskNum);
  free(pWCS);
}
} // extern "C"


/** Constructor for the tomoPreprocess class.
* Creates the message queues for passing messages to and from the workerTask threads.
* Creates the thread that execute the supervisorTask function, 
* and numThreads threads that execute the workerTask function.
* \param[in] pPreprocessParams A structure containing the tomography preprocessing parameters
* \param[in] pAngles Array of projection angles in degrees */
tomoPreprocess::tomoPreprocess(preprocessParamsStruct *pPreprocessParams, float *pDark, float *pFlat, epicsUInt16 *pInput, char *pOutput)
  : params_(*pPreprocessParams),
    pDark_(pDark),
    pFlat_(pFlat),
    preprocessComplete_(0),
    projectionsRemaining_(params_.numProjections),
    shutDown_(0)

{
  epicsThreadId supervisorTaskId;
  char workerTaskName[30];
  epicsThreadId workerTaskId;
  workerCreateStruct *pWCS;
  char *debugFileName = params_.debugFileName;
  epicsUInt16 *pIn = pInput;
  char *pOut = pOutput;
  toDoMessageStruct toDoMessage;
  int projectionSize = params_.numPixels * params_.numSlices;
  int status;
  int i;
  static const char *functionName="tomoPreprocess::tomoPreprocess";

  debugFile_ = stdout;
  if ((debugFileName) && (strlen(debugFileName) > 0)) {
    debugFile_ = fopen(debugFileName, "w");
  }

  if (params_.debug) logMsg("%s: entry, creating message queues, events, threads, etc.", functionName);
 
  toDoQueue_ = epicsMessageQueueCreate(params_.numProjections, sizeof(toDoMessageStruct));
  doneQueue_ = epicsMessageQueueCreate(params_.numProjections, sizeof(doneMessageStruct));
  workerWakeEvents_ = (epicsEventId *) malloc(params_.numThreads * sizeof(epicsEventId));
  workerDoneEvents_ = (epicsEventId *) malloc(params_.numThreads * sizeof(epicsEventId));
  supervisorWakeEvent_ = epicsEventCreate(epicsEventEmpty);
  supervisorDoneEvent_ = epicsEventCreate(epicsEventEmpty);

  /* Create the thread for the supervisor task */
  supervisorTaskId = epicsThreadCreate("supervisorTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC) ::supervisorTask,
                                this);
  if (supervisorTaskId == 0) {
    logMsg("%s: epicsThreadCreate failure for supervisorTask", functionName); 
  }

  /* Create the worker tasks */
  for (i=0; i<params_.numThreads; i++) {
    workerWakeEvents_[i] = epicsEventCreate(epicsEventEmpty);
    workerDoneEvents_[i] = epicsEventCreate(epicsEventEmpty);
    sprintf(workerTaskName, "workerTask%d", i);
    pWCS = (workerCreateStruct *)malloc(sizeof(workerCreateStruct));
    pWCS->pTomoPreprocess = this;
    pWCS->taskNum = i;
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

  // Fill up the toDoQueue with projections to be preprocessed
  for (i=0; i<params_.numProjections; i++) {
    toDoMessage.projectionNumber = i;
    toDoMessage.pIn = pIn;
    toDoMessage.pOut = pOut;
    pIn += projectionSize;
    if (params_.outputDataType == ODT_UInt16)
      pOut += projectionSize * sizeof(epicsUInt16);
    else
      pOut += projectionSize * sizeof(epicsFloat32);
    status = epicsMessageQueueTrySend(toDoQueue_, &toDoMessage, sizeof(toDoMessage));
    if (status) {
      logMsg("%s:, error calling epicsMessageQueueTrySend, status=%d", 
          functionName, status);
    }
  }
  // Send events to start preprocessing
  if (params_.debug) logMsg("%s: sending events to start preprocessing", functionName);
  epicsEventSignal(supervisorWakeEvent_);
  for (i=0; i<params_.numThreads; i++) epicsEventSignal(workerWakeEvents_[i]);
}

/** Destructor for the tomoPreprocess class.
* Calls shutDown() to stop any active preprocessing, which causes workerTasks to exit.
* Waits for supervisor task to exit, which in turn waits for all workerTasks to exit.
* Destroys the EPICS message queues, events and mutexes. Closes the debugging file. */
tomoPreprocess::~tomoPreprocess() 
{
  int i;
  int status;
  static const char *functionName = "tomoPreprocess:~tomoPreprocess";
  
  if (params_.debug) logMsg("%s: entry, shutting down and cleaning up", functionName);
  shutDown();
  status = epicsEventWait(supervisorDoneEvent_);
  if (status) {
    logMsg("%s: error waiting for supervisorDoneEvent=%d", functionName, status);
  }
  epicsMessageQueueDestroy(toDoQueue_);
  epicsMessageQueueDestroy(doneQueue_);
  epicsEventDestroy(supervisorWakeEvent_);
  epicsEventDestroy(supervisorDoneEvent_);
  for (i=0; i<params_.numThreads; i++) {
    epicsEventDestroy(workerWakeEvents_[i]);
    epicsEventDestroy(workerDoneEvents_[i]);
  }
  free(workerWakeEvents_);
  free(workerDoneEvents_);
  if (debugFile_ != stdout) fclose(debugFile_);
}

/** Function to poll the status of the preprocessing
* \param[out] pPreprocessComplete 0 if preprocessing is still in progress, 1 if it is complete
* \param[out] pProjectionsRemaining Number of projections remaining to be preprocessed */
void tomoPreprocess::poll(int *pPreprocessComplete, int *pProjectionsRemaining)
{
  *pPreprocessComplete = preprocessComplete_;
  *pProjectionsRemaining = projectionsRemaining_;
}

/** Function to shut down the object
* Sets the shutDown_ flag and sends an event to wake up the supervisorTask and workerTasks. */
void tomoPreprocess::shutDown()
{
  int i;
  
  // Set the shutdown flag
  shutDown_ = 1;
  // Send events to all threads waking them up
  epicsEventSignal(supervisorWakeEvent_);
  for (i=0; i<params_.numThreads; i++) epicsEventSignal(workerWakeEvents_[i]);
}

/** Supervisor control task that runs as a separate thread. 
* Reads messages from the workerTasks to update the status of the preprocessing (preprocessingComplete and projectionsRemaining).
* When shutting down waits for events from workerTasks threads indicating that they have all exited. */
void tomoPreprocess::supervisorTask()
{
  int i;
  int status;
  doneMessageStruct doneMessage;
  static const char *functionName="tomoPreprocess::supervisorTask";

  while (1) {
    if (params_.debug) logMsg("%s: waiting for wake event", functionName);
    // Wait for a wake event
    epicsEventWait(supervisorWakeEvent_);
    if (shutDown_) goto done;
    // Wait for all the messages to come back, indicating that preprocessing is complete
    while (projectionsRemaining_ > 0) {
      if (shutDown_) goto done;
      status = epicsMessageQueueReceive(doneQueue_, &doneMessage, sizeof(doneMessage));
      if (status != sizeof(doneMessage)) {
        logMsg("%s, error reading worker thread message", functionName);
        continue;
      }
      projectionsRemaining_--;
    }
    if (params_.debug) logMsg("%s: All projections complete!", functionName);
    preprocessComplete_ = 1;
    if (params_.debug) logMsg("%s: Preprocessing complete!", functionName);
  }
  done:
  // Wait for the worker threads to exit before setting the preprocessing complete flag.
  // They will exit because the toDoQueue is now empty
  for (i=0; i<params_.numThreads; i++) {
    if (params_.debug) logMsg("%s: Beginning wait for worker task %d to complete, eventId=%p", 
          functionName, i, workerDoneEvents_[i]);
    status = epicsEventWaitWithTimeout(workerDoneEvents_[i], 1.0);
    if (status != epicsEventWaitOK) {
      logMsg("%s: Error waiting for worker task %d to complete, eventId=%p, status=%d", 
          functionName, i, workerDoneEvents_[i], status);
    }
  }
  // Send a signal to the destructor that the supervisor task is done
  if (params_.debug) logMsg("%s: Exiting supervisor task.", functionName);
  epicsEventSignal(supervisorDoneEvent_);
}

/** Worker task that runs as a separate thread. Multiple worker tasks can be running simultaneously.
 * Each workerTask thread preprocesses projections that it gets from the toDoQueue, and sends messages to
 * the supervisorTask via the doneQueue after preprocessing the projection.
 * \param[in] taskNum Task number (0 to numThreads-1) for this tread; used to into arrays of event numbers in the object.
 */
void tomoPreprocess::workerTask(int taskNum)
{
  toDoMessageStruct toDoMessage;
  doneMessageStruct doneMessage;
  epicsEventId wakeEvent = workerWakeEvents_[taskNum];
  epicsEventId doneEvent = workerDoneEvents_[taskNum];
  epicsTime tStart, tStop;
  int status;
  static const char *functionName="tomoPreprocess::workerTask";
  
  while (1) {
    if (params_.debug) logMsg("%s: %s waiting for wake event", functionName, epicsThreadGetNameSelf());
    // Wait for an event signalling that preprocessing has started or exiting
    epicsEventWait(wakeEvent);
    if (shutDown_) goto done;
    while (1) {
      status = epicsMessageQueueTryReceive(toDoQueue_, &toDoMessage, sizeof(toDoMessage));
      if (status == -1) break;
      if (status != sizeof(toDoMessage)) {
        logMsg("%s:, error calling epicsMessageQueueReceive, status=%d", functionName, status);
        break;
      }
      tStart = epicsTime::getCurrent();
      
      epicsUInt16 *pIn = toDoMessage.pIn;
      epicsUInt16 *pOutUInt16 = (epicsUInt16 *) toDoMessage.pOut;
      epicsFloat32 *pOutFloat32 = (epicsFloat32 *) toDoMessage.pOut;
      int projectionSize = params_.numPixels * params_.numSlices;
      int numZingers=0;
      float ratio;
      float scaleFactor = params_.scaleFactor;
      if (scaleFactor == 1) scaleFactor = 0.;
      float zingerThreshold = params_.zingerThreshold;
      if (scaleFactor != 0.) zingerThreshold *= scaleFactor;
      
      for (int i=0; i<projectionSize; i++) {
        ratio = (pIn[i] - pDark_[i]) / pFlat_[i];
        if (scaleFactor != 0.) {
          ratio *= scaleFactor;
        }
        if (params_.outputDataType == ODT_UInt16) {
          pOutUInt16[i] = (epicsUInt16) ratio;
        } else {
          pOutFloat32[i] = ratio;
        }
      }

      tStop = epicsTime::getCurrent();
      doneMessage.normalizeTime = tStop - tStart;

      tStart = epicsTime::getCurrent();
      int zw = params_.zingerWidth;
      std::vector<float> windowValues(zw*zw, 0);
      size_t windowSize2 = windowValues.size()/2;
      auto medianTarget = windowValues.begin() + windowSize2;
      // Zinger correction
      // Outer loops move averaging window through the image
      if ((zw > 0) && (params_.zingerThreshold > 0.0)) {
        for (int i=0; i<params_.numSlices; i+=zw) {
          for (int j=0; j<params_.numPixels; j+=zw) {
            // First inner loops calculate the median of the pixels in the averaging window
            int m = 0;
            for (int k=0; k<zw; k++) {
              int iy = std::min(i+k, params_.numSlices-1) * params_.numPixels;
              for (int l=0; l<zw; l++) {
                int ix = std::min(j+l, params_.numPixels-1);
                if (params_.outputDataType == ODT_UInt16) {
                  windowValues[m++] = pOutUInt16[iy + ix];
                } else {
                  windowValues[m++] = pOutFloat32[iy + ix];
                }
              }
            }
            std::nth_element(windowValues.begin(), medianTarget, windowValues.end());
            float median = windowValues[windowSize2];
            // Next inner loops replace pixels which are more than threshold above the median with the median
            for (int k=0; k<zw; k++) {
              int iy = std::min(i+k, params_.numSlices-1) * params_.numPixels;
              for (int l=0; l<zw; l++) {
                if (params_.outputDataType == ODT_UInt16) {
                  int ix = std::min(j+l, params_.numPixels-1);
                  if ( (pOutUInt16[iy + ix] - median) > zingerThreshold) {
                    numZingers++;
                    pOutUInt16[iy + ix] = (epicsUInt16) median;
                  }
                } else {
                  int ix = std::min(j+l, params_.numPixels-1);
                  if ( (pOutFloat32[iy + ix] - median) > zingerThreshold) {
                    numZingers++;
                    pOutFloat32[iy + ix] = median;
                  }
                }
              }
            }        
          }
        }
      }

      tStop = epicsTime::getCurrent();
      doneMessage.zingerTime = tStop - tStart;
      doneMessage.projectionNumber = toDoMessage.projectionNumber;
      status = epicsMessageQueueTrySend(doneQueue_, &doneMessage, sizeof(doneMessage));
      if (status) {
        logMsg("%s:, error calling epicsMessageQueueTrySend, status=%d", functionName, status);
      }
      if (params_.debug) { 
        logMsg("%s:, thread=%s, projection=%d, normalize time=%f, zinger time=%f, numZingers=%d", 
            functionName, epicsThreadGetNameSelf(), doneMessage.projectionNumber,
            doneMessage.normalizeTime, doneMessage.zingerTime, numZingers);
      }
      if (shutDown_) break;
    }
  }
  done:
  epicsEventSignal(doneEvent);
  if (params_.debug) {
    logMsg("tomoPreprocess::workerTask %s exiting, eventId=%p", 
        epicsThreadGetNameSelf(), doneEvent);
  }
}


/** Logs messages.
 * Adds time stamps to each message.
 * Does buffering to prevent messages from multiple threads getting garbled.
 * Adds the appropriate terminator for files (LF) and stdout (CR LF, needed for IDL on Linux).
 * Flushes output after each call, so output appears even if application crashes.
 * \param[in] pFormat Format string
 * \param[in] ... Additional arguments for vsprintf
 */
void tomoPreprocess::logMsg(const char *pFormat, ...)
{
  va_list     pvar;
  epicsTimeStamp now;
  char nowText[40];
  char message[256];
  char temp[256];

  epicsTimeGetCurrent(&now);
  nowText[0] = 0;
  epicsTimeToStrftime(nowText,sizeof(nowText),
      "%Y/%m/%d %H:%M:%S.%03f",&now);
  sprintf(message,"%s ",nowText);
  va_start(pvar,pFormat);
  vsprintf(temp, pFormat, pvar);
  va_end(pvar);
  strcat(message, temp);
  if (debugFile_ == stdout)
    strcat(message, "\r\n");
  else
    strcat(message, "\n");
  fprintf(debugFile_, message);
  fflush(debugFile_);
}
