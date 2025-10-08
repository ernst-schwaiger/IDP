project(Helper)

# Create library
add_library(Helper
    src/Helper.cpp
)

# Include headers
target_include_directories(Helper PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(Helper PRIVATE
    ${GIO_INCLUDE_DIRS}
)

target_link_libraries(Helper PRIVATE ${BLUEZ_LIBRARIES} ${GIO_LIBRARIES} libtomcrypt)

#
# Tests
#
add_executable(HelperTest 
    test/HelperTest.cpp
    src/Helper.cpp
    )

# Link with libraries
target_link_libraries(HelperTest PRIVATE Catch2::Catch2WithMain)

# for coverage: ensure tests are executed in debug mode
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(HelperTest PRIVATE --coverage)
    target_link_options(HelperTest PRIVATE --coverage)
endif()

target_include_directories(HelperTest PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GIO_INCLUDE_DIRS}
    )

add_test(NAME HelperTest COMMAND HelperTest)

# Custom target to run tests and generate coverage report 
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
add_custom_target(coverage2 
    COMMAND ${CMAKE_CTEST_COMMAND} -T test --output-on-failure 
    COMMAND gcovr -r ${CMAKE_CURRENT_SOURCE_DIR} --xml -o coverage.xml 
    COMMAND gcovr -r ${CMAKE_CURRENT_SOURCE_DIR} --html --html-details -o coverage.html
    DEPENDS HelperTest )
endif()
