/*
 * tomoPreprocess.h
 * 
 * C++ class for doing computed tomography preprocessing.
 *
 * This runs the preprocessing using multiple threads, each thread preprocessing a set of projections
 *
 * It uses the EPICS libCom library for OS-independent functions for threads, mutexes, message queues, etc.
 *
 * Author: Mark Rivers
 *
 * Created: December 31, 2022
 */

#include <epicsMessageQueue.h>
#include <epicsTypes.h>
#include <epicsThread.h>

/** Structure that is passed from the constructor to the workerTasks in the toDoQueue */
typedef struct {
  int projectionNumber;  /**< Number of this projection */
  epicsUInt16 *pIn;      /**< Pointer to raw projection */
  float *pOut;           /**< Pointer to normalized output */
} toDoMessage_t;

/** Structure that is passed from the workerTask to the supervisorTask in the doneQueue */
typedef struct {
  int projectionNumber;  /**< Number of this projection */
  double normalizeTime;  /**< Time required for dark and flat normalization */
  double zingerTime;     /**< Time required for zinger removal */
} doneMessage_t;

/** Structure that is passed to the constructor to define the preprocessing 
    NOTE: This structure must match the structure defined in IDL in tomo_preprocess_params__define.pro! 
 */
typedef struct {
  int numPixels;            /**< Number of horizontal pixels in the input data */
  int numSlices;            /**< Number of slices in the input data */
  int numProjections;       /**< Number of projection angles in the input data */
  int numThreads;           /**< Number of workerTask threads to create */
  int zingerWidth;          /**< Smoothing width for zinger removal */
  float zingerThreshold;    /**< Threshold for zinger removal */
  float scaleFactor;        /**< Scale factor to multiply normalized data by */
  int debug;                /**< Debug output level; 0: only error messages, 1: debugging from tomoPreprocess */
  char debugFileName[256];  /**< Name of file for debugging output;  use 0 length string ("") to send output to stdout */
} preprocessParams_t;

#ifdef __cplusplus

/** Structure that is used to create a worker task.  This is the structure passed to epicsThreadCreate() */
typedef struct {
  class tomoPreprocess *pTomoPreprocess; /**< Pointer to the tomoPreprocess object */
  int taskNum;                           /**< Task number that is passed to tomoPreprocess::workerTask */
} workerCreateStruct;

/** Class to do tomography preprocessing.
* Creates a supervisorTask that supervises the preprocessing process, and a set of workerTask threads
* that do the dark field correction, flat field correction, and zinger removal.
* When the class is created it can be used to preprocess many projections in a single call, and does
* the preprocessing using multiple threads and cores.  The preprocess function can be called 
* repeatedly to preprocess more sets projections.  Once the object is created it is restricted to
* preprocessinging with the same set of parameters, with the exception of the zinger threshold.  
* If the preprocessing parameters change (number of X pixels, number of projections, etc.) 
* then the tomoPreprocess object must be deleted and a new one created.
*/
class tomoPreprocess {
public:
  tomoPreprocess(preprocessParams_t *pTomoParams);
  virtual ~tomoPreprocess();
  virtual int preprocess(int numProjections, float *pDark, float *pFlat, epicsUInt16 *pInput, float *pOutput);
  virtual void supervisorTask();
  virtual void workerTask(int taskNum);
  virtual void poll(int *pPreprocessComplete, int *pProjectionsRemaining);
  virtual void logMsg(const char *pFormat, ...);

private:
  void shutDown();
  preprocessParams_t *pPreprocessParams_;
  int numPixels_;
  int numSlices_;
  int numProjections_;
  int numThreads_;
  epicsUInt16 *pInput_;
  float *pDark_;
  float *pFlat_;
  float *pOutput_;
  float scaleFactor_;
  int queueElements_;
  int debug_;
  FILE *debugFile_;
  int preprocessComplete_;
  int projectionsRemaining_;
  int shutDown_;
  epicsMessageQueueId toDoQueue_;
  epicsMessageQueueId doneQueue_;
  epicsEventId supervisorWakeEvent_;
  epicsEventId supervisorDoneEvent_;
  epicsEventId *workerWakeEvents_;
  epicsEventId *workerDoneEvents_;
};
#endif
