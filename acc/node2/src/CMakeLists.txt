cmake_minimum_required(VERSION 3.16)
project(acc_gui_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Wenn Qt6 nicht verfügbar ist, ersetze Qt6 durch Qt5 und installiere qtbase5-dev
find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(acc_gui_demo
  src/main.cpp
  src/MainWindow.cpp
  src/MainWindow.h
  src/mainwindow.ui
)

target_link_libraries(acc_gui_demo PRIVATE Qt6::Widgets)
