project(node2)

add_executable(node2 src/main.cpp)

# Link with libraries
target_link_libraries(node2 PRIVATE bluetooth)

# Include CommonLib headers
target_include_directories(node2 PRIVATE
    ${CMAKE_SOURCE_DIR}/libraries/include
)