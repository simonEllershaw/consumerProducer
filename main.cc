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
    return(-1);
  }
  const int queueSize = check_arg(argv[1]);
  const int numJobsPerProducer = check_arg(argv[2]);
  const int numProducers = check_arg(argv[3]);
  const int numConsumers = check_arg(argv[4]);

  // Set up and intialise data structures
  // The main shared data structure- queue
  int jobs[queueSize];
  struct JobsCircularQueue queue = {jobs, 0, 0};
  // Parameters and threadIDs used at creation of threads stored for later
  // cleanup
  struct ThreadParameter* parameters[numProducers + numConsumers];
  pthread_t threadIDs[numProducers + numConsumers];

  // Set up and intialise semphores
  int semaphoreInitFailed = 0;
  const int semaphoreArrayID = sem_create (SEM_KEY, NUM_SEMAPHORES);
  // Mutex is binary semaphore so intial value of 1
  semaphoreInitFailed = sem_init(semaphoreArrayID, MUTEX_SEMAPHORE_INDEX, 1);
  // Ensures buffer not full before prodcuer is allowed access
  semaphoreInitFailed = sem_init(semaphoreArrayID, SPACE_SEMAPHORE_INDEX, queueSize);
  // Ensures there an item in the buffer before consumer allowed access
  semaphoreInitFailed = sem_init(semaphoreArrayID, ITEM_SEMAPHORE_INDEX, 0);
  if(semaphoreInitFailed){
    cerr << "Error- The sempahore intialisation failed."
          << " Please ensure their are no dangling semaphores using key: "
          << hex << SEM_KEY << "." << endl;
    return(-1);
  }


  // Create producers
  for(int producerNum = 0; producerNum < numProducers; producerNum++){
    // (producerNum+1) used so threads printed from 1 and not 0
    parameters[producerNum] = new ThreadParameter{
                                    &queue, &semaphoreArrayID, (producerNum+1),
                                    &queueSize, &numJobsPerProducer};
    pthread_create (&threadIDs[producerNum], NULL, producer,
                    (void *) parameters[producerNum]);
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
  }

  // Wait for all threads to finish execution
  for(int threadNum = 0; threadNum < numConsumers + numProducers; threadNum++){
    pthread_join(threadIDs[threadNum], NULL);
  }

  // Clean up
  for(int threadNum = 0; threadNum < numConsumers + numProducers; threadNum++){
    delete parameters[threadNum];
  }
  sem_close(semaphoreArrayID);

  return 0;
}

void *producer (void *parameter)
{
  int jobTime, jobID;
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
    // the queue (no time limit)
    if(sem_wait(semaphoneID, SPACE_SEMAPHORE_INDEX, TIMEOUT)){
      cerr << "Producer(" << producerNum << "): Timed out as queue is full"
            << endl;
      pthread_exit (0);
    }
    sem_wait(semaphoneID, MUTEX_SEMAPHORE_INDEX, NULL);
    // Add job to the tail of the queue, record the jobID and increment the tail
    // by 1
    jobID = (queue->tail + 1);
    queue->jobs[queue->tail] = jobTime;
    queue->tail = jobID % queueSize; // Modulo as circular queue

    // Print status
    cerr << "Producer(" << producerNum << "): Job id " << jobID
              << " duration " << jobTime << endl;

    // Re-allow access to the queue and increment the number of items by 1
    sem_signal(semaphoneID, MUTEX_SEMAPHORE_INDEX);
    sem_signal(semaphoneID, ITEM_SEMAPHORE_INDEX);
  }

  cerr << "Producer(" << producerNum << "): No jobs left to generate" << endl;
  pthread_exit(0);
}

void *consumer (void *parameter)
{
  int jobTime, jobID;
  //Unpack parameter (mainly to increase readability)
  struct ThreadParameter* param = (ThreadParameter *) parameter;
  JobsCircularQueue* & queue = param->queue;
  const int& semaphoneID = *(param->semaphoneID);
  const int& consumerNum = param->threadNum;
  const int& queueSize = *(param->queueSize);

  // Wait an item to be in the queue (20 secs) and no other threads accessing
  // the queue (no time limit)
  while(!sem_wait(semaphoneID, ITEM_SEMAPHORE_INDEX, TIMEOUT)){
    sem_wait(semaphoneID, MUTEX_SEMAPHORE_INDEX, NULL);
    // Read the jobID and time at the head of the queue and increment the head
    // by 1
    jobID = (queue->head + 1);
    jobTime = queue->jobs[queue->head];
    queue->head = jobID % queueSize;
    // Allow access to the queue and increment the amount of space by 1
    sem_signal(semaphoneID, MUTEX_SEMAPHORE_INDEX);
    sem_signal(semaphoneID, SPACE_SEMAPHORE_INDEX);

    // Execute job
    std::cerr << "Consumer(" << consumerNum << "): Job id " << jobID
              << " executing sleep duration " << jobTime << endl;
    sleep(jobTime);
    std::cerr << "Consumer(" << consumerNum << "): Job id " << jobID
              << " completed" << endl;
  }

  cerr << "Consumer(" << consumerNum << "): No jobs left" << endl;
  pthread_exit (0);
}
