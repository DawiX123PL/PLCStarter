# CMakeList.txt : CMake project for PLCStarter, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("PLCStarter")


# Add source to this project's executable.
add_executable (PLCStarter "src/Starter.cpp")
add_executable (TestApp "src/TestApp.cpp")


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
