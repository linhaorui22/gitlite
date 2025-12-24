cmake_minimum_required(VERSION 3.10)

project(gitlite LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/main.cpp
)

add_executable(gitlite ${SRC_FILES})

if (WIN32)
    target_link_libraries(gitlite PRIVATE stdc++fs)
endif()
