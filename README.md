# OS-Assignment-Extension

Extension Part of OS Assignment - Semester 242.

## Overview

This project implements the CFS (Completely Fair Scheduler) algorithm, which is a fair scheduling algorithm that allocates CPU time proportionally based on process niceness values. CFS is the default scheduler used in the Linux kernel since version 2.6.23.

## Setup and Execution

### Prerequisites

- GCC compiler
- Make build system
- POSIX-compatible operating system (Linux recommended)

### Compiling the Project with CFS Scheduler

```bash
make clean
CFS_SCHED=1 make
```

### Running Test Cases

#### Using the run.sh Script

The easiest way to run and analyze the CFS scheduler is by using the provided `run.sh` script:

```bash
chmod +x run.sh
./run.sh
```

This will run multiple test cases with different time quantum settings:
- `cfs_standard` (target latency = 4)
- `cfs_short_tq` (target latency = 2)
- `cfs_long_tq` (target latency = 8)

Output files will be saved to the `output/` directory.

#### Running a Specific Test Case

To run a specific test case with the CFS scheduler:

```bash
make clean
CFS_SCHED=1 make
./os <test_case_file>
```
For example, to run the `tc1` test case:

```bash
./os tc1
```

You can also redirect the output to a file:

```bash
./os <test_case_file> > output/<test_case_file>.out
```
For example:

```bash
./os tc1 > output/tc1.out
```


### Test Case Format

Each test case file follows this format:
```
<time_quantum> <num_cpus> <num_processes> 
<ram size> <swap size 0> <swap size 1> <swap size 2> <swap size 3>
<arrival_time> <process_name> <priority> <niceness>
<arrival_time> <process_name> <priority> <niceness>
...
```
Where:
- `priority` is the priority value of the process. It is used by the MLQ scheduler.
- `niceness` is used by CFS scheduler (range -20 to 10, lower value = higher priority)

Each process in file input/proc/<process_name> is defined as:
```
<default priority> <instruction_count>
<instruction_1>
<instruction_2>
...
<instruction_n>
```
Where:
- `<default priority>` is the default priority of the process (not used by CFS)
- `<instruction_count>` is the number of instructions to execute
- `<instruction_n>` is the instruction to execute (e.g., `calc`)

## Understanding CFS Output

The CFS scheduler output provides detailed information about scheduling decisions, including:

1. **Process Dispatch**: When a process is selected to run
   ```
   CPU 0: Dispatched process 1 (niceness: -10, weight: 2048.000000, vruntime: 0.000000, time_slice: 2)
   ```

2. **Process Context Switch**: When a process is put back in the run queue
   ```
   CPU 0: Put process 1 to run queue (niceness: -10, vruntime: 0.976562)
   ```

3. **Process Completion**: When a process finishes execution
   ```
   CPU 0: Processed 1 has finished (niceness: -10, vruntime: 4.882812)
   ```

Key metrics in the output:
- **niceness**: Process priority (-20 to 10, lower is higher priority).
- **weight**: Calculated from niceness, determines CPU time allocation.
- **vruntime**: Virtual runtime tracking fairness (increases more slowly for higher priority processes).
- **time_slice**: Dynamically calculated execution time for the process.
