project(node1)

add_executable(node1 
    src/main.cpp
    src/Sensor.cpp)

# Link with libraries
target_link_libraries(node1 PRIVATE 
    CryptoComm
    pigpio
)

# Include CommonLib headers
target_include_directories(node1 PRIVATE
    ${pigpio_SOURCE_DIR}
)

target_link_directories(node1 PRIVATE ${pigpio_BINARY_DIR}) 