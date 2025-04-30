cmake_minimum_required(VERSION 3.28)

project(rmm-test LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 17)

find_package(Boost 1.88.0 REQUIRED COMPONENTS asio)

add_executable(rmm rmm.cpp
    PollingTimer.h
    PollingTimer.cpp
    CPUSampler.h
    CPUSampler.cpp
    MemAvailable.h
    MemAvailable.cpp
)

target_link_libraries(rmm PRIVATE Boost::asio)