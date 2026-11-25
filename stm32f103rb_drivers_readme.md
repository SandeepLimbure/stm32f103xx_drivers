# STM32F103RB Bare-Metal Peripheral Drivers (GPIO, SPI, I2C)

A modular, bare-metal peripheral driver library written from scratch in Embedded C for the **STM32F103RB (ARM Cortex-M3)** microcontroller. All register structures, memory maps, clock calculations, and peripheral APIs are built directly from the **STM32F103xx Reference Manual (RM0008)** without relying on STM32 HAL or LL libraries.

---

## Project Overview

| Parameter                | Details                                      |
| :----------------------- | :------------------------------------------- |
| **Target MCU**           | STM32F103RBTx (ARM Cortex-M3, 32-bit RISC)   |
| **Development Board**    | STM32 Nucleo-F103RB                          |
| **Programming Language** | Embedded C (Bare-Metal)                      |
| **IDE & Toolchain**      | STM32CubeIDE / ARM GCC (`arm-none-eabi-gcc`) |
| **Implemented Modules**  | RCC, NVIC, AFIO, EXTI, GPIO, SPI, I2C        |
| **Reference Manuals**    | ST RM0008 (STM32F103xx), ARM Cortex-M3 TRM   |

---

## Features

### 1. MCU Device Header (`stm32f103xx.h`)

- Memory-mapped base addresses for Flash, SRAM, AHB, APB1, and APB2 peripheral buses.
- Register definition structures (`GPIO_RegDef_t`, `RCC_RegDef_t`, `AFIO_RegDef_t`, `EXTI_RegDef_t`, `SPI_RegDef_t`, `I2C_RegDef_t`).
- Cortex-M3 NVIC `ISERx`, `ICERx`, and `IPRx` register macros along with STM32F103 IRQ numbers and priority levels.
- Peripheral clock enable/disable macros, peripheral reset macros (`APB1RSTR` / `APB2RSTR`), and register bit-position definitions.

### 2. GPIO & EXTI Driver (`stm32f103xx_gpio_driver.h/.c`)

- Full `CRL`/`CRH` pin configuration supporting Input (Analog, Floating, Pull-Up, Pull-Down), Output (Push-Pull / Open-Drain at 2 MHz, 10 MHz, 50 MHz), and Alternate Function modes.
- Atomic pin set/reset via `BSRR` and `BRR`, plus port-level read/write and pin toggle APIs.
- External interrupt (`EXTI`) support with `AFIO->EXTICR` routing, rising/falling/dual-edge triggering, NVIC interrupt enable/priority configuration, and write-1-to-clear pending register handling.

### 3. SPI Driver (`stm32f103xx_spi_driver.h/.c`)

- Supports Master and Slave modes across Full-Duplex, Half-Duplex, and Simplex RX-only configurations.
- Configurable Data Frame Format (8-bit / 16-bit), `CPOL`, `CPHA`, Baud Rate Prescalers (`DIV2` to `DIV256`), and Hardware/Software Slave Management (`SSM`, `SSI`, `SSOE`).
- Blocking (`SPI_SendData`, `SPI_ReceiveData`) and interrupt-driven (`SPI_SendDataIT`, `SPI_ReceiveDataIT`) APIs.
- Interrupt handler (`SPI_IRQHandling`) managing `TXE`, `RXNE`, and `OVR` events with weak application callbacks.

### 4. I2C Driver (`stm32f103xx_i2c_driver.h/.c`)

- Runtime `PCLK1` calculation from `RCC->CFGR` (HSI, HSE, PLL, AHB/APB1 prescalers) for automated `CCR` and `TRISE` timing setup.
- Supports **Standard Mode (100 kHz)** and **Fast Mode (200 kHz / 400 kHz)** with configurable Fast Mode duty cycles (`2` and `16/9`).
- Blocking (`I2C_MasterSendData`, `I2C_MasterReceiveData`) and non-blocking interrupt-driven (`I2C_MasterSendDataIT`, `I2C_MasterReceiveDataIT`) Master transfers with Repeated Start (`Sr`) support.
- Slave mode send/receive helpers and separate Event (`I2C_EV_IRQHandling`) and Error (`I2C_ER_IRQHandling`) ISRs handling `BERR`, `ARLO`, `AF`, `OVR`, and `TIMEOUT` faults.

---

## Repository Structure

