#!/bin/bash
CPPCHECK=cppcheck
CPPCHECKOPTS="--addon=misra --check-level=normal --enable=all --inconclusive --safety --checkers-report=report.txt --language=c++ --std=c++20"
CPPCHECKFOLDERS="libraries node1 node2"
CPPCHECKEXTENSIONS='-name "*.cpp" -o -name "*.c" -o -name "*.h"'

# Files to check in our project, exclude everything in "test" folders
CPPCHECKFILES=$(find ${CPPCHECKFOLDERS} -name "*.cpp" | grep -v "/test/") 
HDRCHECKFILES=$(find ${CPPCHECKFOLDERS} -name "*.h" | grep -v "/test/")

# Folders to find #included header files in
INCLUDE_PATHS=$(find build ${CPPCHECKFOLDERS} -name "*.h" -o -name "*.hpp" | xargs dirname | sort | uniq)
INC_PATH_ARGS=""
for include_path in ${INCLUDE_PATHS}; do
    INC_PATH_ARGS="${INC_PATH_ARGS} -I${include_path}"
done

# gcc library paths
INC_PATHS_GCC=$(gcc --print-search-dirs | grep "libraries" | sed -E "s|[^=]+=(.*)|\1|")
INC_PATHS_GCC_SPLIT=$(echo $INC_PATHS_GCC | tr ":" "\n")
for include_path in ${INC_PATHS_GCC_SPLIT}; do
    INC_PATH_ARGS="${INC_PATH_ARGS} -I${include_path}"
done

INC_PATH_ARGS="${INC_PATH_ARGS} -I/usr/include/c++/14/algorithm -I/usr/include/c++/14/ext/algorithm -I/usr/include"

#
# Runs cppcheck including MISRA rules on the acc source code
#
echo ${CPPCHECK} ${CPPCHECKOPTS} ${INC_PATH_ARGS} ${CPPCHECKFILES} ${HDRCHECKFILES}
${CPPCHECK} ${CPPCHECKOPTS} ${INC_PATH_ARGS} ${CPPCHECKFILES} ${HDRCHECKFILES}
