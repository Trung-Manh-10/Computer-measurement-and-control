# REMOTE MONITORING AND CONTROL OF AGRICULTURAL DRYING TEMPERATURE USING NODE-RED AND MQTT (My team project - course project)

A remote agricultural drying system designed to monitor and control the drying chamber temperature in real time using ESP32, Node-RED, MQTT, and PID control.

The project combines embedded programming, electronic hardware design, power control, IoT communication, and temperature control to develop an automated drying system.

---
Citation:  
You can check the document file there: 
```
https://docs.google.com/document/d/1uAhRXvkbzyLu1bBwa1L5f1M4FZBYYodd/edit?usp=sharing&ouid=100423899415976353201&rtpof=true&sd=true
```
---

## Project Overview

The system monitors the temperature inside the drying chamber using a DS18B20 temperature sensor. The ESP32 processes the sensor data and controls the heating lamp and ventilation fan to maintain the desired temperature.

A PID controller is implemented on the ESP32 to regulate the heating power through PWM control. The heating load is driven by an IRLZ44N MOSFET power circuit, while a relay controls the ventilation fan.

The ESP32 communicates with a **Node-RED dashboard through the MQTT protocol**, allowing users to monitor and control the drying process remotely over Wi-Fi.


---

## Key Features

* Real-time temperature measurement using **DS18B20**
* Automatic temperature regulation using **PID control**
* **PWM-based heating power control**
* MOSFET-based power circuit for the heating load
* Remote monitoring and control through **Node-RED**
* Wireless communication using **Wi-Fi and MQTT**

---

## Hardware Design

The system hardware consists of several functional blocks:

* **ESP32 DevKit** – main controller and Wi-Fi communication
* **DS18B20** – temperature sensing
* **IRLZ44N + 4N25** – isolated MOSFET power control for the heating lamp
* **Relay module** – ventilation fan control
* **LM2596** – DC voltage regulation

The hardware also includes a custom enclosure and drying chamber designed to integrate the electronic control system, heating elements, fan, and temperature sensor.

---


## PID Temperature Control

The temperature control system uses a PID controller to minimize the difference between the desired temperature and the measured temperature.

The final experimentally tuned parameters were:

```text
Kp = 330
Ki = 0.9
Kd = 0.1
```

The PID output is converted into a PWM duty cycle from **0–255** to control the heating power.

Experimental results showed a temperature error ranging from approximately **±0.1°C to ±0.5°C**, depending on the operating temperature range.

---

## MQTT & Node-RED

The system uses the **MQTT publish/subscribe architecture** for communication between the ESP32 and Node-RED.

### ESP32 → Node-RED

The ESP32 publishes:

* Set temperature
* Current temperature
* Heating PWM
* System status
* Set drying time
* Remaining drying time

### Node-RED → ESP32

Node-RED sends:

* Temperature setpoint
* Drying time
* System ON/OFF command

The Node-RED dashboard provides:

* Real-time temperature gauge
* Temperature history graph
* Temperature and drying-time settings
* Heating power display
* System ON/OFF control
* Remaining drying time

---

## Results

The completed system was successfully assembled and tested under different temperature setpoints.

Experimental evaluation demonstrated:

* Stable temperature control
* No significant overshoot in the tested temperature ranges
* Temperature error of approximately **±0.1°C to ±0.5°C**
* Remote monitoring and control through Node-RED
* Stable MQTT communication during testing
* Independent local temperature control on the ESP32

The system achieved a practical combination of **embedded control, IoT communication, electronic hardware, and automatic temperature regulation** for small-scale agricultural drying applications.

---

