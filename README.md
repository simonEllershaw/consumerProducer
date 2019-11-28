# consumerProducer

This is an implementation of the [consumer-producer scenario](https://en.wikipedia.org/wiki/Producer%E2%80%93consumer_problem) using POSIX threads.

Each producer creates a specified number of jobs and adds them to a circular queue at random time periods. The consumers then take these jobs and execute them. In this toy example the jobs are just to sleep for a random period of time.

3 semaphores are used to control access to the queue of the differing threads:
1. Mutex- ensures only 1 thread has access to the queue at once
2. Item- ensures there is a job present before a consumer is granted access
3. Space- ensures there is pace in the queue before a producer is granted access

If a thread cannot get access to the queue within 20 secs it will end its process

## To Run

```
make
./main queueSize numJobsPerProducer numProducers numConsumers
```

If execution fails due to semaphores not being intialised corectly this may be sue to the semaphore key already being in use therefore run:
`impcrm -a`.

## Example output

```
$ make
g++    -c -o helper.o helper.cc
g++ -Wall -c helper.cc main.cc
g++ -Wall -pthread -o main helper.o main.o
$ ./main 2 2 2 2
Producer(2): Job id 1 duration 8
Consumer(1): Job id 1 executing sleep duration 8
Producer(2): Job id 2 duration 4
Producer(2): No jobs left to generate
Consumer(2): Job id 2 executing sleep duration 4
Producer(1): Job id 1 duration 6
Producer(1): Job id 2 duration 3
Producer(1): No jobs left to generate
Consumer(2): Job id 2 completed
Consumer(2): Job id 1 executing sleep duration 6
Consumer(1): Job id 1 completed
Consumer(1): Job id 2 executing sleep duration 3
Consumer(1): Job id 2 completed
Consumer(2): Job id 1 completed
Consumer(2): No jobs left
Consumer(1): No jobs left
```
