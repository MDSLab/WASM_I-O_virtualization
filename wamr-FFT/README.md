# WAMR LED Toggle on Zephyr

Progetto dimostrativo che esegue un'applicazione **WebAssembly** su un MCU
**STM32** tramite **WAMR** (WebAssembly Micro Runtime) e **Zephyr RTOS**.

Il modulo WASM chiama funzioni native (GPIO Zephyr) per fare il toggle del LED,
senza avere accesso diretto all'hardware — il runtime WAMR fa da intermediario.

```
┌─────────────────────────────────────────────┐
│               Zephyr RTOS                   │
│                                             │
│  ┌──────────────────────────────────────┐   │
│  │        WAMR Runtime (vmlib)          │   │
│  │                                      │   │
│  │  ┌────────────────────────────────┐  │   │
│  │  │   led_toggle.wasm              │  │   │
│  │  │                                │  │   │
│  │  │   app_main()                   │  │   │
│  │  │     loop:                      │  │   │
│  │  │       call led_toggle  ─────────┼──┼──┼──► gpio_pin_toggle_dt()
│  │  │       call sleep_ms(500) ───────┼──┼──┼──► k_msleep(500)
│  │  └────────────────────────────────┘  │   │
│  └──────────────────────────────────────┘   │
└─────────────────────────────────────────────┘
```

---

## Struttura del progetto

```
wamr-zephyr-led/
├── CMakeLists.txt              # Build principale, configura WAMR
├── prj.conf                    # Kconfig Zephyr
├── build_wasm.sh               # Script per compilare il modulo WASM
│
├── src/
│   ├── main.c                  # Host: init WAMR, carica e lancia il WASM
│   ├── wasm_export_native.c    # Funzioni native (GPIO) esposte al WASM
│   ├── wasm_native_led.h       # Dichiarazioni funzioni native
│   └── led_toggle_wasm.h       # Binario WASM embedded come array C
│
├── wasm_app/
│   └── led_toggle.c            # Sorgente C dell'app WASM (lato guest)
│
└── boards/
    └── nucleo_f446re.overlay   # Overlay DTS per NUCLEO-F446RE
```

---

## Prerequisiti

| Tool | Versione minima | Note |
|------|----------------|-------|
| Zephyr SDK | 0.16+ | Includes Arm toolchain |
| West | qualsiasi | `pip install west` |
| WAMR | main / latest | clonare separatamente |
| wasi-sdk | 20+ | Per compilare il modulo WASM |
| xxd | qualsiasi | `apt install xxd` |

---

## Setup ambiente

### 1. Clonare WAMR

```bash
cd ~
git clone https://github.com/bytecodealliance/wasm-micro-runtime.git
export WAMR_ROOT=$HOME/wasm-micro-runtime
```

### 2. Workspace Zephyr (se non già configurato)

```bash
west init ~/zephyrproject
cd ~/zephyrproject
west update
west zephyr-export
pip install -r zephyr/scripts/requirements.txt
```

### 3. Installare wasi-sdk

```bash
# Scarica l'ultima release da:
# https://github.com/WebAssembly/wasi-sdk/releases
# Esempio per Linux x86_64:
wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-21/wasi-sdk-21.0-linux.tar.gz
tar xf wasi-sdk-21.0-linux.tar.gz -C /opt
export WASI_SDK=/opt/wasi-sdk-21.0
```

---

## Build

### Step 1 – Compilare il modulo WASM

```bash
cd wamr-zephyr-led
export WASI_SDK=/opt/wasi-sdk-21.0   # adatta al tuo path
bash build_wasm.sh
```

Questo produce `wasm_app/led_toggle.wasm` e aggiorna `src/led_toggle_wasm.h`.

> **Nota:** il repository include già un `led_toggle_wasm.h` con un binario
> WASM hand-assembled funzionante, quindi questo step è opzionale per il
> primo test.

### Step 2 – Build Zephyr

```bash
# Per NUCLEO-F446RE (STM32F446RE)
west build -b nucleo_f446re . -- -DWAMR_ROOT=$WAMR_ROOT

# Per altre board (es. nrf52840dk)
west build -b nrf52840dk_nrf52840 . -- -DWAMR_ROOT=$WAMR_ROOT
```

### Step 3 – Flash

```bash
west flash
```

### Step 4 – Monitor seriale

```bash
west espressif monitor   # oppure
screen /dev/ttyACM0 115200
# oppure
minicom -D /dev/ttyACM0 -b 115200
```

Output atteso:

```
*** Booting Zephyr OS build v3.x.x ***
[00:00:00.001] INF main: WAMR LED Toggle - starting
[00:00:00.012] INF main: WAMR runtime initialised
[00:00:00.013] INF native_led: Registered 4 native LED symbols
[00:00:00.045] INF main: WASM module loaded (NNN bytes)
[00:00:00.046] INF main: WASM module instantiated
[00:00:00.046] INF main: Calling WASM app_main()...
[00:00:00.046] DBG native_led: LED TOGGLE
[00:00:00.546] DBG native_led: LED TOGGLE
[00:00:01.046] DBG native_led: LED TOGGLE
...
```

---

## Personalizzazione

### Cambiare il periodo di blink

Modificare `wasm_app/led_toggle.c`:

```c
sleep_ms(250);   // 250 ms invece di 500 ms
```

Poi ricompilare il WASM con `build_wasm.sh`.

### Target diverso (Cortex-M33, THUMBV8)

In `CMakeLists.txt`:

```cmake
set(WAMR_BUILD_TARGET "THUMBV8")
```

### Abilitare AOT (Advanced Optimization)

Per board con più flash (≥ 256 KB) si può abilitare AOT:

```cmake
set(WAMR_BUILD_AOT 1)
set(WAMR_BUILD_INTERP 0)
```

Richiede la toolchain `wamrc` per pre-compilare il `.wasm` in `.aot`.

---

## Note tecniche

- **Heap WAMR**: 48 KB allocati in BSS (`wamr_global_heap[]`).
  Ridurre `WAMR_GLOBAL_HEAP_SIZE` se la RAM è limitata (min ~16 KB).
- **Isolamento**: il codice WASM non può accedere direttamente ai registri
  hardware — deve passare attraverso le funzioni native registrate.
- **Portabilità**: lo stesso `.wasm` gira su qualsiasi board Zephyr con
  il solo adattamento del layer nativo.