```text
stm32f103xx_drivers/
├── drivers/
│   ├── Inc/
│   │   ├── stm32f103xx.h               # MCU memory map, register structs, NVIC, & bit macros
│   │   ├── stm32f103xx_gpio_driver.h   # GPIO & EXTI driver header
│   │   ├── stm32f103xx_spi_driver.h    # SPI driver header
│   │   └── stm32f103xx_i2c_driver.h    # I2C driver header
│   └── Src/
│       ├── stm32f103xx_gpio_driver.c   # GPIO & EXTI driver implementation
│       ├── stm32f103xx_spi_driver.c    # SPI driver implementation
│       └── stm32f103xx_i2c_driver.c    # I2C driver implementation
├── Src/
│   ├── 001ledblink.c                   # GPIO output toggle (LED blink)
│   ├── led_button.c                    # Polling-based button input & LED control
│   ├── 003_button_interrupt.c          # EXTI interrupt-driven button handling
│   ├── 006spi_tx_testing.c             # SPI Master transmit test
│   ├── 007_spi_txonly_arduino.c        # STM32 SPI Master -> Arduino Slave TX test
│   ├── 008_spi_cmd_handling.c          # Bidirectional SPI command-response protocol (Arduino)
│   ├── 010_i2c_master_tx_testing.c     # I2C Master transmit test
│   ├── main.c                          # Default entry point
│   ├── syscalls.c
│   └── sysmem.c
├── Startup/
│   └── startup_stm32f103rbtx.s         # Startup assembly & vector table
└── STM32F103RBTX_FLASH.ld              # Flash linker script
```

---

## Hardware Pin Mapping (Nucleo-F103RB)

| Peripheral           | Signal                          | Pin                               | Mode                                                               |
| :------------------- | :------------------------------ | :-------------------------------- | :----------------------------------------------------------------- |
| **User LED (LD2)**   | Output                          | `PA5`                             | Output Push-Pull                                                   |
| **User Button (B1)** | Input / EXTI                    | `PC13`                            | Input / `EXTI15_10` (Active Low)                                   |
| **SPI2**             | `NSS` / `SCK` / `MISO` / `MOSI` | `PB12` / `PB13` / `PB14` / `PB15` | Alternate Function Push-Pull                                       |
| **I2C1**             | `SCL` / `SDA`                   | `PB6` / `PB7`                     | Alternate Function Open-Drain (external 4.7 kOhm pull-ups to 3.3V) |

---

## Driver API Summary

| Driver   | Key APIs                                                                                                                                                                                                                                                     |
| :------- | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **GPIO** | `GPIO_PeriClockControl`, `GPIO_Init`, `GPIO_DeInit`, `GPIO_ReadFromInputPin`, `GPIO_ReadFromInputPort`, `GPIO_WriteToOutputPin`, `GPIO_WriteToOutputPort`, `GPIO_ToggleOutputPin`, `GPIO_IRQInterruptConfig`, `GPIO_IRQPriorityConfig`, `GPIO_IRQHandling`   |
| **SPI**  | `SPI_PeriClockControl`, `SPI_Init`, `SPI_DeInit`, `SPI_SendData`, `SPI_ReceiveData`, `SPI_SendDataIT`, `SPI_ReceiveDataIT`, `SPI_PeripheralControl`, `SPI_SSIConfig`, `SPI_SSOEConfig`, `SPI_IRQInterruptConfig`, `SPI_IRQPriorityConfig`, `SPI_IRQHandling` |
| **I2C**  | `I2C_PeriClockControl`, `I2C_Init`, `I2C_DeInit`, `I2C_MasterSendData`, `I2C_MasterReceiveData`, `I2C_MasterSendDataIT`, `I2C_MasterReceiveDataIT`, `I2C_SlaveSendData`, `I2C_SlaveReceiveData`, `I2C_EV_IRQHandling`, `I2C_ER_IRQHandling`                  |

---

## Getting Started (STM32CubeIDE)

1. **Clone the repository**

   ```bash
   git clone https://github.com/SandeepLimbure/stm32f103xx_drivers.git
   ```

2. **Import the project**
   - Open **STM32CubeIDE**.
   - Navigate to `File -> Open Projects from File System...` and select the cloned `stm32f103xx_drivers` directory.

3. **Choose one active application in `Src/`**
   - Multiple test files in `Src/` contain a `main()` function. Keep only **one** application file included in the build at a time.
   - Right-click the other test `.c` files -> `Resource Configurations -> Exclude from Build...` -> select `Debug` and `Release`.

4. **Confirm include paths**
   - Ensure `drivers/Inc` is listed under `Project Properties -> C/C++ Build -> Settings -> MCU GCC Compiler -> Include paths`.

5. **Build and flash**
   - Build the project (`Ctrl + B`).
   - Connect the STM32 Nucleo-F103RB board via ST-Link and flash using `Run -> Debug`.

---

## License

This project is developed for academic and educational purposes. Feel free to explore, fork, or adapt it for learning bare-metal STM32 development.
