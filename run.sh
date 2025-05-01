#!/bin/bash

# Script to test CFS scheduler with preemption in OS simulator
# Created for OS-assignment-extension

# ANSI color codes for better output formatting
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${CYAN}CFS Scheduler Preemption Test Script${NC}"
echo "=========================================="

# Check if input directory exists
if [ ! -d "input" ]; then
    echo -e "${RED}Error: 'input' directory not found!${NC}"
    echo "Please run this script from the project's root directory."
    exit 1
fi

# Create a test configuration file for CFS scheduler with preemption test
cat > input/sched_cfs_preemption << EOF
3 1 3
2048 16777216 0 0 0
0 p1 -10
5 p2 0
10 p3 -5
EOF

echo -e "${BLUE}Created test configuration with 3 processes:${NC}"
echo "  - p1: niceness -10 (high priority), starts at time 0"
echo "  - p2: niceness 0 (normal priority), starts at time 5"
echo "  - p3: niceness -5 (medium priority), starts at time 10"
echo "This will demonstrate preemption when higher priority processes arrive."

# Compile the OS simulator
echo -e "\n${YELLOW}Compiling the OS simulator...${NC}"
make clean && make CFS_SCHED=1

if [ $? -ne 0 ]; then
    echo -e "${RED}Compilation failed!${NC}"
    exit 1
fi

echo -e "\n${GREEN}Compilation successful!${NC}"

# Create output directory if it doesn't exist
mkdir -p output

# Run the test
echo -e "\n${YELLOW}Running CFS scheduler preemption test...${NC}"
echo -e "${BLUE}Test output:${NC}"
echo "----------------------------------------"
./os sched_cfs_preemption > output/sched_cfs_preemption.output
cat output/sched_cfs_preemption.output
echo "----------------------------------------"

# Check if test completed successfully
if [ $? -ne 0 ]; then
    echo -e "${RED}Test execution failed!${NC}"
    exit 1
fi

echo -e "\n${GREEN}Test completed successfully!${NC}"
echo -e "Output saved to: ${CYAN}output/sched_cfs_preemption.output${NC}"

# Analyze the preemption behavior
echo -e "\n${YELLOW}Analyzing preemption results...${NC}"
echo "Checking for preemption patterns in the output:"

# Look for cases where processes are preempted due to higher priority
PREEMPTIONS=$(grep -i "preempted" output/sched_cfs_preemption.output | wc -l)

if [ $PREEMPTIONS -gt 0 ]; then
    echo -e "${GREEN}Found $PREEMPTIONS preemption events in the execution log.${NC}"
    echo "This indicates the preemption mechanism is working correctly."
else
    echo -e "${RED}No explicit preemption events found in the execution log.${NC}"
    echo "You may need to review the code or the test parameters."
fi

echo -e "\n${CYAN}Test summary:${NC}"
echo "- Check the output for processes with lower niceness (higher priority) getting CPU time"
echo "- Verify that when a high priority process arrives, it preempts lower priority processes"
echo "- Confirm that vruntime calculations include the priority bonus mechanism"

echo -e "\n${GREEN}Testing complete!${NC}"