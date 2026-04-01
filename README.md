# RC 3D printed car
This project consists of building an RC car using 3D-printed parts, controlled by a Raspberry Pi Pico.

The aim is to design the car using the basic components of a car, such as the chassis, suspension, wheels, transmission, and other essential mechanical parts. Additionally, it will use an RF transmitter to control the car remotely, and it will be programmed using a Raspberry Pi Pico, implementing communication protocols such as UART, I2C, and SPI.

## Hardware
- Raspberry Pi Pico
- NRF24L01 RF transmitter
- Bearings
- Shock absorbers
- Servomotors
- DC motor
- Cardan joints
- Spherical joints

## How it works
Through  a xbox controller and the "xinput.h" library for Windows, the pc will read the buttons and analog joysticks in order to send them at serial port. A Raspberry Pico connected via UART at the same pc will get the data transfered, will decodify it and will send it throught a RF transmitter to another Raspberry Pi Pico located at the car. Finally, this Raspberry Pi Pico will process the data and will convert it to certain values to activate various PWM signals for the motors.

## What i learned
Basics of:
- Compilating projects with CMake
- C language orientated to microcontrollers and their features, such as configuration, mascaring bits and processing data
- Protocol communications, such as UART, I2C and SPI
- Troubleshooting hardware issues
