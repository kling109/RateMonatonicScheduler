/*
Name: Trevor Kling
ID: 002270716
Email: kling109@mail.chapman.edu
Course: CPSC 380 Operating Systems
Last Date Modified: 1 May 2019
Project: Rate Monotonic Scheduler
*/

#define _GNU_SOURCE
#include <sched.h>
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

using namespace std;

#define NUM_THREADS 4
#define THREAD_2_RUNS 2
#define THREAD_3_RUNS 4

int* BOARD[10];
int RUNTIME = 160;
bool run = true;
sem_t schedule;
sem_t wakeupSchedule [4];
pthread_mutex_t lock [4];
int counter [4];
int expected [4];

/*
Idling work function. Designed to promote cache misses and other small errors.
*/
void doWork()
{
  int product = 1;
  for (int i = 0; i < 5; ++i)
  {
    for (int j = 0; j < 2; ++j)
    {
      for (int k = 0; k < 10; ++k)
      {
        product *= BOARD[k][i+(5*j)];
      }
    }
  }
}

/*
Handles the threads.  Allows the scheduler to tell the thread when to run its
work, and is general enough to be given to all threads being run.
*/
void* threadHandler(void* number)
{
  int tnum = (int)(intptr_t)number;
  int pos;
  switch (tnum)
  {
    case 1: pos = 0;
            break;
    case THREAD_2_RUNS: pos = 1;
            break;
    case THREAD_3_RUNS: pos = 2;
            break;
    case 16: pos = 3;
             break;
    default: cout << "Received unexpected value." << endl;
             pos = -1;
  }
  sem_wait(&wakeupSchedule[pos]);
  while (pos != -1 && run == true)
  {
    for (int i = 0; i < tnum; ++i)
    {
      doWork();
    }
    pthread_mutex_lock(&lock[pos]);
    counter[pos] += 1;
    pthread_mutex_unlock(&lock[pos]);
    sem_wait(&wakeupSchedule[pos]);
  }
}

/*
Signaling function for the timer.  Allows the scheduler to "wake up" every time the timer elapses.
*/
void sigfunc(union sigval val)
{
  sem_post(&schedule);
}

