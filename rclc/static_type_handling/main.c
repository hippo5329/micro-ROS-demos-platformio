#include <unistd.h>
#include <stdio.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <micro_ros_utilities/type_utilities.h>
#include <micro_ros_utilities/string_utilities.h>

#include <sensor_msgs/msg/image.h>

rcl_publisher_t publisher;
sensor_msgs__msg__Image msg;
sensor_msgs__msg__Image msg_static;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
rcl_timer_t timer;

uint8_t my_buffer[1000];

#define LED_PIN 13

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}


void error_loop(){
  while(1){
  }
}

void timer_callback(rcl_timer_t * timer, int64_t last_call_time)
{
  RCLC_UNUSED(last_call_time);
  if (timer != NULL) {
    RCSOFTCHECK(rcl_publish(&publisher, &msg, NULL));
    RCSOFTCHECK(rcl_publish(&publisher, &msg_static, NULL));
  }
}

int rmain(int argc, const char * const * argv)
{
  allocator = rcl_get_default_allocator();

  //create init_options
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // create node
  RCCHECK(rclc_node_init_default(&node, "micro_ros_node", "", &support));

  // create publisher
  RCCHECK(rclc_publisher_init_default(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Image),
    "micro_ros_publisher"));

  // create timer,
  const unsigned int timer_timeout = 1000;
  RCCHECK(rclc_timer_init_default(
    &timer,
    &support,
    RCL_MS_TO_NS(timer_timeout),
    timer_callback));

  // create executor
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));

  // INIT MESSAGE MEMORY

  // --- Configuration ---

  // micro-ROS utilities allows to configure the dynamic memory initialization using a micro_ros_utilities_memory_conf_t structure
  // If some member of this struct is set to zero, the library will use the default value.
  // Check `micro_ros_utilities_memory_conf_default` in `<micro_ros_utilities/type_utilities.h>` for those defaults.

  static micro_ros_utilities_memory_conf_t conf = {0};

  // OPTIONALLY this struct can configure the default size of strings, basic sequences and composed sequences

  conf.max_string_capacity = 50;
  conf.max_ros2_type_sequence_capacity = 5;
  conf.max_basic_type_sequence_capacity = 5;

  // OPTIONALLY this struct can store rules for specific members
  // !! Using the API with rules will use dynamic memory allocations for handling strings !!

  micro_ros_utilities_memory_rule_t rules[] = {
    {"header.frame_id", 30},
    {"encoding", 3},
    {"data", 400}
  };
  conf.rules = rules;
  conf.n_rules = sizeof(rules) / sizeof(rules[0]);

  // --- API ---
  // Type can be instrospected using:

  rosidl_runtime_c__String data = micro_ros_utilities_type_info(
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Image));
  printf("%s", data.data);

  // It can be calculated the size that will be needed by a msg with a certain configuration

  size_t dynamic_size = micro_ros_utilities_get_dynamic_size(
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Image),
    conf
  );

  // The total (stack, static & dynamic) memory usage of a packet will be:

  size_t message_total_size = dynamic_size + sizeof(sensor_msgs__msg__Image);

  // The message dynamic memory can be allocated using the following call.
  // This will use rcutils default allocators for getting memory.

  bool success = micro_ros_utilities_create_message_memory(
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Image),
    &msg,
    conf
  );

  // The message dynamic memory can also be allocated using a buffer.
  // This will NOT use dynamic memory for the allocation.
  // If no rules set in the conf, no dynamic allocation is guaranteed.
  // This method will use need more memory than dynamic due to alignment

  size_t static_size = micro_ros_utilities_get_static_size(
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Image),
    conf
  );

  message_total_size = static_size + sizeof(sensor_msgs__msg__Image);

  // my_buffer should have at least static_size Bytes
  success &= micro_ros_utilities_create_static_message_memory(
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Image),
    &msg_static,
    conf,
    my_buffer,
    sizeof(my_buffer)
  );

  // Dynamically allocated messages can be destroyed using:

  // success &= micro_ros_utilities_destroy_message_memory(
  //   ROSIDL_GET_MSG_TYPE_SUPPORT(control_msgs, msg, JointJog),
  //   &msg,
  //   conf
  // );

  // Fill the message
  msg.header.frame_id = micro_ros_string_utilities_set(msg.header.frame_id, "myframe");
  msg_static.header.frame_id = micro_ros_string_utilities_set(msg_static.header.frame_id, "myframestatic");

  msg.height = msg_static.height = 10;
  msg.width = msg_static.width = 40;

  msg.encoding = micro_ros_string_utilities_set(msg.encoding, "RGB");
  msg_static.encoding = micro_ros_string_utilities_set(msg_static.encoding, "RGB");

  msg.data.size = msg_static.data.size = 400;
  for (size_t i = 0; i < msg.data.size; i++)
  {
    msg.data.data[i] = i;
    msg_static.data.data[i] = i;
  }

  while(1) {
    usleep(100000);
    RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100)));
  }

}
