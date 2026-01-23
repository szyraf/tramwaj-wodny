cmake_minimum_required(VERSION 3.10)
project(water_tram)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(main src/main.cpp src/config.cpp src/ipc.cpp src/logger.cpp)
add_executable(captain src/captain.cpp src/config.cpp src/ipc.cpp src/logger.cpp)
add_executable(passenger src/passenger.cpp src/config.cpp src/ipc.cpp src/logger.cpp)
add_executable(dispatcher src/dispatcher.cpp src/config.cpp src/ipc.cpp src/logger.cpp)
