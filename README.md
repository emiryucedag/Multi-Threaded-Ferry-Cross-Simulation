#  Ferry Crossing Simulation (Multi-threaded)

This project implements a solution to the **Ferry Crossing Problem** using **C** and **POSIX Threads (pthreads)**. It simulates the synchronization between a ferry and cars using semaphores and mutexes, ensuring safe concurrent access to shared resources.

This implementation is designed to work on **macOS** (using named semaphores) and **Linux**.

##  Overview

The program simulates a ferry system with the following rules:
- **Capacity:** The ferry has a fixed capacity of **5 cars**.
- **Boarding:** The ferry waits until it is full (5 cars) before departing.
- **Crossing:** The travel time takes a simulated 3 seconds.
- **Unboarding:** Cars must wait for the ferry to dock before leaving.
- **Continuous Loop:** Cars that leave the ferry wait for a random interval and queue up again (Infinite Loop).
- **Termination:** The simulation runs for exactly **60 seconds** and terminates gracefully.

##  Technologies & Concepts

- **Language:** C (C99 Standard)
- **Concurrency:** `pthread` library
- **Synchronization:**
  - **Semaphores:** For signaling between the ferry and cars (Boarding, Full, Unboard, Empty).
  - **Mutexes:** To protect shared variables (e.g., car counter).
- **Time Management:** `gettimeofday` for high-precision logging and `SIGALRM` for precise termination.
- **Build System:** `Makefile`



