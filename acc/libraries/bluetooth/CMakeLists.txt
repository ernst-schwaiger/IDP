project(bluetooth)

# Create library
add_library(bluetooth
    src/bluetooth.cpp
)

# Include headers
target_include_directories(bluetooth PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

#
# Tests
#
add_executable(bluetooth_test 
    test/bluetooth_test.cpp
    src/bluetooth.cpp
    )

# Link with libraries
target_link_libraries(bluetooth_test PRIVATE ${LibBlueZ})

# for coverage: ensure tests are executed in debug mode
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(bluetooth_test PRIVATE --coverage)
    target_link_options(bluetooth_test PRIVATE --coverage)
endif()

target_include_directories(bluetooth_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(bluetooth_test PRIVATE Catch2::Catch2WithMain)
add_test(NAME bluetooth_test COMMAND bluetooth_test)

# Custom target to run tests and generate coverage report 
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
add_custom_target(coverage 
    COMMAND ${CMAKE_CTEST_COMMAND} -T test --output-on-failure 
    COMMAND gcovr -r ${CMAKE_CURRENT_SOURCE_DIR} --xml -o coverage.xml 
    COMMAND gcovr -r ${CMAKE_CURRENT_SOURCE_DIR} --html --html-details -o coverage.html
    DEPENDS bluetooth_test )
endif()
