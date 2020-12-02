#include <chrono>
#include <cinttypes>
#include <functional>
#include <memory>
#include <string>

#include "example_interfaces/srv/add_two_ints.hpp"
#include "rclcpp/rclcpp.hpp"

class MultiThreadedMultipleExclusiveCBGroup final : public rclcpp::Node
{
public:
  MultiThreadedMultipleExclusiveCBGroup()
  : Node("multi_threaded_multiple_exclusive_cb_group")
  {
    cb_grp_client_ = this->create_callback_group(rclcpp::callback_group::CallbackGroupType::MutuallyExclusive);
    client_ = this->create_client<example_interfaces::srv::AddTwoInts>("add_two_ints", rmw_qos_profile_services_default, cb_grp_client_);
    cb_grp_timer_ = this->create_callback_group(rclcpp::callback_group::CallbackGroupType::MutuallyExclusive);
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(500), std::bind(&MultiThreadedMultipleExclusiveCBGroup::timer_callback, this), cb_grp_timer_);
  }

private:
  void timer_callback()
  {
    while (!client_->wait_for_service(std::chrono::seconds(1))) {
      if (!rclcpp::ok()) {
        RCLCPP_ERROR(this->get_logger(), "client interrupted while waiting for service to appear.");
        return;
      }
      RCLCPP_INFO(this->get_logger(), "waiting for service to appear...");
    }

    auto request = std::make_shared<example_interfaces::srv::AddTwoInts::Request>();
    request->a = 66;
    request->b = 1;
    auto result_future = client_->async_send_request(request);
    while (rclcpp::ok()) {
      RCLCPP_INFO(this->get_logger(), "Waiting for result");
      std::future_status status = result_future.wait_for(std::chrono::seconds(1));
      if (status == std::future_status::ready) {
        break;
      }
    }
    if (!rclcpp::ok()) {
      RCLCPP_INFO(this->get_logger(), "Program canceled");
      return;
    }
    auto result = result_future.get();
    RCLCPP_INFO(this->get_logger(), "Future returned, result is %" PRId64, result->sum);
  }

  rclcpp::callback_group::CallbackGroup::SharedPtr cb_grp_client_;
  rclcpp::Client<example_interfaces::srv::AddTwoInts>::SharedPtr client_;
  rclcpp::callback_group::CallbackGroup::SharedPtr cb_grp_timer_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
  fprintf(stderr, "This example has a timer callback and a service client, in separate mutually\n");
  fprintf(stderr, " exclusive callback groups.  The executor is multi-threaded.  The timer attempts to\n");
  fprintf(stderr, " call the client and wait for the result, which succeeds.  The reasons for this is\n");
  fprintf(stderr, " that the calls are in separate callback groups (so can be serviced at the same time)\n");
  fprintf(stderr, " and that there is more than one thread available to service them.\n");

  rclcpp::init(argc, argv);
  rclcpp::executors::MultiThreadedExecutor exec;
  auto service_caller = std::make_shared<MultiThreadedMultipleExclusiveCBGroup>();
  exec.add_node(service_caller);
  exec.spin();
  rclcpp::shutdown();
  return 0;
}
