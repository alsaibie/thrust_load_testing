cmake_minimum_required(VERSION 3.8)
project(thruster_load_test)
include_directories( include/
        libserial/src/)
ADD_LIBRARY(LibSerial
        libserial/src/PosixSignalDispatcher.cpp
        libserial/src/SerialPort.cpp
        libserial/src/SerialStream.cc
        libserial/src/SerialStreamBuf.cc
        )
set(CMAKE_CXX_STANDARD 11)

link_directories(libserial/src/.libs/)
set(SOURCE_FILES
        src/main.cpp
        src/arduino_interface.cpp
        src/usbscale.cpp
        include/arduino_interface.h
        include/usbscale.h)
add_executable(thruster_load_test ${SOURCE_FILES})
add_executable(lusb src/lsusb.c include/scales.h)
target_link_libraries(thruster_load_test LibSerial m usb-1.0)