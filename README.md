# Assignment 2: MTS 

A project showcasing a Multi-Thread Scheduling (MTS) system, utilizing C and the pthread library to create a process schedular based on various rules and specificaitons.


**FOR THE TA, ASSIGNMENT HAS BEEN FULLY IMPLEMENTED CORRECTLY** 

## Table of Contents

- [Credentials](#credentials)
- [Installation](#installation)
- [Usage](#usage)
- [Formating the input file](#formatting-the-input-file)
- [Example input and output](#example-input-and-output)


## Credentials

- NAME: Luke Edwards
- V#: V01007316
- CLASS: CSC 360 A02

## Installation

### Files:
- **mts.c**: Main() process acts as the thread dispatcher, deploying and deciding when threads should be scheduled.
- **train_thread.c**: thread_thread() acts as the logic that will be executed by each train, including waiting to start and cross, adding itself to its corresponding queue, and handling crossing then signalling the dispacther once complete.
- **train_thread.h**: Containing struct data for queues, trains, and starvation resolution package. Also defines the global variables, mutexes and convars needed by both .c files for communication.
- **Makefile**: Compilation commands, use "make all" and "make clean".
- **input.txt**: Data containing threads that need to be scheduled. *NOTE: YOU MAY MAKE YOUR OWN FILE FOR INPUT*
- **output.txt**: Where the output of the file goes, summary of scheduling operations after process is complete.

### File structure:

REQUIRED FILES FOR COMPILATION:
- mtc.c
- output.txt
- input.txt
- Makefile

OTHER FILES:
- README.md

### Libraries used:

- stdio.h
- stdlib.h
- string.h
- signal.h
- unistd.h
- pthread.h
- stdbool.h

## Usage

In order to execute the program, make sure you are first in the directory that contains all the files.
```
cd ../../../p2
```
Next, use the makefile to compile the necessary files.
```
make all
```
Now execute the output file, /*mts* alongside the path to the input file (does not need to be named input.txt BUT MUST BE A TXT FILE).
```
./mts input.txt
```
Now the output should be visible in the output.txt file.

## Formating the input file

Interface with the program by using the input.txt file to dictate how many train you wish to test and their behavior.
The format of the file must be as follows:
```
X Y Z
X Y Z
X Y Z
```
... and so on
Where,

- X represents the direction and priority of the train.
    - 'E' refers to "East" and "W" refers to "West".
    - Uppercase letter means it is high priority, lowercase means it is low priority.
- Y represents the arrival time of the train in milliseconds.
- Z represents the crossing time of the train in milliseconds once it is signaled to cross.
- Each new line represents a unique train.


#### Example input and output:

In input.txt:
```
e 5 1
w 1 6
W 2 1
W 3 1
```
The result in output.txt:
```
00:00:00.001 Train 1 is ready to go West
00:00:00.001 Train 1 is ON the main track going West
00:00:00.002 Train 2 is ready to go West
00:00:00.003 Train 3 is ready to go West
00:00:00.005 Train 0 is ready to go East
00:00:00.008 Train 1 is OFF the main track going West
00:00:00.008 Train 2 is ON the main track going West
00:00:00.010 Train 2 is OFF the main track going West
00:00:00.010 Train 0 is ON the main track going East
00:00:00.011 Train 0 is OFF the main track going East
00:00:00.011 Train 3 is ON the main track going West
00:00:00.012 Train 3 is OFF the main track going West
```
