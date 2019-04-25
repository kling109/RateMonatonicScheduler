# RateMonatonicScheduler

## Introduction

## Implementation

Should See:
Thread1: 160 runs
Thread2: 80 runs
Thread3: 40 runs
Thread4: 10 runs


Use a timer (calls a function) to wake up the scheduler (Much more precise) (Get other things working first)


Don't preempt threads, just increment a semaphore and allow to run again


Will need 4 semaphores; one for each thread


Add priorities


Add mutex to "counter"


Scheduler must run at least as fast as Thread1
## Known Issues

## Sources
