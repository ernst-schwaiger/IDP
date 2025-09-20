project(bluetooth_conn)

# Create library
add_library(bluetooth_conn
    src/bluetooth.cpp
)

# Include headers
target_include_directories(bluetooth_conn PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(bluetooth_conn PRIVATE
    ${GIO_INCLUDE_DIRS}
)

target_link_libraries(bluetooth_conn PRIVATE ${BLUEZ_LIBRARIES} ${GIO_LIBRARIES} libtomcrypt)

#
# Tests
#
add_executable(bluetooth_test 
    test/bluetooth_test.cpp
    test/crypto_wrapper_test.cpp
    src/bluetooth.cpp
    )

# Link with libraries
target_link_libraries(bluetooth_test PRIVATE Catch2::Catch2WithMain ${BLUEZ_LIBRARIES} ${GIO_LIBRARIES} libtomcrypt)
target_link_directories(bluetooth_test PRIVATE ${GIO_LIBRARY_DIRS}) 

# for coverage: ensure tests are executed in debug mode
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(bluetooth_test PRIVATE --coverage)
    target_link_options(bluetooth_test PRIVATE --coverage)
endif()

target_include_directories(bluetooth_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GIO_INCLUDE_DIRS}
    )

add_test(NAME bluetooth_test COMMAND bluetooth_test)

# Custom target to run tests and generate coverage report 
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
add_custom_target(coverage 
    COMMAND ${CMAKE_CTEST_COMMAND} -T test --output-on-failure 
    COMMAND gcovr -r ${CMAKE_CURRENT_SOURCE_DIR} --xml -o coverage.xml 
    COMMAND gcovr -r ${CMAKE_CURRENT_SOURCE_DIR} --html --html-details -o coverage.html
    DEPENDS bluetooth_test )
endif()
