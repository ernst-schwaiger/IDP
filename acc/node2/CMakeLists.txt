project(node2)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(node2 
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/mainwindow.ui
    src/ACCThread.cpp  
    src/CommThread.cpp
    )

# Link with libraries
target_link_libraries(node2 PRIVATE CryptoComm Helper Qt6::Widgets)

# Include CommonLib headers
target_include_directories(node2 PRIVATE
    ${CMAKE_SOURCE_DIR}/libraries/CryptoComm/include
)