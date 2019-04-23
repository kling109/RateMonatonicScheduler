/*
Name: Trevor Kling
ID: 002270716
Email: kling109@mail.chapman.edu
Course: CPSC 380 Operating Systems
Last Date Modified: 22 April 2019
Project: Rate Monatonic Scheduler
*/

#include <iostream>
#include <chrono>
#include <thread>
#include <pthread.h>

using namespace std;

#define NUM_THREADS 4

int* BOARD[10];
chrono::milliseconds period(10);
sem_t wakeupSchedule;
int t1 = 0;
int t2 = 0;
int t3 = 0;
int t4 = 0;

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
  pthread_t threads[NUM_THREADS];
  int t1Expected = 0;
  int t2Expected = 0;
  int t3Expected = 0;
  int t4Expected = 0;


  std::this_thread::sleep_for(16 * period);

  return 0;
}
