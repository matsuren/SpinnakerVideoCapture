######################################################################
# The project name
cmake_minimum_required (VERSION 3.1)
set(ProjectId "SpinCapture")
message(Project Id : ${ProjectId})
project (${ProjectId})
set(CMAKE_CXX_STANDARD 14)

#Setting the variables
set( PROJ_NAME      "capture_test" )
set( PROJ_INCLUDES "" )
set( PROJ_LIBRARIES "" )
message(------project name : ${PROJ_NAME} ---------)

######################################################################
# OpenCV
#set(OpenCV_DIR $ENV{OpenCV_DIR})
find_package(OpenCV REQUIRED)
# Please set "target_link_libraries( ${PROJ_NAME}  ${OpenCV_LIBS})" 

######################################################################
# Spinnaker SDK
if(WIN32) 
set(Spinnaker_LIBS "Spinnakerd_$(PlatformToolset).lib")
set(Spinnaker_INCLUDE_DIR "C:/Program Files/Point Grey Research/Spinnaker/include")
link_directories("C:/Program Files/Point Grey Research/Spinnaker/lib64/vs2015")
endif(WIN32) 
if(UNIX)
set(Spinnaker_LIBS "")
endif(UNIX)

######################################################################
# Project
add_executable(${PROJ_NAME}  main.cpp)
target_link_libraries( ${PROJ_NAME}  ${OpenCV_LIBS} ${Spinnaker_LIBS})
target_include_directories(${PROJ_NAME}
  PUBLIC ${Spinnaker_INCLUDE_DIR}
)
