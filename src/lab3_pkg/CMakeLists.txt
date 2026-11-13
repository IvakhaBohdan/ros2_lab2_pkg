cmake_minimum_required(VERSION 3.8)
project(lab3_pkg)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(behaviortree_cpp REQUIRED)
find_package(smacc2 REQUIRED)
find_package(std_msgs REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread)

add_executable(hotel_bt_node src/hotel_bt_node.cpp)
ament_target_dependencies(hotel_bt_node rclcpp behaviortree_cpp)

add_executable(hotel_smacc2_node src/hotel_smacc2_node.cpp)
ament_target_dependencies(hotel_smacc2_node rclcpp smacc2)
target_link_libraries(hotel_smacc2_node Boost::thread)

add_executable(joint_p_controller_sim src/joint_p_controller_sim.cpp)
ament_target_dependencies(joint_p_controller_sim rclcpp)

install(TARGETS
  hotel_bt_node
  hotel_smacc2_node
  joint_p_controller_sim
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  config
  launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
