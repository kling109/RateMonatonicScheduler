# Rate Monotonic Scheduler Design

Trevor Kling
1 May 2019
CPSC 380: Operating Systems

## Basic Design
The Rate-Monotonic Scheduler was designed in C++, using the pthreads library and running
on the Linux command line.  Specifically, the program was tested on Ubuntu 18.04, compiled
on the command line with g++.

The program begins with the main thread, which functions to both set the parameters of the
worker threads as well as functioning as the scheduler.  The program begins by setting the
main thread properties; the main is set to schedule as a priority-preemptive FIFO thread
with a priority of 97.  The max priority of any thread in the SCHED_FIFO scheduling
option is 99.  The main then sets the properties of 4 worker threads; each thread is given
a priority less than the one before it.  The first thread, intended to run with a period of 1, is given a priority of 96.  The second thread, intended to run with a period of 2, is
given a priority of 86.  The third thread, intended to run with a period of 4, is given
a priority of 76.  Finally, the last thread is intended to run with a period of 16 and is
given a priority of 66.  The scheduler is given its timing by a CPU timer, which is also
given its own thread with priority 98.  This ensure that the hierarchy of threads goes from
highest period to lowest period, with all workers being preceded by the scheduling.

## Synchronization and Dispatch
To synchronize the threads, a set of 5 semaphores were used.  The sempahores all begin with
a value of 0, and when posted the value is incremented by 1.  The worker threads wait until their semaphore is incremented, then run doWork for a pre-set number of iterations.  Meanwhile, the scheduler main thread also waits on a semaphore.  The post for this semaphore is called periodically by the timer, specifically every 10 ns.  The main scheduler then runs for 160 iterations, scheduling threads at the prescribed intervals by signaling their semaphores.  Additionally, the counters maintained by the threads and checked by the scheduler are protected by a priority-inheriting mutex.
