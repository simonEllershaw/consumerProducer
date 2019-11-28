/******************************************************************
 * Header file for the helper functions. This file includes the
 * required header files, as well as the function signatures and
 * the semaphore values.
 ******************************************************************/


# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/ipc.h>
# include <sys/shm.h>
# include <sys/sem.h>
# include <sys/time.h>
# include <math.h>
# include <errno.h>
# include <string.h>
# include <pthread.h>
# include <ctype.h>
# include <iostream>
# include <vector>
using namespace std;

# define SEM_KEY 0x4d37f876
// Defines how long producer or consumer will wait for SPACE/ITEM sempahore to
// allow access to the queue
const struct timespec TIMEOUT[]={{20,0}};

union semun {
    int val;               /* used for SETVAL only */
    struct semid_ds *buf;  /* used for IPC_STAT and IPC_SET */
    ushort *array;         /* used for GETALL and SETALL */
};


struct JobsCircularQueue{
  // Note JobID given implicitly by the job's index + 1. Only the job time is
  // explicily stored.
      int* jobs;
      int head; // Position in the jobs array to next be consumed
      int tail; // "                        " to next have a job place in it
};

struct ThreadParameter{
  // Parameter passed to consumer and producer function
      JobsCircularQueue* queue;
      const int* semaphoneID;
      const int threadNum;
      const int* queueSize;
      const int* numJobs;
};


int check_arg (char *);
int sem_create (key_t, int);
int sem_init (int, int, int);
int sem_wait (int, short unsigned int, const struct timespec* waitTime);
void sem_signal (int, short unsigned int);
int sem_close (int);
