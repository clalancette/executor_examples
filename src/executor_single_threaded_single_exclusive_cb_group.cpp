#include <chrono>
#include <cinttypes>
#include <functional>
#include <memory>
#include <string>

#include "example_interfaces/srv/add_two_ints.hpp"
#include "rclcpp/rclcpp.hpp"

class SingleThreadedSingleExclusiveCBGroup final : public rclcpp::Node
{
public:
  SingleThreadedSingleExclusiveCBGroup()
  : Node("single_threaded_single_exclusive_cb_group")
  {
    cb_grp_ = this->create_callback_group(rclcpp::callback_group::CallbackGroupType::MutuallyExclusive);
    client_ = this->create_client<example_interfaces::srv::AddTwoInts>("add_two_ints", rmw_qos_profile_services_default, cb_grp_);
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(500), std::bind(&SingleThreadedSingleExclusiveCBGroup::timer_callback, this), cb_grp_);
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

  rclcpp::callback_group::CallbackGroup::SharedPtr cb_grp_;
  rclcpp::Client<example_interfaces::srv::AddTwoInts>::SharedPtr client_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
  fprintf(stderr, "This example has a timer callback and a service client, both in the same mutually\n");
  fprintf(stderr, " exclusive callback group.  The executor is single-threaded.  The timer attempts to\n");
  fprintf(stderr, " call the client and wait for the result, which leads to the client waiting forever.\n");
  fprintf(stderr, " There are two reasons for this; the callback group used is exclusive (so only one of\n");
  fprintf(stderr, " the callbacks within the group can be active at a time), and there is no executor\n");
  fprintf(stderr, " thread available to service the service response.\n");

  rclcpp::init(argc, argv);
  rclcpp::executors::SingleThreadedExecutor exec;
  auto service_caller = std::make_shared<SingleThreadedSingleExclusiveCBGroup>();
  exec.add_node(service_caller);
  exec.spin();
  rclcpp::shutdown();
  return 0;
}
