///////////////////////////////////////////////////////////////////////////////
// settings.h
//
// LoRaWAN Node for Growatt PV-Inverter Data Interface
//
// Modbus interface settings
//
// created: 03/2023
//
//
// MIT License
//
// Copyright (c) 2022 Matthias Prinke
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//
// History:
//
// 20230315 Created
// 20230403 Implemented uplink messages with different ports and cycle times
// 20230408 Modified debug output handling
//          Added Modbus interface option over USB serial
// 20230417 Added pin config for TTGO LoRa32 V2.1
// 20230420 Added pin config for
//          DFRobot FireBeetle ESP32 + FireBeetle Cover LoRa
// 20230421 Added pin config for 
//          Adafruit Feather ESP32 + LoRa Radio Featherwing
// 20231009 Renamed FIREBEETLE_COVER_LORA in FIREBEETLE_ESP32_COVER_LORA
//
///////////////////////////////////////////////////////////////////////////////

#if defined(ARDUINO_ESP32_DEV)
    // Both variants below are based on DFRobot FireBeetle ESP32; 
    // which uses the rather unspecific "ARDUINO_ESP32_DEV"
    #define FIREBEETLE_ESP32_COVER_LORA
    //#define LORAWAN_NODE
#endif

//#define useModulPower   1

//#define ENABLE_JSON
//#define GEN_PAYLOAD               // Generate payload for debugging (do not read Modbus)
//#define SLAVE_ID        1         // Default slave ID of Growatt
//#define SERIAL_RATE     115200    // Serial speed for status info
//#define MODBUS_RATE     9600      // Modbus speed of Growatt, do not change

#define UPDATE_MODBUS   2         // Modbus device is read every <n> seconds
#define MODBUS_RETRIES  5         // no. of modbus retries

#define STATUS_LED    LED_BUILTIN     // Status LED

#if defined(ARDUINO_TTGO_LoRa32_v21new)
    // https://github.com/espressif/arduino-esp32/blob/master/variants/ttgo-lora32-v21new/pins_arduino.h
    #define INTERFACE_SEL   13        // Modbus interface select (0: USB / 1: RS485)
    #define MAX485_DE       14        // D1, DE pin on the TTL to RS485 converter
    #define MAX485_RE_NEG   2         // D2, RE pin on the TTL to RS485 converter
    #define MAX485_RX       15        // D5, RO pin on the TTL to RS485 converter
    #define MAX485_TX       12        // D6, DI pin on the TTL to RS485 converter
    #define DEBUG_TX        00        // Serial port output to   USB converter
    #define DEBUG_RX        04        // Serial port input  from USB converter (n.c.)

#elif defined(FIREBEETLE_ESP32_COVER_LORA)
    // https://wiki.dfrobot.com/FireBeetle_ESP32_IOT_Microcontroller(V3.0)__Supports_Wi-Fi_&_Bluetooth__SKU__DFR0478
    // https://wiki.dfrobot.com/FireBeetle_Covers_LoRa_Radio_868MHz_SKU_TEL0125
    #define INTERFACE_SEL   17        // Modbus interface select (0: USB / 1: RS485)
    #define MAX485_DE       10        // D6, DE pin on the TTL to RS485 converter
    #define MAX485_RE_NEG   13        // D7, RE pin on the TTL to RS485 converter
    #define MAX485_RX       5         // D8, RO pin on the TTL to RS485 converter
    #define MAX485_TX       2         // D9, DI pin on the TTL to RS485 converter
    #define DEBUG_TX        4         // Serial port output to   USB converter
    #define DEBUG_RX        16        // Serial port input  from USB converter (n.c.)
        
#elif defined(LORAWAN_NODE)
    // LoRaWAN_Node board
    // https://github.com/matthias-bs/LoRaWAN_Node
    #define INTERFACE_SEL   13        // Modbus interface select (0: USB / 1: RS485)
    #define MAX485_DE       10        // D1, DE pin on the TTL to RS485 converter
    #define MAX485_RE_NEG   2         // D2, RE pin on the TTL to RS485 converter
    #define MAX485_RX       27        // D5, RO pin on the TTL to RS485 converter
    #define MAX485_TX       9         // D6, DI pin on the TTL to RS485 converter
    #define DEBUG_TX        5         // Serial port output to   USB converter (optional)
    #define DEBUG_RX        26        // Serial port input  from USB converter (n.c.)

#elif defined(ARDUINO_FEATHER_ESP32)
    // https://github.com/espressif/arduino-esp32/blob/master/variants/feather_esp32/pins_arduino.h
    #define INTERFACE_SEL   13        // Modbus interface select (0: USB / 1: RS485)
    #define MAX485_DE       4         // D1, DE pin on the TTL to RS485 converter
    #define MAX485_RE_NEG   15        // D2, RE pin on the TTL to RS485 converter
    #define MAX485_RX       22        // D5, RO pin on the TTL to RS485 converter
    #define MAX485_TX       23        // D6, DI pin on the TTL to RS485 converter
    #define DEBUG_TX        12        // Serial port output to   USB converter (optional)
    #define DEBUG_RX        21        // Serial port input  from USB converter (n.c.)

#else
    // for generic CI target ESP32:ESP32:ESP32
    #define INTERFACE_SEL   13        // Modbus interface select (0: USB / 1: RS485)
    #define MAX485_DE       10        // D1, DE pin on the TTL to RS485 converter
    #define MAX485_RE_NEG   2         // D2, RE pin on the TTL to RS485 converter
    #define MAX485_RX       27        // D5, RO pin on the TTL to RS485 converter
    #define MAX485_TX       9         // D6, DI pin on the TTL to RS485 converter
    #define DEBUG_TX        5         // Serial port output to   USB converter (optional)
    #define DEBUG_RX        26        // Serial port input  from USB converter (n.c.)

#endif
