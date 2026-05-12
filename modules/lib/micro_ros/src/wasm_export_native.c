#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "wasm_export.h"
#include "wasm_export_native.h"

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <rmw_microros/rmw_microros.h>
#include <std_srvs/srv/set_bool.h>
#include <microros_transports.h>

#define LED0_NODE DT_ALIAS(led0)

rcl_allocator_t allocator;
rclc_support_t  support;
rcl_node_t      node;
rcl_service_t   service;
rclc_executor_t executor;
rcl_client_t client;

LOG_MODULE_REGISTER(wasm_native, LOG_LEVEL_INF);


static std_srvs__srv__SetBool_Request  request_msg;
static std_srvs__srv__SetBool_Response response_msg;
static const struct gpio_dt_spec my_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(pb8_out), gpios);

extern bool zephyr_transport_open(struct uxrCustomTransport *transport);
extern bool zephyr_transport_close(struct uxrCustomTransport *transport);
extern size_t zephyr_transport_write(struct uxrCustomTransport *transport,
                                     const uint8_t *buf, size_t len,
                                     uint8_t *err);
extern size_t zephyr_transport_read(struct uxrCustomTransport *transport,
                                    uint8_t *buf, size_t len, int timeout,
                                    uint8_t *err);

void service_callback(const void *req, void *res)
{
    std_srvs__srv__SetBool_Request  *request  = (std_srvs__srv__SetBool_Request *)  req;
    std_srvs__srv__SetBool_Response *response = (std_srvs__srv__SetBool_Response *) res;

    ARG_UNUSED(request);

    gpio_pin_toggle_dt(&my_pin);
    response->success = true;
}

static void native_micro_ros_service_init(wasm_exec_env_t exec_env)
{
    ARG_UNUSED(exec_env);

 
    gpio_pin_configure_dt(&my_pin, GPIO_OUTPUT_ACTIVE);

    rmw_uros_set_custom_transport(
        MICRO_ROS_FRAMING_REQUIRED,
        (void *) &default_params,
        zephyr_transport_open,
        zephyr_transport_close,
        zephyr_transport_write,
        zephyr_transport_read
    );

    allocator = rcl_get_default_allocator();

    printk("Pinging agent...\n");
    while (rmw_uros_ping_agent(500, 1) != RMW_RET_OK) {
        printk("retry...\n");
    }
    printk("Agent found!\n");

    if (rclc_support_init(&support, 0, NULL, &allocator) != RCL_RET_OK) {
        return;
    }

    if (rclc_node_init_default(&node, "service", "", &support) != RCL_RET_OK) {
        return;
    }

    if (rclc_service_init_default(
            &service, &node,
            ROSIDL_GET_SRV_TYPE_SUPPORT(std_srvs, srv, SetBool),
            "/green_led") != RCL_RET_OK) {
        return;
    }

    if (rclc_executor_init(&executor, &support.context, 1, &allocator) != RCL_RET_OK) {
        return;
    }

    if (rclc_executor_add_service(
            &executor, &service,
            &request_msg, &response_msg,
            service_callback) != RCL_RET_OK) {
        return;
    }

    //LOG_INF("micro-ROS service ready on /green_led");

    while (true) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(1000));
    }
}




static void client_callback(const void *response)
{
    std_srvs__srv__SetBool_Response *res =
        (std_srvs__srv__SetBool_Response *)response;
    printk("Response: success=%d\n", res->success);
}

static void native_micro_ros_app_init(wasm_exec_env_t exec_env)
{
    ARG_UNUSED(exec_env);

    rmw_uros_set_custom_transport(
        MICRO_ROS_FRAMING_REQUIRED,
        (void *) &default_params,
        zephyr_transport_open,
        zephyr_transport_close,
        zephyr_transport_write,
        zephyr_transport_read
    );

    allocator = rcl_get_default_allocator();

    while (rmw_uros_ping_agent(500, 1) != RMW_RET_OK) {}

    rclc_support_init(&support, 0, NULL, &allocator);
    rclc_node_init_default(&node, "zephyr_app_node", "", &support);

    rclc_client_init_default(&client, &node,
        ROSIDL_GET_SRV_TYPE_SUPPORT(std_srvs, srv, SetBool),
        "/green_led");

    rclc_executor_init(&executor, &support.context, 1, &allocator);
    rclc_executor_add_client(&executor, &client, &response_msg, client_callback);

}

static void native_green_led_toggle(wasm_exec_env_t exec_env)
{
    ARG_UNUSED(exec_env);

    std_srvs__srv__SetBool_Request req;
    std_srvs__srv__SetBool_Request__init(&req);
    req.data = true;

    int64_t sequence_number;
    rcl_send_request(&client, &req, &sequence_number);

    // Spin per ricevere la response
    rclc_executor_spin_some(&executor, RCL_MS_TO_NS(1000));
}
static void native_sleep_ms(wasm_exec_env_t exec_env, int32_t ms)
{
    (void)exec_env;
    if (ms > 0) {
        k_msleep(ms);
    }
}


static NativeSymbol native_symbols[] = {
    { "micro_ros_service_init", native_micro_ros_service_init, "()", NULL },
    { "micro_ros_app_init", native_micro_ros_app_init, "()", NULL },
    { "green_led_toggle", native_green_led_toggle, "()", NULL },
    { "sleep_ms",   native_sleep_ms,   "(i)",  NULL }
};

bool register_native_functions(void)
{
    int n = sizeof(native_symbols) / sizeof(native_symbols[0]);
    if (!wasm_runtime_register_natives("env", native_symbols, n)) {
        return false;
    }
    //LOG_INF("Registered %d native symbols", n);
    return true;
}