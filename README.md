[![CI](https://github.com/matthias-bs/growatt2lorawan/actions/workflows/CI.yml/badge.svg)](https://github.com/matthias-bs/growatt2lorawan/actions/workflows/CI.yml)
[![GitHub release](https://img.shields.io/github/release/matthias-bs/growatt2lorawan?maxAge=3600)](https://github.com/matthias-bs/growatt2lorawan/releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-green)](https://github.com/matthias-bs/growatt2lorawan/blob/main/LICENSE)

# growatt2lorawan
LoRaWAN Node for Growatt Photovoltaic Inverter Modbus Data Interface

:construction_worker: **Work in Progress!** :construction_worker: 

This is a "remix" of 
* [matthias-bs/BresserWeatherSensorTTN](https://github.com/matthias-bs/BresserWeatherSensorTTN) (or [arduino_lorawan_esp32_example](https://github.com/mcci-catena/arduino-lorawan/tree/master/examples/arduino_lorawan_esp32_example) if you will) - for the LoRaWAN part

and

* [nygma2004/growatt2mqtt](https://github.com/nygma2004/growatt2mqtt) - for the Growatt PV Inverter / Modbus part

## Hardware Requirements
* ESP32 (optionally with LiPo battery charger and battery)
* SX1276 (or compatible) LoRaWAN Radio Transceiver
* LoRaWAN Antenna
* RS485 Transceiver - 3.3V compatible, half-duplex capable (e.g [Waveshare 4777](https://www.waveshare.com/wiki/RS485_Board_(3.3V)) module)

## Power Supply

The ESP32 development board can be powerd from the inverter's USB port **which only provides power if the inverter is active**.

**No sun - no power - no transmission!** :sunglasses:

But: Some ESP32 boards have an integrated LiPo battery charger. You could power the board from a battery while there is no PV power (at least for a few hours). 

## Library Dependencies

* [MCCI Arduino Development Kit ADK](https://github.com/mcci-catena/Catena-mcciadk) by MCCI
* [MCCI LoRaWAN LMIC library](https://github.com/mcci-catena/arduino-lmic) by Thomas Telkamp and Matthijs Kooijman / Terry Moore, MCCI
* [MCCI Arduino LoRaWAN Library](https://github.com/mcci-catena/arduino-lorawan) by Terry Moore, MCCI
* [Lora-Serialization](https://github.com/thesolarnomad/lora-serialization) by Joscha Feth
* [ESP32Time](https://github.com/fbiego/ESP32Time) by Felix Biego
* [ModbusMaster](https://github.com/4-20ma/ModbusMaster) by Doc Walker

## Pinning Configuration

See [src/settings.h](https://github.com/matthias-bs/growatt2lorawan/blob/main/src/settings.h)

| GPIO define   | Waveshare 4777 pin  |
| ------------- | ------------------- |
| MAX485_DE     | RSE                 |
| MAX485_RE_NEG | n.c.                |
| MAX485_RX     | RO                  |
| MAX485_TX     | DI                  |

## MQTT Integration and IoT MQTT Panel Example

[IoT MQTT Panel](https://snrlab.in/iot/iot-mqtt-panel-user-guide)

<img src="https://user-images.githubusercontent.com/83612361/225129950-c323e0c7-a58b-4a3f-ba30-e0fd9adc1594.jpg" alt="Screenshot_20230314-130325_IoT_MQTT_Panel_Pro-1" width="400">
