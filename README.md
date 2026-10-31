# ECODRAIN ILAP Project

We built this ESP32-based bioswale monitoring and water-diversion system.

## Features

- Monitors water level, soil moisture, temperature, and humidity using an ESP32.
- Automatically controls a relay based on the detected water level.
- Provides a live web dashboard for monitoring sensor readings and relay status.
- Stores Wi-Fi credentials in the ESP32's flash memory.
- Starts a setup access point when the ESP32 cannot connect to Wi-Fi.

## Hardware

- ESP32 development board
- Water-level sensor
- Soil-moisture sensor
- DHT11 temperature and humidity sensor
- Active-low relay module

## Pin Configuration

| Component | ESP32 Pin |
|---|---|
| Water-level sensor | GPIO 34 |
| Soil-moisture sensor | GPIO 33 |
| DHT11 sensor | GPIO 25 |
| Bioswale relay | GPIO 26 |

## Setup Instructions

1. Open `esp32main.ino` in the Arduino IDE.
2. Install the ESP32 board package and the **DHT sensor library**.
3. Connect the sensors and relay according to the pin configuration above.
4. Upload the sketch to the ESP32.
5. If setup mode starts, connect to the `ESP32-Bioswale-Setup` Wi-Fi network using the password `12345678`.
6. Open the IP address displayed in the Serial Monitor, configured to **115200 baud**.
7. Enter your Wi-Fi credentials through the web dashboard.

## Web Dashboard and API

The ESP32 hosts a web dashboard that displays live sensor readings and relay status.

Sensor data can also be retrieved programmatically through the `/data` endpoint.
