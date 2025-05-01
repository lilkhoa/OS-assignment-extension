#!/bin/bash

# Script to simulate CFS scheduling in OS simulator
# Created for OS-assignment-extension

# ANSI color codes for better output formatting
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${CYAN}CFS Scheduler Simulation Script${NC}"
echo "=========================================="

# Check if input directory exists
if [ ! -d "input" ]; then
    echo -e "${RED}Error: 'input' directory not found!${NC}"
    echo "Please run this script from the project's root directory."
    exit 1
fi

# Create output directory if it doesn't exist
mkdir -p output

echo -e "${BLUE}Created test configuration with 5 processes:${NC}"
echo "  - p1: Burst time 8, Niceness -10 (highest priority)"
echo "  - p7: Burst time 10, Niceness -5"
echo "  - p2: Burst time 12, Niceness 0"
echo "  - p3: Burst time 9, Niceness 5"
echo "  - p5: Burst time 15, Niceness 10 (lowest priority)"
echo -e "${BLUE}Check the input/proc directory for process information.${NC}"

# Function to run a test case with CFS scheduler
run_cfs_test() {
    local test_name=$1
    local time_quantum=$2
    local cpu_count=$3
    
    echo -e "\n${YELLOW}Creating test case: ${test_name}${NC}"
    
    # Create the test case file with both priority and niceness values
    cat > input/${test_name} << EOF
${time_quantum} ${cpu_count} 5
2048 16777216 0 0 0
1 p1 0 -10
2 p7 1 -5
3 p2 2 0
4 p3 3 5
5 p5 4 10
EOF
    
    # Run with CFS scheduler - Make sure to properly clean and rebuild
    echo -e "\n${YELLOW}Running with CFS scheduler...${NC}"
    make clean > /dev/null
    CFS_SCHED=1 make > /dev/null
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Compilation failed for CFS scheduler!${NC}"
        exit 1
    fi
    
    ./os ${test_name} > output/${test_name}.out
    echo -e "${GREEN}CFS test completed. Output saved to: ${CYAN}output/${test_name}.out${NC}"
}

# Run tests with different parameters
echo -e "\n${CYAN}Running CFS Scheduler Tests${NC}"

# Test with different parameters
run_cfs_test "cfs_standard" 4 1  # Standard time quantum, single CPU
run_cfs_test "cfs_short_tq" 2 1  # Short time quantum, single CPU
run_cfs_test "cfs_long_tq" 8 1   # Long time quantum, single CPU

echo -e "\n${CYAN}All CFS tests completed!${NC}"
echo -e "${YELLOW}You can find detailed outputs in the output/ directory.${NC}"
echo -e "${GREEN}Review the results above to understand CFS scheduling behavior.${NC}"