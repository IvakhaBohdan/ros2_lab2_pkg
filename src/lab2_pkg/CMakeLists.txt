cmake_minimum_required(VERSION 3.8)
project(lab2_pkg)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(std_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(sensor_msgs REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
   "srv/ResetFilter.srv"
   "action/ProcessData.action"
   DEPENDENCIES std_msgs
)

add_executable(data_filter_node src/data_filter_node.cpp)
ament_target_dependencies(data_filter_node rclcpp rclcpp_action std_msgs)
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(data_filter_node "${cpp_typesupport_target}")

add_executable(reset_client_node src/reset_client_node.cpp)
ament_target_dependencies(reset_client_node rclcpp std_msgs)
target_link_libraries(reset_client_node "${cpp_typesupport_target}")

add_executable(cloud_adapter_node src/cloud_adapter_node.cpp)
ament_target_dependencies(cloud_adapter_node
  rclcpp
  sensor_msgs
)

install(TARGETS
  data_filter_node
  reset_client_node
  cloud_adapter_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
