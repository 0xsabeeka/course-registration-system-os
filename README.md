# Course Registration System using POSIX Threads

A C-based Operating Systems project that simulates a university course registration system using POSIX threads and mutex synchronization.

This project demonstrates how multiple student registration requests can be handled concurrently while protecting shared course seat data from race conditions.

## Project Overview

In a real university registration system, many students may try to register for courses at the same time. Since each course has limited seats, concurrent access can cause incorrect seat allocation if synchronization is not used.

This project solves the problem by representing each student as a separate thread. Each course has its own mutex lock, so only one student thread can update the seat count of a course at a time.

## Features

* Course registration simulation in C
* Multiple student threads using POSIX threads
* Mutex locks for synchronization
* Protection of shared course seat data
* Race condition prevention
* Priority handling for high-priority students
* Deadlock prevention by locking only one course at a time
* Mandatory test scenario
* Stress test scenario
* Registration logging in `log.txt`
* Final summary showing successful and failed registrations

## Operating Systems Concepts Used

* Threads
* POSIX Threads
* Mutex Locks
* Synchronization
* Critical Sections
* Race Conditions
* Deadlock Prevention
* Priority Handling
* Thread Joining
* Logging

## Technologies Used

* C
* POSIX Threads
* GCC
* Ubuntu/Linux Terminal

## Project Structure

```text
course-registration-system-os/
│
├── project.c
└── README.md
```

## How to Compile

Use the following command on Ubuntu/Linux:

```bash
gcc project.c -o project -pthread
```

The `-pthread` flag is required because the program uses POSIX threads and mutex locks.

## How to Run

After compiling, run:

```bash
./project
```

## Output

The program displays:

* Registration attempts
* Student ID
* Course ID
* Course name
* Priority level
* Success or failure result
* Final seat allocation summary
* Total successful registrations
* Total failed registrations
* Success rate

The program also creates a log file:

```text
log.txt
```

This file stores registration logs and final summaries.

## Test Scenarios

The project includes:

### Mandatory Test

* 10 students
* 3 courses
* High-priority students
* Limited seats

### Stress Test

* 50 students
* 10 courses
* High-priority and normal-priority students
* Higher concurrency to test synchronization

## Conclusion

This project shows how OS synchronization concepts can be applied to a real-world style course registration problem. Mutex locks protect shared seat data, prevent race conditions, and ensure that course seats are allocated safely without exceeding capacity.
