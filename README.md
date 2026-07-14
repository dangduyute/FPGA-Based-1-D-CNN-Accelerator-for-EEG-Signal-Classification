# FPGA-Based 1-D CNN Accelerator for EEG Signal Classification

## Overview

This project presents the design and implementation of a **1-D Convolutional Neural Network (CNN) accelerator** on the **AMD/Xilinx Kria KV260 FPGA platform** for EEG signal classification.

The accelerator is implemented in Verilog and supports the main operations required by the CNN model, including convolution, pooling, activation, residual addition, and data movement. An **AXI4-Full interface** is used to provide efficient burst-based communication between the Processing System (PS) and the hardware accelerator in the Programmable Logic (PL).

A Linux-based control application is also developed to configure the accelerator, load EEG samples and model parameters, start inference, monitor processing status, and retrieve classification results.

## Key Features

- FPGA-based 1-D CNN accelerator implemented in Verilog
- Designed for EEG signal classification
- Support for convolution, pooling, activation, and residual operations
- AXI4-Full interface for high-speed burst data transfer between the Processing System and Programmable Logic
- Linux-based software for hardware control and inference execution
- Hardware–software co-design on the KV260 platform
- Simulation testbenches for functional verification
- Real-time result visualization through a WebSocket-based interface

## System Architecture

The system consists of two main parts:

### Programmable Logic

The hardware accelerator is implemented in the FPGA fabric and performs the computationally intensive CNN operations.

Main hardware components include:

- Processing units for convolution and arithmetic operations
- Local memories for intermediate feature maps
- Memories for weights, biases, and configuration parameters
- Control logic for layer-by-layer CNN execution
- AXI4-Full slave interface for data communication with the Processing System

### Processing System

The Processing System runs Linux and controls the complete inference process.

The control software is responsible for:

- Loading EEG input samples
- Loading CNN weights, biases, and configuration parameters
- Configuring the accelerator
- Starting hardware inference
- Monitoring completion status
- Reading output data
- Performing final classification processing
- Sending results to the web interface

## Data Flow

1. EEG samples and model parameters are loaded by the Linux application.
2. Data is transferred from the Processing System to the accelerator through AXI4-Full burst transactions.
3. The accelerator executes the CNN layers in hardware.
4. Intermediate results are stored in local memories.
5. The final output is transferred back to the Processing System.
6. The software retrieves and displays the classification result.

## Hardware Platform

- **Board:** AMD/Xilinx Kria KV260 Vision AI Starter Kit
- **FPGA Design Tool:** Vivado
- **Embedded Linux:** PetaLinux
- **Hardware Description Language:** Verilog
- **Communication Interface:** AXI4-Full
- **Control Software:** C
## Demo Video

Watch the demonstration of the FPGA-based 1-D CNN accelerator for EEG signal classification:

[![Watch the demo](https://img.youtube.com/vi/VgfGSVWmEwo/0.jpg)](https://youtu.be/VgfGSVWmEwo)



