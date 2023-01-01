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
tomoPreprocess::tomoPreprocess(preprocessParams_t *pPreprocessParams)
  : pPreprocessParams_(pPreprocessParams),
    numPixels_(pPreprocessParams_->numPixels),
    numSlices_(pPreprocessParams_->numSlices),
    numProjections_(pPreprocessParams_->numProjections),
    numThreads_(pPreprocessParams_->numThreads),
    queueElements_(numProjections_),
    debug_(pPreprocessParams_->debug),
    preprocessComplete_(1),
    shutDown_(0)

{
  epicsThreadId supervisorTaskId;
  char workerTaskName[20];
  epicsThreadId workerTaskId;
  workerCreateStruct *pWCS;
  char *debugFileName = pPreprocessParams_->debugFileName;
  int i;
  static const char *functionName="tomoPreprocess::tomoPreprocess";

  debugFile_ = stdout;
  if ((debugFileName) && (strlen(debugFileName) > 0)) {
    debugFile_ = fopen(debugFileName, "w");
  }
  
  if (debug_) logMsg("%s: entry, creating message queues, events, threads, etc.", functionName);
 
  toDoQueue_ = epicsMessageQueueCreate(queueElements_, sizeof(toDoMessage_t));
  doneQueue_ = epicsMessageQueueCreate(queueElements_, sizeof(doneMessage_t));
  workerWakeEvents_ = (epicsEventId *) malloc(numThreads_ * sizeof(epicsEventId));
  workerDoneEvents_ = (epicsEventId *) malloc(numThreads_ * sizeof(epicsEventId));
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
  for (i=0; i<numThreads_; i++) {
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
  
  if (debug_) logMsg("%s: entry, shutting down and cleaning up", functionName);
  shutDown();
  status = epicsEventWait(supervisorDoneEvent_);
  if (status) {
    logMsg("%s: error waiting for supervisorDoneEvent=%d", functionName, status);
  }
  epicsMessageQueueDestroy(toDoQueue_);
  epicsMessageQueueDestroy(doneQueue_);
  epicsEventDestroy(supervisorWakeEvent_);
  epicsEventDestroy(supervisorDoneEvent_);
  for (i=0; i<numThreads_; i++) {
    epicsEventDestroy(workerWakeEvents_[i]);
    epicsEventDestroy(workerDoneEvents_[i]);
  }
  free(workerWakeEvents_);
  free(workerDoneEvents_);
  if (debugFile_ != stdout) fclose(debugFile_);
}

/** Function to start preprocessing a set of projections
* Sends messages to workerTasks to begin the preprocesssing and wakes up
* the supervisorTasks and workerTasks.
* \param[in] numProjections Number of projections to preprocess
* \param[in] center Rotation center to use for each slice
* \param[in] pInput Pointer to input data [numPixels, numSlices, numProjections]
* \param[out] pOutput Pointer to output data [numPixels, numPixels, numSlices] */
int tomoPreprocess::preprocess(int numProjections, float *pDark, float *pFlat, epicsUInt16 *pInput, float *pOutput)
{
  epicsUInt16 *pIn;
  float *pOut;
  toDoMessage_t toDoMessage;
  int projectionSize = numPixels_ * numSlices_;
  int i;
  int status;
  static const char *functionName="tomoPreprocess::preprocess";

  // If a preprocessingis already in progress return an error
  if (debug_) logMsg("%s: entry, preprocessComplete_=%d", functionName, preprocessComplete_);
  if (preprocessComplete_ == 0) {
    logMsg("%s: error, preprocessing already in progress", functionName);
    return -1;
  }

  if (numProjections > pPreprocessParams_->numProjections) {
    logMsg("%s: error, numSlices=%d, must be <= %d", functionName, numProjections, pPreprocessParams_->numProjections);
    return -1;
  }
  
  numProjections_ = numProjections;
  projectionsRemaining_ = numProjections_;
  pDark = pDark;
  pFlat = pFlat;
  pInput_ = pInput;
  pOutput_ = pOutput;
  pIn = pInput_;
  pOut = pOutput_;

  preprocessComplete_ = 0;

  // Fill up the toDoQueue with projections to be preprocessed
  for (i=0; i<numProjections_; i++) {
    toDoMessage.projectionNumber = i;
    toDoMessage.pIn = pIn;
    toDoMessage.pOut = pOut;
    pIn += projectionSize;
    pOut += projectionSize;
    status = epicsMessageQueueTrySend(toDoQueue_, &toDoMessage, sizeof(toDoMessage));
    if (status) {
      logMsg("%s:, error calling epicsMessageQueueTrySend, status=%d", 
          functionName, status);
    }
  }
  // Send events to start preprocessing
  if (debug_) logMsg("%s: sending events to start preprocessing", functionName);
  epicsEventSignal(supervisorWakeEvent_);
  for (i=0; i<numThreads_; i++) epicsEventSignal(workerWakeEvents_[i]);
  
  return 0;
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
  for (i=0; i<numThreads_; i++) epicsEventSignal(workerWakeEvents_[i]);
}

/** Supervisor control task that runs as a separate thread. 
* Reads messages from the workerTasks to update the status of the preprocessing (preprocessingComplete and projectionsRemaining).
* When shutting down waits for events from workerTasks threads indicating that they have all exited. */
void tomoPreprocess::supervisorTask()
{
  int i;
  int status;
  doneMessage_t doneMessage;
  static const char *functionName="tomoPreprocess::supervisorTask";

  while (1) {
    if (debug_) logMsg("%s: waiting for wake event", functionName);
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
    if (debug_) logMsg("%s: All projections complete!", functionName);
    preprocessComplete_ = 1;
    if (debug_) logMsg("%s: Preprocessing complete!", functionName);
  }
  done:
  // Wait for the worker threads to exit before setting the preprocessing complete flag.
  // They will exit because the toDoQueue is now empty
  for (i=0; i<numThreads_; i++) {
    if (debug_) logMsg("%s: Beginning wait for worker task %d to complete, eventId=%p", 
          functionName, i, workerDoneEvents_[i]);
    status = epicsEventWaitWithTimeout(workerDoneEvents_[i], 1.0);
    if (status != epicsEventWaitOK) {
      logMsg("%s: Error waiting for worker task %d to complete, eventId=%p, status=%d", 
          functionName, i, workerDoneEvents_[i], status);
    }
  }
  // Send a signal to the destructor that the supervisor task is done
  if (debug_) logMsg("%s: Exiting supervisor task.", functionName);
  epicsEventSignal(supervisorDoneEvent_);
}

/** Worker task that runs as a separate thread. Multiple worker tasks can be running simultaneously.
 * Each workerTask thread preprocesses projections that it gets from the toDoQueue, and sends messages to
 * the supervisorTask via the doneQueue after preprocessing the projection.
 * \param[in] taskNum Task number (0 to numThreads-1) for this tread; used to into arrays of event numbers in the object.
 */
void tomoPreprocess::workerTask(int taskNum)
{
  toDoMessage_t toDoMessage;
  doneMessage_t doneMessage;
  epicsEventId wakeEvent = workerWakeEvents_[taskNum];
  epicsEventId doneEvent = workerDoneEvents_[taskNum];
  epicsTime tStart, tStop;
  int status;
  int i;
  static const char *functionName="tomoPreprocess::workerTask";
  
  while (1) {
    if (debug_) logMsg("%s: %s waiting for wake event", functionName, epicsThreadGetNameSelf());
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
      float *pOut = toDoMessage.pOut;
      int projectionSize = numPixels_ * numSlices_;
      
      for (i=0; i<projectionSize; i++) {
        pOut[i] = scaleFactor_ * (pIn[i] - pDark_[i]) / pFlat_[i];
      }

      tStop = epicsTime::getCurrent();
      doneMessage.normalizeTime = tStop - tStart;
 
      tStart = epicsTime::getCurrent();
      // Do zinger correction here 
      tStop = epicsTime::getCurrent();
      doneMessage.zingerTime = tStop - tStart;
      doneMessage.projectionNumber = toDoMessage.projectionNumber;
      status = epicsMessageQueueTrySend(doneQueue_, &doneMessage, sizeof(doneMessage));
      if (status) {
        printf("%s, error calling epicsMessageQueueTrySend, status=%d", functionName, status);
      }
      if (debug_ > 0) { 
        logMsg("%s:, thread=%s, projection=%d, normalize time=%f, zinger time=%f", 
            functionName, epicsThreadGetNameSelf(), doneMessage.projectionNumber,
            doneMessage.normalizeTime, doneMessage.zingerTime);
      }
      if (shutDown_) break;
    }
  }
  done:
  epicsEventSignal(doneEvent);
  if (debug_ > 0) {
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
