cmake_minimum_required(VERSION 3.22)
project(acc VERSION 0.0.1 LANGUAGES CXX)

Include(FetchContent)
Include(CTest)

#
# For checking out Catch2 sources
#
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.10.0 # or a later release
)

FetchContent_MakeAvailable(Catch2)

#
# Getting include paths, link paths, library names for pre-installed libraries
#
find_package(PkgConfig REQUIRED)
pkg_check_modules(BLUEZ REQUIRED bluez)
pkg_check_modules(GIO REQUIRED gio-2.0)

message("BLUEZ_LIBRARIES: ${BLUEZ_LIBRARIES}")
message("BLUEZ_LIBRARY_DIRS: ${BLUEZ_LIBRARY_DIRS}")
message("BLUEZ_INCLUDE_DIRS: ${BLUEZ_INCLUDE_DIRS}")

message("GIO_LIBRARIES: ${GIO_LIBRARIES}")
message("GIO_LIBRARY_DIRS: ${GIO_LIBRARY_DIRS}")
message("GIO_INCLUDE_DIRS: ${GIO_INCLUDE_DIRS}")


# Commonly used C++ standard settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Increase Warnings, treat warnings as errors
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Enable AddressSanitizer for X86_64 Debug builds on clang or gcc only
# Sanitizers don't seem to work on ARM/Raspberry Pi
if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
        set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address,undefined")
    endif()
endif()

# Add subdirectories
add_subdirectory(libraries/bluetooth_conn)
add_subdirectory(node1)
add_subdirectory(node2)
