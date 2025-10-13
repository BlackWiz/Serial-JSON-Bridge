# Serial-JSON-Bridge
A lightweight and easy-to-use library for STM32 microcontrollers to handle command and control over a UART serial connection using the JSON data format.

**Project Overview**

The core of this project is to bridge the gap between high-level applications and low-level hardware control. A Python script running on a host PC creates and sends JSON-formatted commands. An STM32 microcontroller receives this data via UART, parses the JSON string to extract commands and parameters, and then directly manipulates hardware peripherals—such as controlling the brightness of an LED or the angle of a servo motor using PWM.

This project serves as a practical example of a host-controller system, a fundamental design pattern in robotics, IoT, and industrial automation.

**Architecture**

The data flows from the host computer to the microcontroller, which then actuates the hardware.

-> `JSON Command String` -> `UART Serial` -> -> Parse JSON -> ``

**Key Features**

Structured Command Protocol: Utilizes JSON for sending clear, human-readable, and easily expandable commands.

Serial Communication: Employs the fundamental UART protocol for reliable data transfer between the host and microcontroller.   

On-Device Parsing: The resource-constrained STM32 microcontroller parses the incoming JSON strings to make real-time decisions.

Real-Time Hardware Control: Demonstrates precise hardware control by manipulating PWM signals to control an LED's brightness and a servo's position.
