// Source: 
// https://github.com/nygma2004/growatt2mqtt
// V1.2 20230311
//
// 20230313 matthias-bs Replaced SoftwareSerial by HardwareSerial
// 20230408 Added different Modbus data rates for RS485 and USB
#ifndef GROWATTINTERFACE_H
#define GROWATTINTERFACE_H

#include "Arduino.h"
#include <ModbusMaster.h>            // Modbus master library for ESP8266 by Doc Walker (https://github.com/4-20ma/ModbusMaster)
#define SLAVE_ID                 1   // Default slave ID of Growatt
#define MODBUS_RATE_RS485     9600   // Growatt Modbus data rate over RS485
#define MODBUS_RATE_USB     115200   // Growatt Modbus data rate over USB 

class growattIF {

  private:
    ModbusMaster growattInterface;
    //SoftwareSerial *serial;
    HardwareSerial *serial;
    void preTransmission();
    void postTransmission();
    int PinMAX485_RE_NEG;
    int PinMAX485_DE;
    int PinMAX485_RX;
    int PinMAX485_TX;
    int setcounter = 0;
    int overflow;

  public:
    struct modbus_input_registers
    {
      int status;
      float solarpower, pv1voltage, pv1current, pv1power, pv2voltage, pv2current, pv2power, outputpower, gridfrequency, gridvoltage;
      float energytoday, energytotal, totalworktime, pv1energytoday, pv1energytotal, pv2energytoday, pv2energytotal, opfullpower;
      float tempinverter, tempipm, tempboost;
      int ipf, realoppercent, deratingmode, faultcode, faultbitcode, warningbitcode;
    };
    struct modbus_input_registers modbusdata;

    struct modbus_holding_registers
    {
      int enable, safetyfuncen, maxoutputactivepp, maxoutputreactivepp, modul;
      float  maxpower, voltnormal, startvoltage, gridvoltlowlimit, gridvolthighlimit, gridfreqlowlimit, gridfreqhighlimit, gridvoltlowconnlimit, gridvolthighconnlimit, gridfreqlowconnlimit, gridfreqhighconnlimit;
      char firmware[6], controlfirmware[6];
      char serial[10];
    };

    struct modbus_holding_registers modbussettings;

    growattIF(int _PinMAX485_RE_NEG, int _PinMAX485_DE, int _PinMAX485_RX, int _PinMAX485_TX);
    void initGrowatt();
    uint8_t writeRegister(uint16_t reg, uint16_t message);
    uint16_t readRegister(uint16_t reg);
    uint8_t ReadInputRegisters(char* json);
    uint8_t ReadHoldingRegisters(char* json);
    String sendModbusError(uint8_t result);

    // Error codes
    static const uint8_t Success    = 0x00;
    static const uint8_t Continue   = 0xFF;

    // Growatt Holding registers
    static const uint8_t regOnOff           = 0;
    static const uint8_t regMaxOutputActive = 3;
    static const uint8_t regStartVoltage    = 17;
    static const uint8_t regModulPower      = 121;
};

#endif
