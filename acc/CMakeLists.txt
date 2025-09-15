cmake_minimum_required(VERSION 3.22)
project(acc VERSION 0.0.1 LANGUAGES CXX)

Include(FetchContent)
Include(CTest)

#
# For getting Catch2 sources
#
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.10.0 # or a later release
)

find_library(LibBlueZ NAMES libbluetooth.a)
find_library(LibBlueZ NAMES bluetooth.lib)

if(MYLIB_LIBRARY)
    target_link_libraries(MyApp PRIVATE ${MYLIB_LIBRARY})
endif()

FetchContent_MakeAvailable(Catch2)

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
add_subdirectory(libraries/bluetooth)
add_subdirectory(node1)
add_subdirectory(node2)
