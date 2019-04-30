# RateMonatonicScheduler

Trevor Kling
29 April 2019

## Introduction

Implements a sample rate monotonic scheduler in C++.  The scheduler handles 4 threads, dispatching them all to a single
CPU core and being checking if they meet the schedule or not.  Each thread does arbitrary work while running.

## Implementation

Compile on the Linux Command Line with "g++ rms.cpp -lpthread -lrt".  The program is implemented in C++ using pthreads and
unix standard libraries.  The main thread becomes a "scheduler," which handles dispatching the threads by incrementing semaphores.
The scheduler itself is managed by a CPU timer, which also controls a semaphore for the scheduler.  The threads are also all assigned to a single core, the first core of the CPU.  The priorities of the threads are set by their period, in the following order:

1. CPU Timer
2. Scheduler
3. Thread of period 1
2. Thread of period 2
3. Thread of period 4
5. Thread of period 16

## Known Issues

Never actually allows the lower-priority threads to run when set to priority FIFO.

## Sources

http://devlib.symbian.slions.net/s3/GUID-B4039418-6499-555D-AC24-9B49161299F2.html - Timer workings

https://www.geeksforgeeks.org/mutex-lock-for-linux-thread-synchronization/ - Mutex Locks syntax

https://www.geeksforgeeks.org/use-posix-semaphores-c/ - Semaphore syntax

https://docs.oracle.com/cd/E19455-01/806-5257/attrib-16/index.html - Thread Priorities
