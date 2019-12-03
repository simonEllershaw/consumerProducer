/******************************************************************
 * The Main program with the two functions. A simple
 * example of creating and using a thread is provided.
 ******************************************************************/

#include "helper.h"

// Global constants
const int REQ_NUM_CMD_LINE_ARGS = 5;
const char KEY_CHAR = 'M';
// Sempaphore constants
const int NUM_SEMAPHORES = 3;
const int MUTEX_SEMAPHORE_INDEX = 0;
const int SPACE_SEMAPHORE_INDEX = 1;
const int ITEM_SEMAPHORE_INDEX = 2;
// Constants for random time
const int MAX_PRODUCER_WAIT_TIME = 5;
const int MAX_JOB_TIME = 10;

void *producer (void *parameter);
void *consumer (void *parameter);

int main (int argc, char **argv)
{
  // Read in command line args
  if(argc != REQ_NUM_CMD_LINE_ARGS){
    std::cerr << "Error- Incorrect number of cmd line args given ("
              << (REQ_NUM_CMD_LINE_ARGS - 1) << " are required)." << std::endl;
    return -1;
  }
  const int queueSize = check_arg(argv[1]);
  const int numJobsPerProducer = check_arg(argv[2]);
  const int numProducers = check_arg(argv[3]);
  const int numConsumers = check_arg(argv[4]);

  // Set up and intialise data structures
  // The main shared data structure- queue
  int jobs[queueSize], errorCode = 0;
  struct JobsCircularQueue queue = {jobs, 0, 0};
  // Parameters and threadIDs used at creation of threads stored for later
  // cleanup
  struct ThreadParameter* parameters[numProducers + numConsumers] = {nullptr};
  pthread_t threadIDs[numProducers + numConsumers];

  // Set up and intialise semphores
  const int semaphoreArrayID = sem_create (SEM_KEY, NUM_SEMAPHORES);
  // Mutex is binary semaphore so intial value of 1
  errorCode = sem_init(semaphoreArrayID, MUTEX_SEMAPHORE_INDEX, 1);
  // Ensures buffer not full before prodcuer is allowed access
  errorCode = sem_init(semaphoreArrayID, SPACE_SEMAPHORE_INDEX, queueSize);
  // Ensures there an item in the buffer before consumer allowed access
  errorCode = sem_init(semaphoreArrayID, ITEM_SEMAPHORE_INDEX, 0);
  if(errorCode){
    fprintf(stderr, "Error- The sempahore intialisation failed. Please ensure"
                  "their are no dangling semaphores using key: %x\n", SEM_KEY);
    return -1;
  }

  // Create producers
  for(int producerNum = 0; producerNum < numProducers; producerNum++){
    // (producerNum+1) used so threads printed from 1 and not 0
    parameters[producerNum] = new ThreadParameter{
                                    &queue, &semaphoreArrayID, (producerNum+1),
                                    &queueSize, &numJobsPerProducer};
    errorCode = pthread_create (&threadIDs[producerNum], NULL, producer,
                    (void *) parameters[producerNum]);
    if(errorCode){
      // Assume none critical and so continue execution without thread
      fprintf(stderr, "Warning- Failed to create Producer(%d)\n", producerNum);
      errorCode = 0;
    }
  }

  // Create consumers
  for(int consumerNum = 0; consumerNum < numConsumers; consumerNum++){
    // Array indicies displaced by the number of producers allready created
    // Number of jobs is not a field required or consumers and so is NULL
    parameters[consumerNum + numProducers] = new ThreadParameter{
                                    &queue, &semaphoreArrayID, (consumerNum+1),
                                    &queueSize, NULL};
    pthread_create (&threadIDs[consumerNum + numProducers], NULL, consumer,
                    (void *) parameters[consumerNum + numProducers]);
    if(errorCode){
      // Assume none critical and so continue execution without thread
      fprintf(stderr, "Warning- Failed to create Consumer(%d)\n", consumerNum);
      errorCode = 0;
    }
  }

  // Wait for all threads to finish execution
  for(int threadNum = 0; threadNum < numConsumers + numProducers; threadNum++){
    errorCode = pthread_join(threadIDs[threadNum], NULL);
    if(errorCode){
      // Assume none critical and so continue execution without thread
      fprintf(stderr, "Warning- Thread %lu may not have finished execution\n",
              threadIDs[threadNum]);
      errorCode = 0;
    }
  }

  // Clean up
  for(int threadNum = 0; threadNum < numConsumers + numProducers; threadNum++){
    if(parameters[threadNum] != nullptr) delete parameters[threadNum];
  }
  errorCode = sem_close(semaphoreArrayID);
  if(errorCode){
    fprintf(stderr, "Error- Failed to close the semaphore array. Please do this"
                    "manually using ipcs and ipcrm");
    return -1;
  }

  return 0;
}

