#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <rmw_microros/rmw_microros.h>

#include <std_msgs/msg/int32.h>

LOG_MODULE_REGISTER(microros_pub, LOG_LEVEL_INF);

/* ── Macro di controllo errori ─────────────────────────────────────────── */
#define RCCHECK(fn)                                                        \
    do {                                                                   \
        rcl_ret_t _rc = (fn);                                              \
        if (_rc != RCL_RET_OK) {                                           \
            LOG_ERR("micro-ROS error %d at %s:%d", (int)_rc,              \
                    __FILE__, __LINE__);                                    \
            return;                                                        \
        }                                                                  \
    } while (0)

#define RCSOFTCHECK(fn)                                                    \
    do {                                                                   \
        rcl_ret_t _rc = (fn);                                              \
        if (_rc != RCL_RET_OK) {                                           \
            LOG_WRN("micro-ROS soft error %d at %s:%d", (int)_rc,        \
                    __FILE__, __LINE__);                                    \
        }                                                                  \
    } while (0)

/* ── Trasporto seriale Zephyr ──────────────────────────────────────────── */
/*
 * Il modulo micro_ros_zephyr_module fornisce queste funzioni di trasporto.
 * Vengono dichiarate extern e poi passate a rmw_uros_set_custom_transport.
 */
extern bool zephyr_transport_open(struct uxrCustomTransport *transport);
extern bool zephyr_transport_close(struct uxrCustomTransport *transport);
extern size_t zephyr_transport_write(struct uxrCustomTransport *transport,
                                     const uint8_t *buf, size_t len,
                                     uint8_t *err);
extern size_t zephyr_transport_read(struct uxrCustomTransport *transport,
                                    uint8_t *buf, size_t len, int timeout,
                                    uint8_t *err);

/* ── Parametri del trasporto ───────────────────────────────────────────── */
struct zephyr_transport_params {
    const struct device *uart_dev;
};

/* ── Timer callback: pubblica il messaggio ─────────────────────────────── */
static rcl_publisher_t publisher;
static std_msgs__msg__Int32 msg;

static void timer_callback(rcl_timer_t *timer, int64_t last_call_time)
{
    ARG_UNUSED(last_call_time);

    if (timer == NULL) {
        return;
    }

    RCSOFTCHECK(rcl_publish(&publisher, &msg, NULL));
    LOG_INF("Published: %d", msg.data);
    msg.data++;
}

/* ── Entry point ───────────────────────────────────────────────────────── */
void main(void)
{
    LOG_INF("micro-ROS publisher starting on nucleo_f446re");

    /* Recupera il device UART dal device-tree (alias microros-uart) */
    const struct device *uart_dev =
        DEVICE_DT_GET(DT_CHOSEN(microros_uart));

    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not ready!");
        return;
    }

    /* Configura il trasporto custom seriale */
    struct zephyr_transport_params transport_params = {
        .uart_dev = uart_dev,
    };

    rmw_uros_set_custom_transport(
        true,                        /* framing abilitato */
        (void *)&transport_params,
        zephyr_transport_open,
        zephyr_transport_close,
        zephyr_transport_write,
        zephyr_transport_read);

    /* ── Inizializzazione micro-ROS ──────────────────────────────────── */
    rcl_allocator_t allocator = rcl_get_default_allocator();
    rclc_support_t  support;

    /* Attendi che l'Agent sia disponibile (retry ogni 500 ms) */
    LOG_INF("Waiting for micro-ROS agent...");
    while (rmw_uros_ping_agent(500, 10) != RMW_RET_OK) {
        LOG_INF("Agent not found, retrying...");
    }
    LOG_INF("Agent found!");

    RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

    /* Nodo */
    rcl_node_t node;
    RCCHECK(rclc_node_init_default(&node, "zephyr_int32_publisher",
                                   "", &support));

    /* Publisher su /zephyr_int32_publisher */
    RCCHECK(rclc_publisher_init_default(
        &publisher, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "zephyr_int32_publisher"));

    /* Timer: callback ogni 1000 ms */
    rcl_timer_t timer;
    RCCHECK(rclc_timer_init_default(&timer, &support,
                                    RCL_MS_TO_NS(1000),
                                    timer_callback));

    /* Executor con 1 handle (il timer) */
    rclc_executor_t executor;
    RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
    RCCHECK(rclc_executor_add_timer(&executor, &timer));

    /* Inizializza il messaggio */
    msg.data = 0;

    LOG_INF("Spinning...");

    /* Loop principale */
    while (true) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
        k_msleep(10);
    }

    /* Cleanup (non raggiunto normalmente) */
    RCSOFTCHECK(rcl_publisher_fini(&publisher, &node));
    RCSOFTCHECK(rcl_node_fini(&node));
    rclc_executor_fini(&executor);
    rclc_support_fini(&support);
}