/*
Main method.  Will be used to initialize and run threads.
Also initializes a semaphore.  The semaphore, with value of 0,
causes any thread that calls sem_wait to wait until a sem_signal
is called.  This ensures that, after a thread finishes its work, it waits to
be rescheduled until the sleep of the original thread finishes.
*/
int main()
{
  pthread_mutexattr_t priorityProtection;
  int protocol;
  pthread_mutexattr_init(&priorityProtection);
  pthread_mutexattr_getprotocol(&priorityProtection, &protocol);
  if (protocol != PTHREAD_PRIO_INHERIT)
  {
    pthread_mutexattr_setprotocol(&priorityProtection, PTHREAD_PRIO_INHERIT);
  }

  if (sem_init(&schedule, 0, 0) == -1)
  {
    cout << "The semaphore failed to initialize." << endl;
    return 1;
  }
  for (int i = 0; i < 4; ++i)
  {
    if (sem_init(&wakeupSchedule[i], 0, 0) == -1)
    {
      cout << "The semaphore failed to initialize." << endl;
      return 1;
    }
    if (pthread_mutex_init(&lock[i], &priorityProtection) != 0)
    {
      cout << "The mutex failed to initialize." << endl;
      return 1;
    }
  }

  for (int i = 0; i < 10; ++i)
  {
    int* row = new int[10];
    for (int j = 0; j < 10; ++j)
    {
      row[j] = 1;
    }
    BOARD[i] = row;
  }
  for (int i = 0; i < 4; ++i)
  {
    counter[i] = 0;
    expected[i] = 0;
  }

  // Set priority for main thread
  pthread_t mainID = pthread_self();
  struct sched_param mainParams, mainParamst;
  int mainPolicy = 0;
  pthread_getschedparam(pthread_self(), &mainPolicy, &mainParams);
  mainParams.sched_priority = sched_get_priority_max(SCHED_FIFO) - 2;
  pthread_setschedparam(pthread_self(), SCHED_FIFO, &mainParams);
  pthread_setschedprio(pthread_self(), sched_get_priority_max(SCHED_FIFO) - 2);

  // Ensures main thread values were set correctly
  int mainErr = pthread_getschedparam(pthread_self(), &mainPolicy, &mainParamst);
  if (mainErr != 0)
  {
    cout << "Setting priority failed for main thread." << endl;
    return 1;
  }
  if (mainPolicy != SCHED_FIFO)
  {
    cout << "Scheduling is not set to FIFO." << endl;
    return 1;
  }
  if (mainParamst.sched_priority != sched_get_priority_max(SCHED_FIFO) - 2)
  {
    cout << "Setting scheduler priority failed." << endl;
    return 1;
  }

  // Set priorities for each thread
  pthread_t threads[NUM_THREADS];
  struct sched_param threadParams[4];
  pthread_attr_t threadAttributes[4];
  int err;
  int policy;
  int prio;
  int inher;
  for (int i = 0; i < 4; ++i)
  {
    pthread_attr_init(&threadAttributes[i]);
    pthread_attr_getschedpolicy(&threadAttributes[i], &policy);
    pthread_attr_setschedpolicy(&threadAttributes[i], SCHED_FIFO);
    threadParams[i].sched_priority = sched_get_priority_max(SCHED_FIFO) - 10*i - 3;
    err = pthread_attr_setschedparam(&threadAttributes[i], &threadParams[i]);
    pthread_attr_setinheritsched(&threadAttributes[i], PTHREAD_EXPLICIT_SCHED);
    pthread_attr_getinheritsched(&threadAttributes[i], &inher);
    if (inher != PTHREAD_EXPLICIT_SCHED)
    {
      cout << "Failed to disable inheritance." << endl;
      return 1;
    }
    if (err == EINVAL)
    {
      cout << "Invalid priority for a thread." << endl;
      return 1;
    }
    else if (err == EPERM)
    {
      cout << "Insufficient permissions to set priority." << endl;
      return 1;
    }
    else
    {
      pthread_attr_getschedparam(&threadAttributes[i], &threadParams[i]);
      prio = threadParams[i].sched_priority;
      if (prio != sched_get_priority_max(SCHED_FIFO) - 10*i - 3)
      {
        cout << "Setting the priority of thread " << i << " failed." << endl;
        return 1;
      }
    }
  }

  // Set processor affinity
  cpu_set_t cpu[5];
  CPU_ZERO(&cpu[0]);
  CPU_SET(0, &cpu[0]);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu[0]);
  for (int i = 1; i < 5; ++i)
  {
    CPU_ZERO(&cpu[i]);
    CPU_SET(0, &cpu[i]);
    pthread_attr_setaffinity_np(&threadAttributes[i], sizeof(cpu_set_t), &cpu[i]);
  }

  // Initialize threads
  pthread_create(&threads[0], &threadAttributes[0], threadHandler, (void *)(intptr_t)1);
  pthread_create(&threads[1], &threadAttributes[1], threadHandler, (void *)(intptr_t)THREAD_2_RUNS);
  pthread_create(&threads[2], &threadAttributes[2], threadHandler, (void *)(intptr_t)THREAD_3_RUNS);
  pthread_create(&threads[3], &threadAttributes[3], threadHandler, (void *)(intptr_t)16);

  // Initialize timer to call the given function on this thread each time it expires
  pthread_attr_t clockAttr;
  pthread_attr_init(&clockAttr);

  cpu_set_t cpuClock;
  CPU_ZERO(&cpuClock);
  CPU_SET(0, &cpuClock);

  struct sched_param parameters;
  parameters.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
  pthread_attr_setschedparam(&clockAttr, &parameters);
  pthread_attr_setinheritsched(&clockAttr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setaffinity_np(&clockAttr, sizeof(cpu_set_t), &cpuClock);

  struct sigevent sig;
  sig.sigev_notify = SIGEV_THREAD;
  sig.sigev_notify_function = sigfunc;
  sig.sigev_value.sival_int = 0;
  sig.sigev_notify_attributes = &clockAttr;

  timer_t timerid;
  if (timer_create(CLOCK_REALTIME, &sig, &timerid) != 0)
  {
    cout << "The timer failed to initialize." << endl;
    return 1;
  }

  // Initialize the timer values; the starting time will last 10 milliseocnds once started, then the
  // timer will invoke the "sigfunc" method every 10 milliseconds until it is destroyed.

  struct itimerspec start, end;
  start.it_value.tv_sec = 0;
  start.it_value.tv_nsec = 10000000;
  start.it_interval.tv_sec = 0;
  start.it_interval.tv_nsec = 10000000;

  int deadline[4] = {0, 0, 0, 0};
  // Handles scheduling the threads
  timer_settime(timerid, 0, &start, &end);

  for (int i = 0; i < RUNTIME; ++i)
  {
    sem_post(&wakeupSchedule[0]);
    pthread_mutex_lock(&lock[0]);
    if (counter[0] < expected[0])
    {
      ++deadline[0];
    }
    pthread_mutex_unlock(&lock[0]);
    expected[0] += 1;
    if (i % 2 == 0)
    {
      sem_post(&wakeupSchedule[1]);
      pthread_mutex_lock(&lock[1]);
      if (counter[1] < expected[1])
      {
        ++deadline[1];
      }
      pthread_mutex_unlock(&lock[1]);
      expected[1] += 1;
    }
    if (i % 4 == 0)
    {
      sem_post(&wakeupSchedule[2]);
      pthread_mutex_lock(&lock[2]);
      if (counter[2] < expected[2])
      {
        ++deadline[2];
      }
      pthread_mutex_unlock(&lock[2]);
      expected[2] += 1;
    }
    if (i % 16 == 0)
    {
      sem_post(&wakeupSchedule[3]);
      pthread_mutex_lock(&lock[3]);
      if (counter[3] < expected[3])
      {
        ++deadline[3];
      }
      pthread_mutex_unlock(&lock[3]);
      expected[3] += 1;
    }
    sem_wait(&schedule);
  }

  // Method that can be used to test for cpu affinity being set properly
  /*
  for (int i = 0; i < 4; ++i)
  {
    cpu_set_t cput;
    CPU_ZERO(&cput);
    pthread_getaffinity_np(threads[i], sizeof(cpu_set_t), &cput);
    for (int j = 0; j < sysconf(_SC_NPROCESSORS_ONLN); ++j)
    {
      if (CPU_ISSET(j, &cput))
      {
        cout << "Thread " << i << " ran on CPU " << j << endl;
      }
    }
  }
  */

  // Closes out all timing mechanisms and merges the threads back into the parent.
  run = false;
  for (int i = 0; i < 4; ++i)
  {
    pthread_mutex_lock(&lock[i]);
    if (counter[i] < expected[i])
    {
      ++deadline[i];
    }
    pthread_mutex_unlock(&lock[i]);
    sem_post(&wakeupSchedule[i]);
  }
  timer_delete(timerid);
  pthread_attr_destroy(&clockAttr);
  pthread_join(threads[0], NULL);
  pthread_join(threads[1], NULL);
  pthread_join(threads[2], NULL);
  pthread_join(threads[3], NULL);
  sem_destroy(&schedule);
  for (int i = 0; i < 4; ++i)
  {
    sem_destroy(&wakeupSchedule[i]);
    pthread_mutex_destroy(&lock[i]);
    pthread_attr_destroy(&threadAttributes[i]);
  }
  // Display results
  for (int i = 0; i < 4; ++i)
  {
    cout << "Thread " << i+1 << ": Counter " << counter[i] << endl;
    cout << "Thread " << i+1 << " missed its deadline " << deadline[i] << " times." << endl;
  }

  return 0;
}
