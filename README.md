# growatt2lorawan
LoRaWAN Node for Growatt PV-Inverter Modbus Data Interface

Hardware Requirements:
* ESP32
* SX1276 (or compatible) LoRaWAN Radio Transceiver
* LoRaWAN Antenna
* RS485 Transceiver - 3.3V compatible, half-duplex capable (e.g [Waveshare 4777](https://www.waveshare.com/wiki/RS485_Board_(3.3V)) module)

:construction_worker: **Work in Progress!** :construction_worker: 

This is a "remix" of 
* [matthias-bs/BresserWeatherSensorTTN](https://github.com/matthias-bs/BresserWeatherSensorTTN) (or [arduino_lorawan_esp32_example](https://github.com/mcci-catena/arduino-lorawan/tree/master/examples/arduino_lorawan_esp32_example) if you will) - for the LoRaWAN part

and

* [nygma2004/growatt2mqtt](https://github.com/nygma2004/growatt2mqtt) - for the Growatt PV Inverter / Modbus part
