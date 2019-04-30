/*
Name: Trevor Kling
ID: 002270716
Email: kling109@mail.chapman.edu
Course: CPSC 380 Operating Systems
Last Date Modified: 29 April 2019
Project: Rate Monatonic Scheduler
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
    case 10000000: pos = 1;
            break;
    case 4: pos = 2;
            break;
    case 16: pos = 3;
             break;
    default: cout << "Received unexpected value." << endl;
             pos = -1;
  }
  cout << "Here" << endl;
  sem_wait(&wakeupSchedule[pos]);
  while (pos != -1 && run == true)
  {
    cout << "Running " << pos << endl;
    for (int i = 0; i < tnum; ++i)
    {
      doWork();
    }
    cout << "Finished " << pos << endl;
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
    if (pthread_mutex_init(&lock[i], NULL) != 0)
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
  struct sched_param mainParams;
  mainParams.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
  pthread_setschedparam(pthread_self(), SCHED_FIFO, &mainParams);

  // Set priorities for each thread
  pthread_t threads[NUM_THREADS];
  struct sched_param threadParams[4];
  pthread_attr_t threadAttributes[4];
  for (int i = 0; i < 4; ++i)
  {
    pthread_attr_init(&threadAttributes[i]);
    pthread_attr_setinheritsched(&threadAttributes[i], PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&threadAttributes[i], SCHED_OTHER);
    threadParams[i].sched_priority = sched_get_priority_max(SCHED_OTHER) - 1*i - 2;
    pthread_attr_setschedparam(&threadAttributes[i], &threadParams[i]);
  }

  // Initialize threads
  pthread_create(&threads[0], &threadAttributes[0], threadHandler, (void *)(intptr_t)1);
  pthread_create(&threads[1], &threadAttributes[1], threadHandler, (void *)(intptr_t)10000000);
  pthread_create(&threads[2], &threadAttributes[2], threadHandler, (void *)(intptr_t)4);
  pthread_create(&threads[3], &threadAttributes[3], threadHandler, (void *)(intptr_t)16);

  // Set processor affinity
  cpu_set_t cpu;
  CPU_ZERO(&cpu);
  CPU_SET(2, &cpu);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu);
  for (int i = 0; i < 4; ++i)
  {
    pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpu);
  }

  // Initialize timer to call the given function on this thread each time it expires
  pthread_attr_t attributes;
  pthread_attr_init(&attributes);

  struct sched_param parameters;
  parameters.sched_priority = sched_get_priority_max(SCHED_FIFO);
  pthread_attr_setschedpolicy(&attributes, SCHED_FIFO);
  pthread_attr_setschedparam(&attributes, &parameters);

  struct sigevent sig;
  sig.sigev_notify = SIGEV_THREAD;
  sig.sigev_notify_function = sigfunc;
  sig.sigev_value.sival_int = 0;
  sig.sigev_notify_attributes = &attributes;

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

  // Handles scheduling the threads
  timer_settime(timerid, 0, &start, &end);
  for (int i = 0; i < RUNTIME; ++i)
  {
    sem_post(&wakeupSchedule[0]);
    pthread_mutex_lock(&lock[0]);
    if (counter[0] < expected[0])
    {
      cout << "Thread " << 0 << " missed a cycle." << endl;
    }
    pthread_mutex_unlock(&lock[0]);
    expected[0] += 1;
    if (i % 2 == 0)
    {
      sem_post(&wakeupSchedule[1]);
      pthread_mutex_lock(&lock[1]);
      if (counter[1] < expected[1])
      {
        cout << "Thread " << 1 << " missed a cycle." << endl;
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
        cout << "Thread " << 2 << " missed a cycle." << endl;
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
        cout << "Thread " << 3 << " missed a cycle." << endl;
      }
      pthread_mutex_unlock(&lock[3]);
      expected[3] += 1;
    }
    sem_wait(&schedule);
  }

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

  // Closes out all timing mechanisms and merges the threads back into the parent.
  run = false;
  for (int i = 0; i < 4; ++i)
  {
    sem_post(&wakeupSchedule[i]);
  }
  timer_delete(timerid);
  pthread_attr_destroy(&attributes);
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

  for (int i = 0; i < 4; ++i)
  {
    cout << "Thread " << i+1 << ": Counter " << counter[i] << endl;
  }

  return 0;
}
