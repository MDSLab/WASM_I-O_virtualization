# micro-ROS Publisher — NUCLEO-F446RE + Zephyr (Serial UART)

Publisher minimale che invia `std_msgs/Int32` con valore incrementale
ogni secondo sul topic `/zephyr_int32_publisher`, usando UART2
(Virtual COM Port ST-Link).

---

## Struttura del progetto

```
microros_publisher_nucleo/
├── west.yml                  # Manifest: Zephyr v4.1.0 + micro_ros_zephyr_module
├── CMakeLists.txt
├── prj.conf                  # Kconfig: micro-ROS, UART, heap/stack
├── boards/
│   └── nucleo_f446re.overlay # DTS: aggancia USART2 al trasporto micro-ROS
└── src/
    └── main.c                # Publisher con timer 1 Hz
```

---

## Setup iniziale (una tantum)

### 1. Dipendenze Python

```bash
pip3 install catkin_pkg lark-parser empy colcon-common-extensions
```

### 2. Aggiunta del modulo al workspace west

Copia il contenuto di `west.yml` in quello del tuo workspace
(o usalo come manifest principale), poi:

```bash
west update
```

> **Nota versione**: il branch `main` del modulo è compatibile con Zephyr v4.1.0.
> Se usi Jazzy/Iron/Humble, cambia `revision:` nel west.yml con
> `jazzy`, `iron` o `humble` rispettivamente.

---

## Build

```bash
west build -b nucleo_f446re \
           -p always \
           -- -DBOARD_ROOT=$(pwd)
```

La prima build scarica e compila `libmicroros.a` tramite colcon:
può richiedere qualche minuto.

---

## Flash

```bash
west flash
```

---

## Avvio Agent (PC)

Connetti il NUCLEO via USB (Virtual COM Port ST-Link) e avvia l'agent:

```bash
# Con Docker (modo più semplice):
docker run -it --rm \
  -v /dev:/dev \
  --privileged \
  --net=host \
  microros/micro-ros-agent:jazzy \
  serial --dev /dev/ttyACM0 -b 115200 -v6

# Con ROS 2 installato in locale:
ros2 run micro_ros_agent micro_ros_agent serial \
     --dev /dev/ttyACM0 -b 115200 -v6
```

Sostituisci `jazzy` con la tua distro ROS 2.

---

## Verifica

Su un altro terminale (con ROS 2 sourced):

```bash
ros2 topic echo /zephyr_int32_publisher std_msgs/msg/Int32
```

Dovresti vedere:
```
data: 0
---
data: 1
---
data: 2
---
```

---

## Troubleshooting

| Sintomo | Causa | Fix |
|---------|-------|-----|
| `UART device not ready` | overlay non applicato | Assicurati che `boards/nucleo_f446re.overlay` sia nella root del progetto |
| `Agent not found` loop infinito | Agent non avviato / porta sbagliata | Controlla `/dev/ttyACM*` con `ls /dev/ttyACM*` |
| Build fallisce su `libmicroros` | Mancano dipendenze Python | Reinstalla con `pip3 install catkin_pkg lark-parser empy colcon-common-extensions` |
| Flash non parte | Board non riconosciuta | Verifica che `west flash` trovi la board con `west boards` |
