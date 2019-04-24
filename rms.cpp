/*
Name: Trevor Kling
ID: 002270716
Email: kling109@mail.chapman.edu
Course: CPSC 380 Operating Systems
Last Date Modified: 22 April 2019
Project: Rate Monatonic Scheduler
*/

#include <iostream>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

using namespace std;

#define NUM_THREADS 4

int* BOARD[10];
chrono::milliseconds period(10);
sem_t wakeupSchedule;
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

void* threadHandler(void* number)
{
  int tnum = (int)(intptr_t)number;
  int pos;
  switch (tnum)
  {
    case 1: pos = 0;
            break;
    case 2: pos = 1;
            break;
    case 4: pos = 2;
            break;
    case 16: pos = 3;
             break;
    default: cout << "Received unexpected value." << endl;
             pos = -1;
  }
  while (pos != -1)
  {
    // Add synchronization mechanism here
    for (int i = 0; i < tnum; ++i)
    {
      doWork();
    }
    counter[pos] += 1;
  }
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
  if (sem_init(&wakeupSchedule, 0, 0) == -1)
  {
    cout << "The semaphore failed to initialize." << endl;
    return 1;
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
  pthread_t threads[NUM_THREADS];
  pthread_create(&threads[0], NULL, threadHandler, (void *)(intptr_t)1);
  pthread_create(&threads[1], NULL, threadHandler, (void *)(intptr_t)2);
  pthread_create(&threads[2], NULL, threadHandler, (void *)(intptr_t)4);
  pthread_create(&threads[3], NULL, threadHandler, (void *)(intptr_t)16);
  chrono::system_clock::time_point runtime = chrono::system_clock::now();
  for (int i = 0; i < 10; ++i)
  {
    // Add synchronization mechanism here
    for (int j = 0; j < 4; ++j)
    {
      if (counter[j] < expected[j])
      {
        cout << "Thread " << j << " missed a cycle." << endl;
      }
      expected[j] += 1;
    }
    runtime += 16 * period;
    this_thread::sleep_until(runtime);
  }
  for (int i = 0; i < 4; ++i)
  {
    cout << "Thread " << i+1 << ": Counter " << counter[i] << endl;
  }
  return 0;
}
