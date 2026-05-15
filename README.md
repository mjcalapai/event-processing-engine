# Multi-Threaded Security Event Processing Engine

This project is a high-performance, multi-threaded security event processing engine designed to simulate a real-world Security Information and Event Management (SIEM) or Intrusion Detection System (IDS) pipeline. It uses a Producer-Consumer architecture to ingest, classify, prioritize, and correlate security logs in real-time.

## Features

- **Producer-Consumer Architecture:** Utilizes multi-threading to handle high-throughput log ingestion and processing.
- **Priority Scheduling:** Features a Multi-Level Feedback Queue (MLFQ) with Weighted Round-Robin (WRR) scheduling to ensure critical security alerts are processed swiftly while preventing starvation of lower-priority logs through an aging mechanism.
- **Real-Time Threat Correlation:** A stateful Correlation Engine detects complex attack patterns across a sliding time window (e.g., burst failed logins, port scans).
- **Concurrency Control:** Thread-safe communication via a custom `BoundedBuffer` using `pthread` mutexes and condition variables.
- **Performance Metrics:** Real-time tracking of throughput, latency by severity level (measured in nanoseconds), and scheduler performance.

## Architecture

1. **Log Generation:** `generate_logs.cpp` creates realistic, type-coherent synthetic security logs (DEBUG, INFO, WARNING, ERROR, CRITICAL).
2. **Producers:** Read logs from the file system, parse them, classify their severity, and push them into the shared buffer.
3. **Schedulers:** 
   - `FIFO`: First-In-First-Out processing.
   - `WEIGHTED`: MLFQ scheduler that prioritizes CRITICAL events while maintaining fairness via WRR and starvation prevention.
4. **Consumers:** Pop logs from the scheduler, run them through the Correlation Engine, and record metrics.
5. **Correlation Engine:** Tracks state to identify attacks like repeated failed logins from a single IP or sequential port scanning.

## Requirements

- C++17 compiler (e.g., `g++` or `clang++`)
- POSIX Threads (`pthread`)
- Google Test (`googletest`) for the unit testing suite

## Build Instructions

To compile the application, the log generator, and the test suite, simply run:

```bash
make
```

This will produce three executables:
- `event_engine_app`: The main processing engine.
- `generate_logs`: The utility to create test data.
- `event_engine_test`: The unit test suite.

To clean up object files and executables:
```bash
make clean
```

## Usage

### 1. Generate Log Data
Before running the engine, you must generate a synthetic log file. 
```bash
./generate_logs <number_of_logs> <output_filename>
```
*Example: Generate 100,000 logs into `logs.txt`*
```bash
./generate_logs 100000 logs.txt
```

### 2. Run the Engine
Execute the engine by specifying the scheduling policy, number of producers, number of consumers, buffer size, and the input file.

```bash
./event_engine_app <fifo|weighted|rr> <num_producers> <num_consumers> <buffer_capacity> <log_file>
```
*Example: Run with the MLFQ (weighted) scheduler, 2 producers, 4 consumers, a buffer of 1000, reading `logs.txt`*
```bash
./event_engine_app weighted 2 4 1000 logs.txt
```

### 3. Run the Unit Tests
The project includes a robust suite of unit tests verifying the bounded buffer, MLFQ scheduling logic, threat correlation accuracy, and thread-safety.
```bash
./event_engine_test
```

## Understanding the Metrics Output

Upon completion, the engine will output detailed performance metrics:
- **Throughput:** Number of events processed per second.
- **Average Latency:** The end-to-end processing time (in nanoseconds) from ingestion to analysis.
- **Latency by Severity:** Detailed breakdown of wait times. When using `weighted` scheduling, high-severity logs may be prioritized, depending on system contention.
- **Alerts:** Total number of correlated attacks detected (e.g., brute force, port scans).
- **Aging Boosts:** The number of times the scheduler evaluated queues to promote lower-priority logs and prevent starvation.