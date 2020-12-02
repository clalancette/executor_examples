#include <chrono>
#include <cinttypes>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "example_interfaces/srv/add_two_ints.hpp"
#include "rclcpp/rclcpp.hpp"

class MultipleExecutorsMultipleExclusiveCBGroup final : public rclcpp::Node
{
public:
  MultipleExecutorsMultipleExclusiveCBGroup()
  : Node("multiple_executors_multiple_exclusve_cb_group")
  {
    // The second argument tells create_callback_group to not automatically add the callback group
    // to an executor.
    cb_grp_client_ = this->create_callback_group(rclcpp::callback_group::CallbackGroupType::MutuallyExclusive, false);
    client_ = this->create_client<example_interfaces::srv::AddTwoInts>("add_two_ints", rmw_qos_profile_services_default, cb_grp_client_);
    cb_grp_timer_ = this->create_callback_group(rclcpp::callback_group::CallbackGroupType::MutuallyExclusive);
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(500), std::bind(&MultipleExecutorsMultipleExclusiveCBGroup::timer_callback, this), cb_grp_timer_);

    service_thread_ = std::thread([this]() {
      client_executor_.add_callback_group(this->cb_grp_client_, this->get_node_base_interface());
      client_executor_.spin();
    });
  }

  ~MultipleExecutorsMultipleExclusiveCBGroup()
  {
    client_executor_.cancel();
    service_thread_.join();
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

  std::thread service_thread_;
  rclcpp::callback_group::CallbackGroup::SharedPtr cb_grp_client_;
  rclcpp::Client<example_interfaces::srv::AddTwoInts>::SharedPtr client_;
  rclcpp::executors::SingleThreadedExecutor client_executor_;
  rclcpp::callback_group::CallbackGroup::SharedPtr cb_grp_timer_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
  fprintf(stderr, "This example has a timer callback and a service client, in separate exclusive\n");
  fprintf(stderr, " callback groups.  There are two executors; one in the main thread, and one in a\n");
  fprintf(stderr, " separate thread in the class.  One callback group is added to each of them.  The\n");
  fprintf(stderr, " timer attempts to call the client and wait for the result, which succeeds.  This\n");
  fprintf(stderr, " is because there are multiple threads available to service the timer and the\n");
  fprintf(stderr, " service response, and they are in separate callback groups.\n");

  rclcpp::init(argc, argv);

  rclcpp::executors::SingleThreadedExecutor exec;
  auto service_caller = std::make_shared<MultipleExecutorsMultipleExclusiveCBGroup>();
  exec.add_node(service_caller);
  exec.spin();

  rclcpp::shutdown();
  return 0;
}
