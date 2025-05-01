#!/bin/bash

# Script to switch between CFS and MLQ scheduling in OS simulator
# without manually editing header files

# ANSI color codes for better output formatting
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${CYAN}OS Scheduler Switcher${NC}"
echo "========================"

# Check if input directory exists
if [ ! -d "input" ]; then
    echo -e "${RED}Error: 'input' directory not found!${NC}"
    echo "Please run this script from the project's root directory."
    exit 1
fi

# Function to backup the original header files if not already backed up
backup_headers() {
    if [ ! -f "include/os-cfg.h.orig" ]; then
        cp include/os-cfg.h include/os-cfg.h.orig
    fi
    
    if [ ! -f "include/sched.h.orig" ]; then
        cp include/sched.h include/sched.h.orig
    fi
}

# Function to enable CFS scheduling
enable_cfs() {
    echo -e "${YELLOW}Enabling CFS scheduling...${NC}"
    
    # Create a new os-cfg.h file with CFS enabled
    cat > include/os-cfg.h << EOF
#ifndef OSCFG_H
#define OSCFG_H

// #define MLQ_SCHED 1
#define CFS_SCHED 2
#define MAX_PRIO 140

#define MM_PAGING
//#define MM_FIXED_MEMSZ
//#define VMDBG 1
//#define MMDBG 1
#define IODUMP 1
#define PAGETBL_DUMP 1

#endif
EOF

    # Create a new sched.h file with CFS enabled
    cat > include/sched.h << EOF
#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

// #ifndef MLQ_SCHED
// #define MLQ_SCHED
// #endif

#ifndef CFS_SCHED
#define CFS_SCHED
#endif

#define MAX_PRIO 140

int queue_empty(void);

void init_scheduler(void);
void finish_scheduler(void);

/* Get the next process from ready queue */
struct pcb_t * get_proc(void);

/* Put a process back to run queue */
void put_proc(struct pcb_t * proc);

/* Add a new process to ready queue */
void add_proc(struct pcb_t * proc);

#ifdef CFS_SCHED
void update_vruntime(struct pcb_t * proc, uint32_t exec_time);
#endif

#ifdef CFS_SCHED
uint32_t get_min_vruntime(void);
#endif

#endif
EOF
    
    echo -e "${GREEN}CFS scheduling enabled${NC}"
}

# Function to enable MLQ scheduling
enable_mlq() {
    echo -e "${YELLOW}Enabling MLQ scheduling...${NC}"
    
    # Create a new os-cfg.h file with MLQ enabled
    cat > include/os-cfg.h << EOF
#ifndef OSCFG_H
#define OSCFG_H

#define MLQ_SCHED 1
// #define CFS_SCHED 2
#define MAX_PRIO 140

#define MM_PAGING
//#define MM_FIXED_MEMSZ
//#define VMDBG 1
//#define MMDBG 1
#define IODUMP 1
#define PAGETBL_DUMP 1

#endif
EOF

    # Create a new sched.h file with MLQ enabled
    cat > include/sched.h << EOF
#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

#ifndef MLQ_SCHED
#define MLQ_SCHED
#endif

// #ifndef CFS_SCHED
// #define CFS_SCHED
// #endif

#define MAX_PRIO 140

int queue_empty(void);

void init_scheduler(void);
void finish_scheduler(void);

/* Get the next process from ready queue */
struct pcb_t * get_proc(void);

/* Put a process back to run queue */
void put_proc(struct pcb_t * proc);

/* Add a new process to ready queue */
void add_proc(struct pcb_t * proc);

#ifdef CFS_SCHED
void update_vruntime(struct pcb_t * proc, uint32_t exec_time);
#endif

#ifdef CFS_SCHED
uint32_t get_min_vruntime(void);
#endif

#endif
EOF
    
    echo -e "${GREEN}MLQ scheduling enabled${NC}"
}

# Function to restore the original header files
restore_headers() {
    if [ -f "include/os-cfg.h.orig" ] && [ -f "include/sched.h.orig" ]; then
        cp include/os-cfg.h.orig include/os-cfg.h
        cp include/sched.h.orig include/sched.h
        echo -e "${BLUE}Original header files restored${NC}"
    fi
}

# Function to compile and run with specified scheduler
run_test() {
    local scheduler=$1
    local input_file=$2
    local output_file=$3
    
    echo -e "\n${YELLOW}Compiling with $scheduler scheduling...${NC}"
    make clean && make
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Compilation failed!${NC}"
        return 1
    fi
    
    echo -e "\n${GREEN}Compilation successful!${NC}"
    
    # Create output directory if it doesn't exist
    mkdir -p output
    
    # Run the test
    echo -e "\n${YELLOW}Running $scheduler scheduler for $input_file...${NC}"
    
    # Check if input file exists
    if [ ! -f "input/$input_file" ]; then
        echo -e "${RED}Error: Input file '$input_file' not found!${NC}"
        return 1
    fi
    
    # Run with more verbose output
    echo -e "${BLUE}Command: ./os $input_file > output/$output_file${NC}"
    ./os $input_file > output/$output_file
    
    # Check exit status
    if [ $? -ne 0 ]; then
        echo -e "${RED}Warning: Test execution returned non-zero exit code. Check the output file for details.${NC}"
        echo -e "${YELLOW}Continuing with tests anyway...${NC}"
    else
        echo -e "\n${GREEN}Test completed successfully!${NC}"
    fi
    
    echo -e "Output saved to: ${CYAN}output/$output_file${NC}"
}

# Main execution logic
if [ $# -lt 1 ]; then
    echo -e "${RED}Error: Missing input file!${NC}"
    echo "Usage: $0 <input_file> [more input files...]"
    exit 1
fi

# Backup the original header files
backup_headers

# Flag to track if any test compilation or execution failed
any_failure=0

# Run each input file with both schedulers
for input_file in "$@"; do
    filename=$(basename "$input_file")
    
    # Run with MLQ scheduler
    enable_mlq
    run_test "MLQ" "$input_file" "${filename}-mlq.out"
    if [ $? -ne 0 ]; then
        any_failure=1
    fi
    
    # Run with CFS scheduler
    enable_cfs
    run_test "CFS" "$input_file" "${filename}-cfs.out"
    if [ $? -ne 0 ]; then
        any_failure=1
    fi
    
    echo -e "\n${CYAN}=====================================${NC}"
    echo -e "${GREEN}Finished testing ${YELLOW}$input_file${GREEN} with both schedulers!${NC}"
    echo -e "${CYAN}=====================================${NC}"
done

# Restore headers to original state when done
restore_headers

if [ $any_failure -eq 1 ]; then
    echo -e "\n${YELLOW}Some tests encountered issues. Please check the logs above for details.${NC}"
else
    echo -e "\n${GREEN}All tests completed successfully!${NC}"
fi