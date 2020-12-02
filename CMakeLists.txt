cmake_minimum_required(VERSION 3.5)
project(executor_examples)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(example_interfaces REQUIRED)
find_package(rclcpp REQUIRED)

add_executable(executor_single_threaded_single_exclusive_cb_group
  src/executor_single_threaded_single_exclusive_cb_group.cpp
)
ament_target_dependencies(executor_single_threaded_single_exclusive_cb_group
  example_interfaces
  rclcpp
)

add_executable(executor_single_threaded_multiple_exclusive_cb_group
  src/executor_single_threaded_single_exclusive_cb_group.cpp
)
ament_target_dependencies(executor_single_threaded_multiple_exclusive_cb_group
  example_interfaces
  rclcpp
)

install(TARGETS
  executor_single_threaded_single_exclusive_cb_group
  executor_single_threaded_multiple_exclusive_cb_group
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
