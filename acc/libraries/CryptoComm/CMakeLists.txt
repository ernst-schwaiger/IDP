project(CryptoComm)

# Create library
add_library(CryptoComm
    src/BTListenSocket.cpp
    src/BTConnection.cpp
    src/CryptoWrapper.cpp
)

# Include headers
target_include_directories(CryptoComm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(CryptoComm PRIVATE
    ${GIO_INCLUDE_DIRS}
)

target_link_libraries(CryptoComm PRIVATE ${BLUEZ_LIBRARIES} ${GIO_LIBRARIES} libtomcrypt)

#
# Tests
#
add_executable(CryptoCommTest 
    test/bluetooth_test.cpp
    test/CryptoWrapperTest.cpp
    src/bluetooth.cpp
    src/CryptoWrapper.cpp
    )

# Link with libraries
target_link_libraries(CryptoCommTest PRIVATE Catch2::Catch2WithMain ${BLUEZ_LIBRARIES} ${GIO_LIBRARIES} libtomcrypt)
target_link_directories(CryptoCommTest PRIVATE ${GIO_LIBRARY_DIRS}) 

# for coverage: ensure tests are executed in debug mode
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(CryptoCommTest PRIVATE --coverage)
    target_link_options(CryptoCommTest PRIVATE --coverage)
endif()

target_include_directories(CryptoCommTest PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GIO_INCLUDE_DIRS}
    )

add_test(NAME CryptoCommTest COMMAND CryptoCommTest)

# Custom target to run tests and generate coverage report 
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
add_custom_target(coverage 
    COMMAND ${CMAKE_CTEST_COMMAND} -T test --output-on-failure 
    COMMAND gcovr -r ${CMAKE_CURRENT_SOURCE_DIR} --xml -o coverage.xml 
    COMMAND gcovr -r ${CMAKE_CURRENT_SOURCE_DIR} --html --html-details -o coverage.html
    DEPENDS CryptoCommTest )
endif()
