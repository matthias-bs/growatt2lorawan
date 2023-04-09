///////////////////////////////////////////////////////////////////////////////
// payload.cpp
//
// Create data payload from simulated data or Modbus data
//
// created: 03/2023
//
//
// MIT License
//
// Copyright (c) 2023 Matthias Prinke
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
// 20230314 Created
// 20230409 Improved serial port reading reliability
//
// ToDo:
// -
//
///////////////////////////////////////////////////////////////////////////////

#include "payload.h"

growattIF growattInterface(MAX485_RE_NEG, MAX485_DE, MAX485_RX, MAX485_TX);
//bool holdingregisters = false;

void gen_payload(uint8_t port, LoraEncoder & encoder)
{
    const uint8_t     status = 1; // 0: waiting, 1: normal, 3: fault
    const uint8_t     faultcode = 0;
    const float       pv1voltage = 60.0; // V
    const float       pv1current = 2.0; // A
    const float       pv1power = 120.0; // W
    const float       outputpower = 111.1; // VA
    const float       gridvoltage = 233.3; // V
    const float       gridfrequency = 50.5; // Hz
    const float       energytoday = 1.11; // kWh
    const float       energytotal = 444.4; // kWh
    const float       totalworktime = 15998400; // seconds
    const float       tempinverter = 22.2; // °C
    const float       tempipm = 33.3; // °C
    const float       pv1energytoday = 1.11; // kWh 
    const float       pv1energytotal = 444.4; // kWh
    
    encoder.writeUint8(0x00); // Modbus status
    if (port == 1) {
        encoder.writeUint8(status);
        encoder.writeUint8(faultcode);
        encoder.writeRawFloat(pv1voltage);
        encoder.writeRawFloat(pv1current);
        encoder.writeRawFloat(pv1power);
        encoder.writeRawFloat(outputpower);
        encoder.writeRawFloat(gridvoltage);
        encoder.writeRawFloat(gridfrequency);
    } else {
        encoder.writeRawFloat(energytoday);      
        encoder.writeRawFloat(energytotal);
        encoder.writeRawFloat(totalworktime);
        encoder.writeTemperature(tempinverter);
        encoder.writeTemperature(tempipm);
        encoder.writeRawFloat(pv1energytoday);
        encoder.writeRawFloat(pv1energytotal);
            
    }
}

#if 0
void ReadInputRegisters() {
  uint8_t result;

  digitalWrite(STATUS_LED, 0);

  result = growattInterface.ReadInputRegisters(NULL);
  if (result == growattInterface.Success) {

#ifdef DEBUG_SERIAL
    Serial.println(result);
#endif

  } else if (result != growattInterface.Continue) {

    Serial.print(F("Error: "));
    String message = growattInterface.sendModbusError(result);
    Serial.println(message);
    delay(5);
  }
  digitalWrite(STATUS_LED, 1);
}

void ReadHoldingRegisters() {
  uint8_t result;

  digitalWrite(STATUS_LED, 0);
  result = growattInterface.ReadHoldingRegisters(NULL);
  if (result == growattInterface.Success)   {

#ifdef DEBUG_SERIAL
    Serial.println(json);
#endif

    // Set the flag to true not to read the holding registers again
    holdingregisters = true;

  } else if (result != growattInterface.Continue) {
    Serial.print(F("Error: "));
    String message = growattInterface.sendModbusError(result);
    Serial.println(message);

    delay(5);
  }
  digitalWrite(STATUS_LED, 1);
}
#endif

void get_payload(uint8_t port, LoraEncoder & encoder)
{
    uint8_t result;
    
    growattInterface.initGrowatt();
    delay(500);
    /*
    if (!holdingregisters) {
      // Read the holding registers
      ReadHoldingRegisters();
    } else {
      // Read the input registers
      ReadInputRegisters();
    }
    */
    
    int retries=0;
    do {
        result = growattInterface.ReadInputRegisters(NULL);
        log_d("ReadInputRegisters: 0x%02x", result);
        while (result == growattInterface.Continue) {
            delay(1000);
            result = growattInterface.ReadInputRegisters(NULL);
            log_d("ReadInputRegisters: 0x%02x", result);
        }
    
        encoder.writeUint8(result);
        if (result == growattInterface.Success) {
            if (port == 1) {
                encoder.writeUint8(growattInterface.modbusdata.status);
                encoder.writeUint8(growattInterface.modbusdata.faultcode);
                encoder.writeRawFloat(growattInterface.modbusdata.pv1voltage);
                encoder.writeRawFloat(growattInterface.modbusdata.pv1current);
                encoder.writeRawFloat(growattInterface.modbusdata.pv1power);
                encoder.writeRawFloat(growattInterface.modbusdata.outputpower);
                encoder.writeRawFloat(growattInterface.modbusdata.gridvoltage);
                encoder.writeRawFloat(growattInterface.modbusdata.gridfrequency);
                
            } else {
                encoder.writeRawFloat(growattInterface.modbusdata.energytoday);      
                encoder.writeRawFloat(growattInterface.modbusdata.energytotal);
                encoder.writeRawFloat(growattInterface.modbusdata.totalworktime);
                encoder.writeTemperature(growattInterface.modbusdata.tempinverter);
                encoder.writeTemperature(growattInterface.modbusdata.tempipm);
                encoder.writeRawFloat(growattInterface.modbusdata.pv1energytoday);
                encoder.writeRawFloat(growattInterface.modbusdata.pv1energytotal);
                
            }

        } else if (result != growattInterface.Continue) {
            String message = growattInterface.sendModbusError(result);
            log_e("Error: %s", message.c_str());
            delay(1000);
        }
    } while ((result != growattInterface.Success) && (++retries < MODBUS_RETRIES));
}
