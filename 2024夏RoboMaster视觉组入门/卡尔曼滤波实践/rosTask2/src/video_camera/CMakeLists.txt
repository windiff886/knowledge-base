cmake_minimum_required(VERSION 3.8)
project(video_camera)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

#add_definitions(-Wall -Werror)

#######################
## Find dependencies ##
#######################

find_package(ament_cmake_auto REQUIRED)
find_package(OpenCV REQUIRED)
ament_auto_find_build_dependencies()

###########
## Build ##
###########

ament_auto_add_library(${PROJECT_NAME} SHARED
  DIRECTORY src
)

target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

rclcpp_components_register_node(${PROJECT_NAME}
  PLUGIN video_camera::CameraNode
  EXECUTABLE camera_node
)

#############
## Install ##
#############

install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}/
)

ament_auto_package(
  INSTALL_TO_SHARE
)
