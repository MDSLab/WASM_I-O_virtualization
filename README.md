# WASM I/O peripherals virtualization

This repository contains a proof-of-concept demonstrating transparent, cross-architecture peripheral access between heterogeneous MCU nodes. WebAssembly applications running on top of WAMR (WebAssembly Micro Runtime) inside Zephyr threads communicate through micro-ROS, allowing one node to operate a peripheral physically attached to another node as if it were local.

## Repository Layout

The source files in this repository are intended to be placed under:

```
modules/lib/micro_ros/src/
```

within a Zephyr workspace that already includes the `micro_ros_zephyr_module` integration.

| File | Role |
|------|------|
| `main.c` | Zephyr entry point. Initializes WAMR, loads the AOT-compiled WASM module, instantiates it, and spawns a dedicated Zephyr thread that executes the WASM `app_main` function. |
| `wasm_native.c` | Native host implementations exported to the WASM sandbox via the `NativeSymbol` API. Provides `micro_ros_service_init`, `micro_ros_app_init`, `green_led_toggle`, and `sleep_ms`. All micro-ROS / DDS-XRCE setup and GPIO access live here — the WASM module never touches hardware directly. |
| `service_app.c` | WASM application running on the Nucleo. Calls `micro_ros_service_init` to expose the `/green_led` endpoint. |
| `led_toggle_app.c` | WASM application running on the ESP32-S3. Calls `micro_ros_app_init` and then repeatedly invokes `green_led_toggle` in an infinite loop. |

## Experimental Setup

The experiment involved the use of two boards: a **ST NUCLEO-H723ZG** (Arm Cortex-M7 MCU) and an **ESP32-S3-DevKitC-1** (Xtensa dual-core MCU), representing two distinct MCU architectures.

Inter-node communication is handled through micro-ROS, which provides a lightweight DDS abstraction specifically designed for resource-constrained environments via the DDS-XRCE (eXtremely Resource Constrained Environments) protocol. Since DDS-XRCE requires a bridge between the constrained nodes and the full DDS domain, a PC hosting two micro-ROS agents — one per board — is used to connect the two nodes via serial communication, acting as a protocol gateway toward the broader DDS infrastructure.

Each application is containerized as a WASM module and then scheduled as a Zephyr thread hosting the WAMR execution runtime. For the purposes of this experiment, each board hosts a single thread; however, the target architecture supports multiple concurrent applications per node.

```
+------------------+        serial         +------------------------+        serial         +------------------+
|  NUCLEO-H723ZG   | <-------------------> |   PC (DDS Gateway)     | <-------------------> |  ESP32-S3        |
|  WASM app        |                       |   micro-ROS Agent x2   |                       |  WASM app        |
|  (owns the LED)  |                       |     full DDS domain    |                       |  (toggles LED)   |
+------------------+                       +------------------------+                       +------------------+
```

### Nucleo side (NUCLEO-H723ZG)

A WASM module exposes a ROS 2 service named `/green_led`, which toggles a green LED physically connected to the board. Upon initialization, the module invokes `micro_ros_service_init`, linked to a native host implementation that sets up the micro-ROS communication stack and registers a service callback. Each time a request is received from a remote node, the callback invokes Zephyr's `gpio_pin_toggle_dt`, performing the actual GPIO operation.

### ESP32-S3 side (ESP32-S3-DevKitC-1)

A WASM application running on the ESP32-S3 enters, after initialization, an infinite loop in which it repeatedly calls `green_led_toggle`, a function whose native host implementation issues the corresponding ROS 2 request to the Nucleo board.

From the developer's perspective, the call behaves as if the LED were physically attached to the ESP32. The underlying DDS communication, protocol bridging, and cross-architecture routing remain entirely hidden — the WASM module is unaware of where the peripheral physically resides.

> **Note on API naming.** In this proof-of-concept, the API names exposed to the WASM modules reflect the underlying micro-ROS implementation in order to maintain a direct mapping between the application logic and the communication stack. A more abstract naming layer, fully decoupled from the middleware, is planned as part of the ongoing development of the system.

## Building the WASM Modules

Both WASM applications are compiled with the WASI SDK and then converted into C header arrays embedded in the firmware.

**`service_app` (Nucleo):**

```bash
/opt/wasi-sdk/bin/clang --target=wasm32-unknown-unknown \
    -nostdlib -Wl,--no-entry \
    -Wl,--export=app_main -Wl,--allow-undefined \
    -O2 -o service_app.wasm service_app.c
xxd -i service_app.wasm > ../src/service_app_wasm.h
```

**`led_toggle_app` (ESP32-S3):**

```bash
/opt/wasi-sdk/bin/clang --target=wasm32-unknown-unknown \
    -nostdlib -Wl,--no-entry \
    -Wl,--export=app_main -Wl,--allow-undefined \
    -O2 -o led_toggle_app.wasm led_toggle_app.c
xxd -i led_toggle_app.wasm > ../src/led_toggle_app_wasm.h
```

For deployment, the `.wasm` files should be further compiled to AOT format using `wamrc` targeting the architecture of each node (Arm Cortex-M7 for the Nucleo, Xtensa for the ESP32-S3) and embedded in the firmware in place of the interpreted bytecode.

## Building the Firmware

Standard Zephyr workflow using `west`:

```bash
# Nucleo build
west build -b nucleo_h723zg -p always .

# ESP32-S3 build
west build -b esp32s3_devkitc -p always .
```

Flash with `west flash` on each board.

## Running the Demo

1. Launch two `micro_ros_agent` instances on the host PC, one per serial port:
   ```bash
   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyACM0  # Nucleo
   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0  # ESP32-S3
   ```
2. Power on both boards. The Nucleo registers the `/green_led` endpoint; the ESP32-S3 starts issuing requests.
3. The green LED on the Nucleo toggles in response to each request originating from the ESP32-S3.

## Design Notes

- **Sandbox boundary.** WASM modules never access hardware registers directly. All peripheral and middleware operations live in native functions registered through WAMR's `NativeSymbol` API.
- **Threading.** WAMR runs inside a dedicated Zephyr thread (`wasm_thread_entry` in `main.c`). The architecture supports multiple concurrent WASM applications per node, each in its own thread.
- **Transport.** The custom Zephyr transport (`zephyr_transport_open/close/read/write`) is provided by the `micro_ros_zephyr_module` and registered with `rmw_uros_set_custom_transport`.
- **Transparency.** Neither WASM application is aware of the physical location of the LED. The naming on the application side (`green_led_toggle`) reflects the operation, not the network role.
