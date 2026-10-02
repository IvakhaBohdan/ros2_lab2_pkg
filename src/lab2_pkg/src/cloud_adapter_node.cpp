#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <cstring>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"

// 1. Власний C++ тип даних із визначеними смарт-вказівниками
struct CustomPointCloud
{
  using SharedPtr = std::shared_ptr<CustomPointCloud>;
  using ConstSharedPtr = std::shared_ptr<const CustomPointCloud>;

  std::string frame_id;
  std::vector<float> points;
};

// 2. Спеціалізація rclcpp::TypeAdapter
template<>
struct rclcpp::TypeAdapter<CustomPointCloud, sensor_msgs::msg::PointCloud2>
{
  using is_specialized = std::true_type;
  using custom_type = CustomPointCloud;
  using ros_message_type = sensor_msgs::msg::PointCloud2;

  static void convert_to_ros_message(
    const custom_type & custom,
    ros_message_type & ros_msg)
  {
    RCLCPP_INFO(rclcpp::get_logger("TypeAdapter"), "-> CONVERT: Custom -> ROS PointCloud2 (Серіалізація)");
    ros_msg.header.frame_id = custom.frame_id;
    ros_msg.height = 1;
    ros_msg.width = custom.points.size() / 3;
    ros_msg.row_step = custom.points.size() * sizeof(float);
    ros_msg.data.resize(ros_msg.row_step);
    std::memcpy(ros_msg.data.data(), custom.points.data(), ros_msg.row_step);
  }

  static void convert_to_custom(
    const ros_message_type & ros_msg,
    custom_type & custom)
  {
    RCLCPP_INFO(rclcpp::get_logger("TypeAdapter"), "-> CONVERT: ROS PointCloud2 -> Custom (Десеріалізація)");
    custom.frame_id = ros_msg.header.frame_id;
    size_t float_count = ros_msg.data.size() / sizeof(float);
    custom.points.resize(float_count);
    std::memcpy(custom.points.data(), ros_msg.data.data(), ros_msg.data.size());
  }
};

using AdaptedCloud = rclcpp::TypeAdapter<CustomPointCloud, sensor_msgs::msg::PointCloud2>;

class CloudPublisherSubscriberNode : public rclcpp::Node
{
public:
  CloudPublisherSubscriberNode(const rclcpp::NodeOptions & options)
  : Node("cloud_adapter_node", options)
  {
    publisher_ = this->create_publisher<AdaptedCloud>("adapted_cloud_topic", 10);
    
    subscription_ = this->create_subscription<AdaptedCloud>(
      "adapted_cloud_topic", 10,
      [this](const CustomPointCloud::SharedPtr msg) {
        RCLCPP_INFO(
          this->get_logger(),
          "Отримано адаптовану хмару точок [frame: '%s', точок: %zu]",
          msg->frame_id.c_str(), msg->points.size() / 3);
      });

    timer_ = this->create_wall_timer(
      std::chrono::seconds(2),
      [this]() { publish_cloud(); });

    RCLCPP_INFO(this->get_logger(), "Вузол TypeAdapter успішно запущено із Intra-Process зв'язком.");
  }

private:
  void publish_cloud()
  {
    CustomPointCloud cloud;
    cloud.frame_id = "laser_frame";
    cloud.points = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};

    RCLCPP_INFO(this->get_logger(), "Публікація кастомної хмари точок...");
    publisher_->publish(cloud);
  }

  rclcpp::Publisher<AdaptedCloud>::SharedPtr publisher_;
  rclcpp::Subscription<AdaptedCloud>::SharedPtr subscription_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  
  rclcpp::NodeOptions options;
  options.use_intra_process_comms(true);

  auto node = std::make_shared<CloudPublisherSubscriberNode>(options);
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
