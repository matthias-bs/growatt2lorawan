[![CI](https://github.com/matthias-bs/growatt2lorawan/actions/workflows/CI.yml/badge.svg)](https://github.com/matthias-bs/growatt2lorawan/actions/workflows/CI.yml)
[![GitHub release](https://img.shields.io/github/release/matthias-bs/growatt2lorawan?maxAge=3600)](https://github.com/matthias-bs/growatt2lorawan/releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-green)](https://github.com/matthias-bs/growatt2lorawan/blob/main/LICENSE)

> [!WARNING]
> This repository is deprecated and maintenance is reduced to a minimum.
>
> The recommended alternative is [growatt2lorawan-v2](https://github.com/matthias-bs/growatt2lorawan-v2)
> which provides a few more features and has a much cleaner architecture.
>
> Furthermore, the underlying LoRaWAN libraries [MCCI LoRaWAN LMIC library](https://github.com/mcci-catena/arduino-lmic)  and [MCCI Arduino LoRaWAN Library](https://github.com/mcci-catena/arduino-lorawan) seem not to be maintained any longer.
> 
> Only if you are using the Helium Network, you will have to stick with 
> [growatt2lorawan](https://github.com/matthias-bs/growatt2lorawan), because Helium requires LoRaWAN v1.0.X while [growatt2lorawan-v2](https://github.com/matthias-bs/growatt2lorawan-v2) is based on [RadioLib](https://github.com/jgromes/RadioLib), which implements LoRaWAN v1.1.

# growatt2lorawan
LoRaWAN Node for Growatt Photovoltaic Inverter Modbus Data Interface

This project can be used if you want to monitor your PV Inverter in case it is out of reach of your WiFi access point (and a WiFi repeater would not help/is not wanted) and you are not able to cover the distance with an RS485 cable.
Otherwise, the original Growatt ShineWiFi-S (serial) / ShineWiFi-X (USB), the customized [otti/Growatt_ShineWiFi-S](https://github.com/otti/Growatt_ShineWiFi-S) or [nygma2004/growatt2mqtt](https://github.com/nygma2004/growatt2mqtt) can be used.

<img src="https://user-images.githubusercontent.com/83612361/234670204-18edc9af-aa36-49aa-8b0f-26082e73d37f.png" alt="growatt2lorawan Architecture" height="600" />

:construction_worker: **Work in Progress!** :construction_worker: 

This is a "remix" of 
* [matthias-bs/BresserWeatherSensorTTN](https://github.com/matthias-bs/BresserWeatherSensorTTN) (or [arduino_lorawan_esp32_example](https://github.com/mcci-catena/arduino-lorawan/tree/master/examples/arduino_lorawan_esp32_example) if you will) - for the LoRaWAN part

and

* [nygma2004/growatt2mqtt](https://github.com/nygma2004/growatt2mqtt) - for the Growatt PV Inverter / Modbus part

## Hardware Requirements
* ESP32 (optionally with LiPo battery charger and battery)
* SX1276 (or compatible) LoRaWAN Radio Transceiver
* LoRaWAN Antenna
* optional: RS485 Transceiver - 3.3V compatible, half-duplex capable (e.g [Waveshare 4777](https://www.waveshare.com/wiki/RS485_Board_(3.3V)) module)
* optional: USB-to-TTL converter for Debugging (e.g. [AZ Delivery HW-598](https://www.az-delivery.de/en/products/hw-598-usb-auf-seriell-adapter-mit-cp2102-chip-und-kabel))

## Inverter Modbus Interface Options

1. USB Interface

    The inverter's USB port operates like a USB serial port (UART) interface at 115200 bits/s. If the length of a standard USB cable is sufficient to connect the ESP32 to the inverter (and there are no compatibility issues with the ESP32 board's USB serial interface), this is the easiest variant, because no extra hardware is needed.
    
    As pointed out in [otti/Growatt_ShineWiFi-S](https://github.com/otti/Growatt_ShineWiFi-S/blob/master/README.md), **only CH340-based USB-Serial converters are compatible** - converters with CP21XX and FTDI chips **do not work**!

2. COM Interface

    The inverter's COM port provides an RS485 interface at 9600 bits/s. An RS485 tranceiver is required to connect it to the ESP32.

### Modbus Interface Select Input

The desired interface is selected by pulling the GPIO pin `INTERFACE_SEL` (defined in `settings.h`) to 3.3V or GND, respectively:

| Level | Modbus Interface Selection |
| ----- | ---------------------------- |
| low (GND) | USB Interface |
| high (3.3V/open) | RS485 Interface |

## Power Supply

The ESP32 development board can be powered from the inverter's USB port **which only provides power if the inverter is active**.

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

| GPIO define | Description |
| ----------- | ----------- |
| INTERFACE_SEL | Modbus Interface Selection (USB/RS485) |

### Modbus via RS485 Interface Only:
| GPIO define   | Waveshare 4777 pin  |
| ------------- | ------------------- |
| MAX485_DE     | RSE                 |
| MAX485_RE_NEG | n.c.                |
| MAX485_RX     | RO                  |
| MAX485_TX     | DI                  |

### Debug Interface in case of using Modbus via USB Interface (optional):

USB-to-TTL converter, e.g. [AZ Delivery HW-598](https://www.az-delivery.de/en/products/hw-598-usb-auf-seriell-adapter-mit-cp2102-chip-und-kabel)

| GPIO define | USB to TTL Converter |
| ----------- | -------------------- | 
| DEBUG_TX    | RXD                  |
| DEBUG_RX    | TXD / n.c.           |

## MQTT Integration and IoT MQTT Panel Example

Arduino App: [IoT MQTT Panel](https://snrlab.in/iot/iot-mqtt-panel-user-guide)

<img src="https://user-images.githubusercontent.com/83612361/225129950-c323e0c7-a58b-4a3f-ba30-e0fd9adc1594.jpg" alt="Screenshot_20230314-130325_IoT_MQTT_Panel_Pro-1" width="400">

### Set up *IoT MQTT Panel* from configuration file

You can either edit the provided [JSON configuration file](https://github.com/matthias-bs/growatt2lorawan/blob/main/IoT_MQTT_Panel_Growatt2LoRaWAN.json) before importing it or import it as-is and make the required changes in *IoT MQTT Panel*. Don't forget to add the broker's certificate if using Secure MQTT! (in the App: *Connections -> Edit Connections: Certificate path*.)

**Editing [IoT_MQTT_Panel_Growatt2LoRaWAN.json](https://github.com/matthias-bs/growatt2lorawan/blob/main/IoT_MQTT_Panel_Growatt2LoRaWAN.json)**

Change *USERNAME* and *PASSWORD* as needed:
```
[...]
"username":"USERNAME","password":"PASSWORD"
[...]
```
