project(node1)

add_executable(node1 
    src/main.cpp
    src/sensor.cpp)

# Link with libraries
target_link_libraries(node1 PRIVATE 
    CryptoComm
    pigpio
)

# Include CommonLib headers
target_include_directories(node1 PRIVATE
    ${CMAKE_SOURCE_DIR}/libraries/CryptoComm/include
)