# ##############################################################################
# The project name
cmake_minimum_required(VERSION 3.1)
set(ProjectId "SpinCapture")
message(Project Id : ${ProjectId})
project(${ProjectId})
set(CMAKE_CXX_STANDARD 14)

# Setting the variables
set(PROJ_NAME "capture_test")
set(PROJ_INCLUDES "")
set(PROJ_LIBRARIES "")
message(------project name : ${PROJ_NAME} ---------)

# ##############################################################################
# OpenCV set(OpenCV_DIR $ENV{OpenCV_DIR})
find_package(OpenCV REQUIRED)
# Please set "target_link_libraries( ${PROJ_NAME}  ${OpenCV_LIBS})"

# ##############################################################################
# Spinnaker SDK
if(WIN32)
  set(Spinnaker_LIBS debug "Spinnakerd_v140.lib" optimized "Spinnaker_v140.lib")
  set(Spinnaker_INCLUDE_DIR
      "C:/Program Files/Point Grey Research/Spinnaker/include")
  link_directories(
    "C:/Program Files/Point Grey Research/Spinnaker/lib64/vs2015")
endif(WIN32)
if(UNIX)
  add_definitions(-DLINUX)
  find_path(Spinnaker_INCLUDE_DIR Spinnaker.h 
    PATHS /opt/spinnaker /usr /usr/local
    PATH_SUFFIXES include)
  find_library(Spinnaker_LIBS NAMES Spinnaker 
    PATHS /opt/spinnaker /usr /usr/local
    PATH_SUFFIXES lib
  )
endif(UNIX)

# ##############################################################################
# Project
add_executable(${PROJ_NAME} main.cpp SpinCap/spincamera.cpp)
target_link_libraries(${PROJ_NAME} ${OpenCV_LIBS} ${Spinnaker_LIBS})
target_include_directories(${PROJ_NAME} PUBLIC ${PROJECT_SOURCE_DIR} ${Spinnaker_INCLUDE_DIR})
