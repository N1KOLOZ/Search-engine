cmake_minimum_required(VERSION 3.12)

project(SearchEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -O3")
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

#--- Include project files
file(GLOB sources ${PROJECT_SOURCE_DIR}/*.cpp)
file(GLOB headers ${PROJECT_SOURCE_DIR}/*.h)

add_executable(${PROJECT_NAME} main.cpp ${sources}  ${headers})
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
