import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    pkg_share = get_package_share_directory("lab3_pkg")
    controllers_yaml = os.path.join(pkg_share, "config", "controllers.yaml")

    robot_description_xml = """<?xml version="1.0"?>
<robot name="test_joint_robot">
  <link name="base_link"/>
  <link name="link1"/>
  <joint name="joint1" type="revolute">
    <parent link="base_link"/>
    <child link="link1"/>
    <limit lower="-3.14" upper="3.14" effort="10.0" velocity="2.0"/>
  </joint>
  <ros2_control name="TestJointSystem" type="system">
    <hardware>
      <plugin>mock_components/GenericSystem</plugin>
    </hardware>
    <joint name="joint1">
      <command_interface name="position"/>
      <state_interface name="position"/>
      <state_interface name="velocity"/>
    </joint>
  </ros2_control>
</robot>
"""

    control_node = Node(
        package="controller_manager",
        executable="ros2_control_node",
        parameters=[
            {"robot_description": robot_description_xml},
            controllers_yaml,
        ],
        output="screen",
    )

    jsb_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["joint_state_broadcaster", "--controller-manager", "/controller_manager"],
        output="screen",
    )

    primary_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["primary_pos_controller", "--controller-manager", "/controller_manager"],
        output="screen",
    )

    backup_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["backup_pos_controller", "--inactive", "--controller-manager", "/controller_manager"],
        output="screen",
    )

    return LaunchDescription([
        control_node,
        jsb_spawner,
        primary_spawner,
        backup_spawner,
    ])
