#include <chrono>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "lab2_pkg/srv/reset_filter.hpp"

using namespace std::chrono_literals;

int main(int argc, char ** argv)
{
   rclcpp::init(argc, argv);
   auto node = rclcpp::Node::make_shared("reset_client");
   auto client = node->create_client<lab2_pkg::srv::ResetFilter>("reset_filter");

   while (!client->wait_for_service(1s)) {
     if (!rclcpp::ok()) {
        RCLCPP_ERROR(node->get_logger(), "Перервано очікування.");
        return 1;
      }
      RCLCPP_INFO(node->get_logger(), "Очікування доступності сервісу reset_filter...");
    }

    auto request = std::make_shared<lab2_pkg::srv::ResetFilter::Request>();
    auto future = client->async_send_request(request);

    if (rclcpp::spin_until_future_complete(node, future, 2s) == rclcpp::FutureReturnCode::SUCCESS) {
       auto response = future.get();
       RCLCPP_INFO(node->get_logger(), "Відповідь: Success = %s, Message = '%s'",
                   response->success ? "true" : "false", response->message.c_str());
     } else {
       RCLCPP_ERROR(node->get_logger(), "Помилка: перевищено таймаут очікування відповіді.");
     }

     rclcpp::shutdown();
     return 0;
   }
