cmake_minimum_required(VERSION 4.1)
project(httpfromtcp_cpp)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_TESTING "Enable a Unit Testing Build" ON)

include(FetchContent)

if(ENABLE_TESTING)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2
        GIT_TAG v3.12.0
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(Catch2)
endif ()

set(REQUEST_LIBRARY request_lib)
set(SERVER_LIBRARY server_lib)

add_subdirectory(cmd)
add_subdirectory(internal)
if(ENABLE_TESTING)
    include(CTest)
    add_subdirectory(tests)
endif ()