void *producer (void *parameter)
{
  int jobTime, jobID, errorCode;
  //Unpack parameter
  struct ThreadParameter* param = (ThreadParameter *) parameter;
  const int& numJobs = *(param->numJobs);
  JobsCircularQueue* & queue = param->queue;
  const int& semaphoneID = *(param->semaphoneID);
  const int& producerNum = param->threadNum;
  const int& queueSize = *(param->queueSize);

  for(int jobNum = 0; jobNum < numJobs; jobNum++){
    // Wait for 1 to MAX_PRODUCER_WAIT_TIME secs
    sleep(rand() % MAX_PRODUCER_WAIT_TIME + 1);
    // Set random job time
    jobTime = rand() % MAX_JOB_TIME + 1;

    // Wait for space in buffer (20 secs) and no other threads accessing
    // the queue (no time limit). Checks for semaphore failures also occur
    errorCode = sem_wait(semaphoneID, SPACE_SEMAPHORE_INDEX, TIMEOUT);
    if(errno == EAGAIN){
      fprintf(stderr, "Producer(%d): Timed out as queue is full\n", producerNum);
      pthread_exit(0);
    }
    else if(errorCode){
      fprintf(stderr, "Producer(%d): SPACE semaphore failed exiting thread\n",
              producerNum);
      pthread_exit(0);
    }
    if(sem_wait(semaphoneID, MUTEX_SEMAPHORE_INDEX, NULL)){
      fprintf(stderr, "Producer(%d): MUTEX semaphore failed exiting thread\n",
              producerNum);
      pthread_exit(0);
    }

    // Add job to the tail of the queue, record the jobID and increment the tail
    // by 1
    jobID = (queue->tail + 1);
    queue->jobs[queue->tail] = jobTime;
    queue->tail = jobID % queueSize; // Modulo as circular queue

    // Re-allow access to the queue and increment the number of items by 1
    // Raise users attention if errors occur
    errorCode = sem_signal(semaphoneID, MUTEX_SEMAPHORE_INDEX);
    if(errorCode){
      fprintf(stderr, "Producer(%d): Warning MUTEX semaphore may not have been"
        "restored to correct value. Exciting thread manual kill and clean up"
        "maybe required\n", producerNum);
      pthread_exit(0);
    }
    sem_signal(semaphoneID, ITEM_SEMAPHORE_INDEX);
    if(errorCode){
      fprintf(stderr, "Producer(%d): Warning ITEM semaphore may not have been"
        "restored to correct value. Exciting thread manual kill and clean up"
        "maybe required\n", producerNum);
      pthread_exit(0);
    }

    // Print status use fprintf to avoid interleaving of prints
    fprintf(stderr, "Producer(%d): Job id %d duration %d\n",
            producerNum, jobID, jobTime);
  }
  fprintf(stderr, "Producer(%d): No jobs left to generate\n", producerNum);
  pthread_exit(0);
}

void *consumer (void *parameter)
{
  int jobTime, jobID, errorCode;
  //Unpack parameter (mainly to increase readability)
  struct ThreadParameter* param = (ThreadParameter *) parameter;
  JobsCircularQueue* & queue = param->queue;
  const int& semaphoneID = *(param->semaphoneID);
  const int& consumerNum = param->threadNum;
  const int& queueSize = *(param->queueSize);

  // Wait an item to be in the queue (20 secs) and no other threads accessing
  // the queue (no time limit)
  while(!(errorCode = sem_wait(semaphoneID, ITEM_SEMAPHORE_INDEX, TIMEOUT))){
    if(sem_wait(semaphoneID, MUTEX_SEMAPHORE_INDEX, NULL)){
      fprintf(stderr, "Consumer(%d): MUTEX semaphore failed exiting thread\n",
              consumerNum);
      pthread_exit(0);
    }

    // Read the jobID and time at the head of the queue and increment the head
    // by 1
    jobID = (queue->head + 1);
    jobTime = queue->jobs[queue->head];
    queue->head = jobID % queueSize;

    // Reallow access to the queue and increment the amount of space by 1
    errorCode = sem_signal(semaphoneID, MUTEX_SEMAPHORE_INDEX);
    if(errorCode){
      fprintf(stderr, "Producer(%d): Warning MUTEX semaphore may not have been"
        "restored to correct value. Exciting thread manual kill and clean up"
        "maybe required\n", consumerNum);
      pthread_exit(0);
    }
    sem_signal(semaphoneID, SPACE_SEMAPHORE_INDEX);
    if(errorCode){
      fprintf(stderr, "Producer(%d): Warning ITEM semaphore may not have been"
        "restored to correct value. Exciting thread manual kill and clean up"
        "maybe required\n", consumerNum);
      pthread_exit(0);
    }

    // Execute job outside sempahore locks
    fprintf(stderr, "Consumer(%d): Job id %d executing sleep duration %d\n",
            consumerNum, jobID, jobTime);
    sleep(jobTime);
    fprintf(stderr, "Consumer(%d): Job id %d completed\n", consumerNum, jobID);
  }

  // Ensure consumer exits due to timeout error
  if(errno == EAGAIN){
    fprintf(stderr, "Consumer(%d): No jobs left\n", consumerNum);
  }
  else{
    fprintf(stderr, "Consumer(%d): ITEM semaphore failed. Exiting thread\n",
            consumerNum);
  }

  pthread_exit (0);
}
