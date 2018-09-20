cmake_minimum_required( VERSION 3.2 )
set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_MODULE_PATH})

project(RoboticArmKinect)
set(PROJECT_NAME RoboticArmKinect)
set(SRC ${PROJECT_SOURCE_DIR}/SkeletonBasics.cpp
        ${PROJECT_SOURCE_DIR}/SkeletonBasics.h
		
		${PROJECT_SOURCE_DIR}/ImageRenderer.cpp
		${PROJECT_SOURCE_DIR}/ImageRenderer.h)
		
add_executable(${PROJECT_NAME}  ${SRC})

# Find Kinect SDK v1
find_package(OpenCV REQUIRED)
find_package(KinectSDK REQUIRED)

# Settings Kinect SDK v1
if(KinectSDK_FOUND)
  include_directories(${KinectSDK_INCLUDE_DIRS} )
  link_directories(${KinectSDK_LIBRARY_DIRS} )
  target_link_libraries(${PROJECT_NAME} ${KinectSDK_LIBRARIES} )
endif()

if(OpenCV_FOUND)
  include_directories(${OpenCV_INCLUDE_DIRS})
  target_link_libraries(${EXECUTABLE_NAME} ${OpenCV_LIBS})
endif()

# Setrtings Kinect Toolkit v1
if(KinectToolkit_FOUND)
  include_directories( ${KinectToolkit_INCLUDE_DIRS} )
  link_directories( ${KinectToolkit_LIBRARY_DIRS} )
  target_link_libraries( project ${KinectToolkit_LIBRARIES} )
  add_custom_command( TARGET project POST_BUILD ${KinectToolkit_COMMANDS} )
endif()
