# CMakeList.txt : CMake project for PLCStarter, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("PLCStarter")


# Add source to this project's executable.

set(PLCStarter_files 
            "src/Starter.cpp"         
            "src/Starter.cpp"
            "src/thread.h"
            "src/Server.h"
            "src/PLCServer.h"
            "src/StarterConfig.cpp"
            "src/StarterConfig.h")

add_executable (PLCStarter ${PLCStarter_files})

add_executable (TestApp "src/TestApp.cpp")

# include boost library
target_include_directories(PLCStarter PRIVATE "./boost_1_79_0")

set(boost_lib_dir "${CMAKE_SOURCE_DIR}/boost_1_79_0/stage/lib/")
target_link_libraries(PLCStarter PRIVATE "${boost_lib_dir}/libboost_container-vc143-mt-gd-x64-1_79.lib")
target_link_libraries(PLCStarter PRIVATE "${boost_lib_dir}/libboost_json-vc143-mt-gd-x64-1_79.lib")



# change default out dir
set_target_properties( PLCStarter TestApp
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build"
)

# set language standard to c++11
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET PLCStarter PROPERTY CXX_STANDARD 11)
  set_property(TARGET TestApp PROPERTY CXX_STANDARD 11)
endif()

# TODO: Add tests and install targets if needed.
