#!/bin/bash
CPPCHECK=cppcheck
# building cppcheck from github does not increase findings.
#CPPCHECK=${HOME}/projects/cppcheck/build/bin/cppcheck
CPPCHECKOPTS="--cppcheck-build-dir=cppcheck-build --addon=misra --language=c++ --std=c++20 --enable=all --inconclusive --safety --platform=unix64 --checkers-report=report.txt --suppress=misra-config --suppress=missingIncludeSystem"
INC_PATH_ARGS="-Inode1/src -Inode2/src -Ilibraries/CryptoComm/include -Ilibraries/Helper/include"
CPPCHECKFILES="libraries/CryptoComm/src/BTListenSocket.cpp \
    libraries/CryptoComm/src/BTConnection.cpp \
    libraries/CryptoComm/src/CryptoWrapper.cpp \
    libraries/Helper/src/Helper.cpp \
    node1/src/main.cpp \
    node1/src/Sensor.cpp \
    node1/src/CommThread.cpp \
    node1/src/SensorThread.cpp \
    node2/src/main.cpp \
    node2/src/MainWindow.cpp \
    node2/src/ACCThread.cpp \
    node2/src/CommThread.cpp"

mkdir -p cppcheck-build

#
# Runs cppcheck including MISRA rules on the acc source code
#
${CPPCHECK} ${CPPCHECKOPTS} ${INC_PATH_ARGS} ${CPPCHECKFILES} ${HDRCHECKFILES}
