#include <chrono>
#include <cstddef>
#include <cctype>
#include <memory>
#include <string>
#include <algorithm>
#include <thread>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "std_msgs/msg/string.hpp"

#include "lab2_pkg/srv/reset_filter.hpp"
#include "lab2_pkg/action/process_data.hpp"

class DataFilterNode : public rclcpp::Node
{
public:
  using ProcessData = lab2_pkg::action::ProcessData;
  using GoalHandleProcessData = rclcpp_action::ServerGoalHandle<ProcessData>;

  DataFilterNode() : Node("data_filter")
  {
    // 1. QoS Transient Local + QosOverridingOptions
    auto qos = rclcpp::QoS(1).transient_local();
    rclcpp::PublisherOptions pub_options;
    pub_options.qos_overriding_options = rclcpp::QosOverridingOptions{
      rclcpp::QosPolicyKind::Depth,
      rclcpp::QosPolicyKind::Reliability,
      rclcpp::QosPolicyKind::Durability
    };

    pub_ = this->create_publisher<std_msgs::msg::String>("uppercase_topic", qos, pub_options);

    // 2. Lazy Publisher
    lazy_timer_ = this->create_wall_timer(
      std::chrono::milliseconds(500),
      [this]() { check_subscribers(); });

    // 3. Сервіс ResetFilter
    reset_service_ = this->create_service<lab2_pkg::srv::ResetFilter>(
      "reset_filter",
      [this](const std::shared_ptr<lab2_pkg::srv::ResetFilter::Request> req,
             std::shared_ptr<lab2_pkg::srv::ResetFilter::Response> res) {
        handle_reset(req, res);
      });

    // 4. Action Server ProcessData
    action_server_ = rclcpp_action::create_server<ProcessData>(
      this,
      "process_data",
      [this](const rclcpp_action::GoalUUID & uuid, std::shared_ptr<const ProcessData::Goal> goal) {
        return handle_goal(uuid, goal);
      },
      [this](const std::shared_ptr<GoalHandleProcessData> goal_handle) {
        return handle_cancel(goal_handle);
      },
      [this](const std::shared_ptr<GoalHandleProcessData> goal_handle) {
        handle_accepted(goal_handle);
      });

    RCLCPP_INFO(this->get_logger(), "Вузол data_filter успішно запущено.");
  }

private:
  void check_subscribers()
  {
    size_t sub_count = pub_->get_subscription_count();
    if (sub_count > 0 && !sub_) {
      RCLCPP_INFO(this->get_logger(), "З'явився слухач на uppercase_topic. Вмикаємо підписку на input_topic.");
      sub_ = this->create_subscription<std_msgs::msg::String>(
        "input_topic", 10,
        [this](const std_msgs::msg::String::SharedPtr msg) {
          input_callback(msg);
        });
    } else if (sub_count == 0 && sub_) {
      RCLCPP_INFO(this->get_logger(), "Слухачі відсутні. Вимикаємо підписку на input_topic.");
      sub_.reset();
    }
  }

  void input_callback(const std_msgs::msg::String::SharedPtr msg)
  {
    auto out_msg = std_msgs::msg::String();
    out_msg.data = msg->data;
    std::transform(out_msg.data.begin(), out_msg.data.end(), out_msg.data.begin(), ::toupper);
    pub_->publish(out_msg);
    RCLCPP_INFO(this->get_logger(), "Отримано: '%s' | Опубліковано: '%s'", msg->data.c_str(), out_msg.data.c_str());
  }

  void handle_reset(
    const std::shared_ptr<lab2_pkg::srv::ResetFilter::Request>,
    std::shared_ptr<lab2_pkg::srv::ResetFilter::Response> response)
  {
    RCLCPP_INFO(this->get_logger(), "Отримано запит скидання фільтра.");
    response->success = true;
    response->message = "Фільтр успішно скинуто!";
  }

  rclcpp_action::GoalResponse handle_goal(
    const rclcpp_action::GoalUUID &,
    std::shared_ptr<const ProcessData::Goal> goal)
  {
    RCLCPP_INFO(this->get_logger(), "Отримано Action ціль із %zu елементами.", goal->numbers.size());
    return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  rclcpp_action::CancelResponse handle_cancel(const std::shared_ptr<GoalHandleProcessData>)
  {
    RCLCPP_INFO(this->get_logger(), "Запит скасування Action цілі прийнято.");
    return rclcpp_action::CancelResponse::ACCEPT;
  }

  void handle_accepted(const std::shared_ptr<GoalHandleProcessData> goal_handle)
  {
    std::thread{[this, goal_handle]() { execute_action(goal_handle); }}.detach();
  }

  void execute_action(const std::shared_ptr<GoalHandleProcessData> goal_handle)
  {
    const auto goal = goal_handle->get_goal();
    auto feedback = std::make_shared<ProcessData::Feedback>();
    auto result = std::make_shared<ProcessData::Result>();

    size_t total = goal->numbers.size();
    for (size_t i = 0; i < total; ++i) {
      if (goal_handle->is_canceling()) {
        result->success = false;
        result->total_processed = static_cast<int32_t>(i);
        goal_handle->canceled(result);
        RCLCPP_INFO(this->get_logger(), "Action скасовано.");
        return;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(400));
      feedback->progress = (static_cast<float>(i + 1) / total) * 100.0f;
      goal_handle->publish_feedback(feedback);
      RCLCPP_INFO(this->get_logger(), "Прогрес Action: %.1f%%", feedback->progress);
    }

    result->success = true;
    result->total_processed = static_cast<int32_t>(total);
    goal_handle->succeed(result);
    RCLCPP_INFO(this->get_logger(), "Action завершено зі станом SUCCEEDED.");
  }

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_;
  rclcpp::TimerBase::SharedPtr lazy_timer_;
  rclcpp::Service<lab2_pkg::srv::ResetFilter>::SharedPtr reset_service_;
  rclcpp_action::Server<ProcessData>::SharedPtr action_server_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<DataFilterNode>());
  rclcpp::shutdown();
  return 0;
}
