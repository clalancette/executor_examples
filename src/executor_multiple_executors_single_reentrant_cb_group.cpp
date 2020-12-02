#include <chrono>
#include <cinttypes>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "example_interfaces/srv/add_two_ints.hpp"
#include "rclcpp/rclcpp.hpp"

class MultipleExecutorsSingleReentrantCBGroup final : public rclcpp::Node
{
public:
  MultipleExecutorsSingleReentrantCBGroup()
  : Node("multiple_executors_single_reentrant_cb_group")
  {
    // The second argument tells create_callback_group to not automatically add the callback group
    // to an executor.
    cb_grp_ = this->create_callback_group(rclcpp::callback_group::CallbackGroupType::Reentrant, false);
    client_ = this->create_client<example_interfaces::srv::AddTwoInts>("add_two_ints", rmw_qos_profile_services_default, cb_grp_);
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(500), std::bind(&MultipleExecutorsSingleReentrantCBGroup::timer_callback, this), cb_grp_);

    service_thread_ = std::thread([this]() {
      client_executor_.add_callback_group(this->cb_grp_, this->get_node_base_interface());
      client_executor_.spin();
    });
  }

  ~MultipleExecutorsSingleReentrantCBGroup()
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
  rclcpp::callback_group::CallbackGroup::SharedPtr cb_grp_;
  rclcpp::Client<example_interfaces::srv::AddTwoInts>::SharedPtr client_;
  rclcpp::executors::SingleThreadedExecutor client_executor_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
  fprintf(stderr, "This example has a timer callback and a service client, both in the same\n");
  fprintf(stderr, " reentrant callback group.  There are two executors; one in the main thread,\n");
  fprintf(stderr, " and one in a separate thread in the class.  Since the callback group is set\n");
  fprintf(stderr, " to *not* automatically add to an executor with the node, it only is used when\n");
  fprintf(stderr, " it is explicitly added to an executor (and that can only be done once). The\n");
  fprintf(stderr, " timer attempts to call the client and wait for the result, which leads to the\n");
  fprintf(stderr, " client waiting forever.  There are two reasons for this; the callback group\n");
  fprintf(stderr, " is exclusive (so only one of the callbacks within the group can be active\n");
  fprintf(stderr, " at a time), and there is only one thread (the thread inside the class) that.\n");
  fprintf(stderr, " is available to service both the timer callback and the service response.\n");

  rclcpp::init(argc, argv);

  rclcpp::executors::SingleThreadedExecutor exec;
  auto service_caller = std::make_shared<MultipleExecutorsSingleReentrantCBGroup>();
  exec.add_node(service_caller);
  exec.spin();

  rclcpp::shutdown();
  return 0;
}
