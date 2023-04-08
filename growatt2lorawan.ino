///////////////////////////////////////////////////////////////////////////////
// growatt2lorawan.ino
// 
// LoRaWAN Node for Growatt PV-Inverter Data Interface
//
// https://github.com/mcci-catena/arduino-lorawan/tree/master/examples/arduino_lorawan_esp32_example
// - implements power saving by using the ESP32 deep sleep mode
// - implements fast re-joining after sleep by storing network session data
//   in the ESP32 RTC RAM
// - LoRa_Serialization is used for encoding various data types into bytes
// - internal Real-Time Clock (RTC) set from LoRaWAN network time (optional)
//
//
// Based on:
// ---------
// MCCI LoRaWAN LMIC library by Thomas Telkamp and Matthijs Kooijman / Terry Moore, MCCI
// (https://github.com/mcci-catena/arduino-lmic)

// MCCI Arduino LoRaWAN Library by Terry Moore, MCCI 
// (https://github.com/mcci-catena/arduino-lorawan)
//
//
// Library dependencies (tested versions):
// ---------------------------------------
// MCCI Arduino Development Kit ADK     0.2.2
// MCCI LoRaWAN LMIC library            4.1.1
// MCCI Arduino LoRaWAN Library         0.9.2
// LoRa_Serialization                   3.2.1
// ESP32Time                            2.0.0
//
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
//
//
// Notes:
// - After a successful transmission, the controller can go into deep sleep
//   (option SLEEP_EN)
// - Sleep time is defined in SLEEP_INTERVAL
// - If joining the network or transmitting uplink data fails,
//   the controller can go into deep sleep
//   (option FORCE_SLEEP)
// - Timeout is defined in SLEEP_TIMEOUT_INITIAL and SLEEP_TIMEOUT_JOINED
// - The ESP32's RTC RAM is used to store information about the LoRaWAN 
//   network session; this speeds up the connection after a restart
//   significantly
// - To enable Network Time Requests:
//   #define LMIC_ENABLE_DeviceTimeReq 1
// - settimeofday()/gettimeofday() must be used to access the ESP32's RTC time
// - Arduino ESP32 package has built-in time zone handling, see 
//   https://github.com/SensorsIot/NTP-time-for-ESP8266-and-ESP32/blob/master/NTP_Example/NTP_Example.ino
//
///////////////////////////////////////////////////////////////////////////////

//--- Select LoRaWAN Network ---
// The Things Network
#define ARDUINO_LMIC_CFG_NETWORK_TTN 1

// Helium Network
// see mcci-cathena/arduino-lorawan issue #185 "Add Helium EU868 support"
// (https://github.com/mcci-catena/arduino-lorawan/issues/185)
#define ARDUINO_LMIC_CFG_NETWORK_GENERIC 0

// (Add other networks here)


#include <Arduino_LoRaWAN_network.h>
#include <Arduino_LoRaWAN_EventLog.h>
#include <arduino_lmic.h>
#include "src/settings.h"
#include "src/payload.h"

// NOTE: Add #define LMIC_ENABLE_DeviceTimeReq 1
//        in ~/Arduino/libraries/MCCI_LoRaWAN_LMIC_library/project_config/lmic_project_config.h
#if (not(LMIC_ENABLE_DeviceTimeReq))
    #warning "LMIC_ENABLE_DeviceTimeReq is not set - will not be able to retrieve network time!"
#endif

//-----------------------------------------------------------------------------
//
// User Configuration
//

// Enable debug mode (debug messages via serial port)
#define _DEBUG_MODE_

// Enable sleep mode - sleep after successful transmission to TTN (recommended!)
#define SLEEP_EN

// Enable setting RTC from LoRaWAN network time
#define GET_NETWORKTIME

#if defined(GET_NETWORKTIME)
    // Enter your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)
    const char* TZ_INFO    = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";

    // RTC to network time sync interval (in minutes)
    #define CLOCK_SYNC_INTERVAL 24 * 60
#endif

// Number of uplink ports
#define NUM_PORTS 2

// Uplink period multipliers
#define UPLINK_PERIOD_MULTIPLIERS {1, 5}
typedef struct {
    int port;
    int mult;
} Schedule;

const Schedule UplinkSchedule[NUM_PORTS] = {
  // {port, mult}
  {1, 1},
  {2, 2}
};

// If SLEEP_EN is defined, MCU will sleep for SLEEP_INTERVAL seconds after succesful transmission
#define SLEEP_INTERVAL 360

// Force deep sleep after a certain time, even if transmission was not completed
//#define FORCE_SLEEP

// During initialization (not joined), force deep sleep after SLEEP_TIMEOUT_INITIAL (if enabled)
#define SLEEP_TIMEOUT_INITIAL 1800

// If already joined, force deep sleep after SLEEP_TIMEOUT_JOINED seconds (if enabled)
#define SLEEP_TIMEOUT_JOINED 600

//-----------------------------------------------------------------------------

#if defined(GET_NETWORKTIME)
  #include <ESP32Time.h>
#endif

// LoRa_Serialization
#include <LoraMessage.h>

// Pin mappings for some common ESP32 LoRaWAN boards.
// The ARDUINO_* defines are set by selecting the appropriate board (and borad variant, if applicable) in the Arduino IDE.
// The default SPI port of the specific board will be used.
#if defined(ARDUINO_TTGO_LoRa32_V1)
    // https://github.com/espressif/arduino-esp32/blob/master/variants/ttgo-lora32-v1/pins_arduino.h
    // http://www.lilygo.cn/prod_view.aspx?TypeId=50003&Id=1130&FId=t3:50003:3
    // https://github.com/Xinyuan-LilyGo/TTGO-LoRa-Series
    // https://github.com/LilyGO/TTGO-LORA32/blob/master/schematic1in6.pdf
    #define PIN_LMIC_NSS      LORA_CS
    #define PIN_LMIC_RST      LORA_RST
    #define PIN_LMIC_DIO0     LORA_IRQ
    #define PIN_LMIC_DIO1     33
    #define PIN_LMIC_DIO2     cMyLoRaWAN::lmic_pinmap::LMIC_UNUSED_PIN

#elif defined(ARDUINO_TTGO_LoRa32_V2)
    // https://github.com/espressif/arduino-esp32/blob/master/variants/ttgo-lora32-v2/pins_arduino.h
    #define PIN_LMIC_NSS      LORA_CS
    #define PIN_LMIC_RST      LORA_RST
    #define PIN_LMIC_DIO0     LORA_IRQ
    #define PIN_LMIC_DIO1     33
    #define PIN_LMIC_DIO2     cMyLoRaWAN::lmic_pinmap::LMIC_UNUSED_PIN
    #pragma message("LoRa DIO1 must be wired to GPIO33 manually!")

#elif defined(ARDUINO_TTGO_LoRa32_v21new)
    // https://github.com/espressif/arduino-esp32/blob/master/variants/ttgo-lora32-v21new/pins_arduino.h
    #define PIN_LMIC_NSS      LORA_CS
    #define PIN_LMIC_RST      LORA_RST
    #define PIN_LMIC_DIO0     LORA_IRQ
    #define PIN_LMIC_DIO1     LORA_D1
    #define PIN_LMIC_DIO2     LORA_D2

#elif defined(ARDUINO_heltec_wireless_stick)
    // https://github.com/espressif/arduino-esp32/blob/master/variants/heltec_wireless_stick/pins_arduino.h
    #define PIN_LMIC_NSS      SS
    #define PIN_LMIC_RST      RST_LoRa
    #define PIN_LMIC_DIO0     DIO0
    #define PIN_LMIC_DIO1     DIO1
    #define PIN_LMIC_DIO2     DIO2

#elif defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
    #define PIN_LMIC_NSS      6
    #define PIN_LMIC_RST      9
    #define PIN_LMIC_DIO0     5
    #define PIN_LMIC_DIO1     11
    #define PIN_LMIC_DIO2     cMyLoRaWAN::lmic_pinmap::LMIC_UNUSED_PIN
    #pragma message("ARDUINO_ADAFRUIT_FEATHER_ESP32S2 defined; assuming RFM95W FeatherWing will be used")
    #pragma message("Required wiring: E to IRQ, D to CS, C to RST, A to DI01")
    #pragma message("BLE is not available!")

#elif defined(ARDUINO_FEATHER_ESP32)
    #define PIN_LMIC_NSS      14
    #define PIN_LMIC_RST      27
    #define PIN_LMIC_DIO0     32
    #define PIN_LMIC_DIO1     33
    #define PIN_LMIC_DIO2     cMyLoRaWAN::lmic_pinmap::LMIC_UNUSED_PIN
    #pragma message("ARDUINO_ADAFRUIT_FEATHER_ESP32 defined; assuming RFM95W FeatherWing will be used")
    #pragma message("Required wiring: A to RST, B to DIO1, D to DIO0, E to CS")
    
#else
    // LoRaWAN_Node board
    // https://github.com/matthias-bs/LoRaWAN_Node
    #define PIN_LMIC_NSS      14
    #define PIN_LMIC_RST      12
    #define PIN_LMIC_DIO0     4
    #define PIN_LMIC_DIO1     16
    #define PIN_LMIC_DIO2     17

#endif

// Uplink message payload size
// The maximum allowed for all data rates is 51 bytes.
const uint8_t PAYLOAD_SIZE = 51;

// RTC Memory Handling
#define MAGIC1 (('m' << 24) | ('g' < 16) | ('c' << 8) | '1')
#define MAGIC2 (('m' << 24) | ('g' < 16) | ('c' << 8) | '2')
#define EXTRA_INFO_MEM_SIZE 64

// Debug printing
// To enable debug mode (debug messages via serial port):
// Arduino IDE: Tools->Core Debug Level: "Debug|Verbose"
// or
// set CORE_DEBUG_LEVEL in BresserWeatherSensorTTNCfg.h
#define DEBUG_PORT Serial2
#define DEBUG_PRINTF(...) { log_d(__VA_ARGS__); }
#define DEBUG_PRINTF_TS(...) { log_d(__VA_ARGS__); }

#if defined(GET_NETWORKTIME)
    void printDateTime(void);
#endif

/****************************************************************************\
|
|	The LoRaWAN object
|
\****************************************************************************/

class cMyLoRaWAN : public Arduino_LoRaWAN_network {
public:
    cMyLoRaWAN() {};
    using Super = Arduino_LoRaWAN_network;
    void setup();
    
    #if defined(GET_NETWORKTIME)
        /*!
         * \fn requestNetworkTime
         * 
         * \brief Wrapper function for LMIC_requestNetworkTime()
         */
        void requestNetworkTime(void);
    #endif

/*
// Attempt to override GetPinmap_ThisBoard() from MCCI_LoRaWAN_LMIC_library
// to get rid of "#pragma message: Board not supported -- use an explicit pinmap"
private:
    const Arduino_LMIC::HalPinmap_t* Arduino_LMIC::GetPinmap_ThisBoard(void) {
      return nullptr;
    };
*/

protected:
    // you'll need to provide implementation for this.
    virtual bool GetOtaaProvisioningInfo(Arduino_LoRaWAN::OtaaProvisioningInfo*) override;

    // NetTxComplete() activates deep sleep mode (if enabled)
    virtual void NetTxComplete(void) override;

    // NetJoin() changes <sleepTimeout>
    virtual void NetJoin(void) override;
    
    // Used to store/load data to/from persistent (at least during deep sleep) memory 
    virtual void NetSaveSessionInfo(const SessionInfo &Info, const uint8_t *pExtraInfo, size_t nExtraInfo) override;
    virtual void NetSaveSessionState(const SessionState &State) override;
    virtual bool NetGetSessionState(SessionState &State) override;
    virtual bool GetAbpProvisioningInfo(AbpProvisioningInfo *pAbpInfo) override;  
};


/****************************************************************************\
|
|	The sensor object
|
\****************************************************************************/

class cSensor {
public:
    /// \brief the constructor. Deliberately does very little.
    cSensor() {};

    // Sensor data function stubs
    float    getTemperature(void);
    uint16_t getVoltageBattery(void);
    uint16_t getVoltageSupply(void);
    bool     isUplinkPending(void) {
        if (this->m_fBusy) {
            log_d("Busy");
            return true;          
        }   
        for (int idx=0; idx<NUM_PORTS; idx++) {
            log_d("m_fUplinkRequest[%d]=%d", idx, m_fUplinkRequest[idx]);
            if (m_fUplinkRequest[idx])
                return true;
        }
        return false;
     };
    
    void uplinkRequest(void) {
        m_fUplinkRequest[0] = true;
    };
    ///
    /// \brief set up the sensor object
    ///
    /// \param uplinkPeriodMs optional uplink interval. If not specified,
    ///         transmit every six minutes.
    ///
    void setup(std::uint32_t uplinkPeriodMs = 6 * 60 * 1000);

    ///
    /// \brief update sensor loop.
    ///
    /// \details
    ///     This should be called from the global loop(); it periodically
    ///     gathers and transmits sensor data.
    ///
    void loop();

    // Example sensor status flags
    bool data_ok;               //<! sensor data validation
    bool battery_ok;            //<! sensor battery status
    
    // Example sensor data
    float    temperature_deg_c; //<! outdoor air temperature in °C
    uint16_t battery_voltage_v; //<! battery voltage
    uint16_t supply_voltage_v;  //<! supply voltage
    
private:
    void doUplink(int port);

    bool m_fUplinkRequest[NUM_PORTS];             // set true when uplink is requested
    bool m_fBusy;                                 // set true while sending an uplink
    std::uint32_t m_uplinkPeriodMs;               // uplink period in milliseconds
    std::uint8_t const m_uplinkPeriodMult[NUM_PORTS] = UPLINK_PERIOD_MULTIPLIERS;  // uplink period multiplier per port 
    std::uint32_t m_tReference[NUM_PORTS];        // time of last uplink
};

/****************************************************************************\
|
|	Globals
|
\****************************************************************************/

// the global LoRaWAN instance.
cMyLoRaWAN myLoRaWAN {};

// the global sensor instance
cSensor mySensor {};

class cMyEventLog: public Arduino_LoRaWAN::cEventLog {
  public:
    void printTx(uint32_t ts, std::uint8_t channel, std::uint8_t rps) const
    {
      if (CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG) {
        char buf[64];
        *buf = '\0';

        sprintf(buf, "TX @%u ms: ch=%d rps=0x%02X (%s %s %s %s IH=%u)", 
          ts,
          channel, 
          rps, 
          getSfName(rps),
          getBwName(rps),
          getCrName(rps),
          getCrcName(rps),
          unsigned(getIh(rps))
        );
        DEBUG_PRINTF_TS("%s", buf);

      }
    }
    
    void printCh(HardwareSerial & serial, std::uint8_t channel) const
    {
        serial.print(" ch=");
        serial.print(std::uint32_t(channel));
    }

    void printRps(HardwareSerial & serial, std::uint8_t rps) const
    {
      serial.print(F(" rps=0x")); printHex2(serial, rps);
      serial.print(F(" (")); serial.print(getSfName(rps));
      printSpace(serial); serial.print(getBwName(rps));
      printSpace(serial); serial.print(getCrName(rps));
      printSpace(serial); serial.print(getCrcName(rps));
      serial.print(F(" IH=")); serial.print(unsigned(getIh(rps)));
      serial.print(')');
    }
    
    void printHex2(HardwareSerial & serial, unsigned v) const
    {
      v &= 0xff;
      if (v < 16)
        serial.print('0');
      serial.print(v, HEX);
    }

    void printSpace(HardwareSerial & serial) const
    {
      serial.print(' ');
    }
};

// the global event log instance
cMyEventLog myEventLog;

// The pin map. This form is convenient if the LMIC library
// doesn't support your board and you don't want to add the
// configuration to the library (perhaps you're just testing).
// This pinmap matches the FeatherM0 LoRa. See the arduino-lmic
// docs for more info on how to set this up.
const cMyLoRaWAN::lmic_pinmap myPinMap = {
     .nss = PIN_LMIC_NSS,
     .rxtx = cMyLoRaWAN::lmic_pinmap::LMIC_UNUSED_PIN,
     .rst = PIN_LMIC_RST,
     .dio = { PIN_LMIC_DIO0, PIN_LMIC_DIO1, PIN_LMIC_DIO2 },
     .rxtx_rx_active = 0,
     .rssi_cal = 0,
     .spi_freq = 8000000,
};


// The following variables are stored in the ESP32's RTC RAM -
// their value is retained after a Sleep Reset.
RTC_DATA_ATTR uint32_t                        magicFlag1;
RTC_DATA_ATTR Arduino_LoRaWAN::SessionState   rtcSavedSessionState;
RTC_DATA_ATTR uint32_t                        magicFlag2;
RTC_DATA_ATTR Arduino_LoRaWAN::SessionInfo    rtcSavedSessionInfo;
RTC_DATA_ATTR size_t                          rtcSavedNExtraInfo;
RTC_DATA_ATTR uint8_t                         rtcSavedExtraInfo[EXTRA_INFO_MEM_SIZE];
RTC_DATA_ATTR bool                            runtimeExpired = 0;
RTC_DATA_ATTR uint32_t                        tReference[NUM_PORTS] = { 0 };        // time of last uplink

#if defined(GET_NETWORKTIME)
    RTC_DATA_ATTR time_t                      rtcLastClockSync = 0;     //!< timestamp of last RTC synchonization to network time
#endif

// Uplink payload buffer
static uint8_t loraData[PAYLOAD_SIZE];

// Force sleep mode after <sleepTimeout> has been reached (if FORCE_SLEEP is defined) 
ostime_t sleepTimeout;

/// RTC sync request flag - set (if due) in setup() / cleared in UserRequestNetworkTimeCb()
bool rtcSyncReq = false;

#if defined(GET_NETWORKTIME)
    /// Seconds since the UTC epoch
    uint32_t userUTCTime;

    /// Real time clock
    ESP32Time rtc;
#endif

bool modbusRS485; // Modbus interface select: 0 - USB / 1 - RS485

/****************************************************************************\
|
|	Provisioning info for LoRaWAN OTAA
|
\****************************************************************************/

//
// For normal use, we require that you edit the sketch to replace FILLMEIN_x
// with values assigned by the your network. However, for regression tests,
// we want to be able to compile these scripts. The regression tests define
// COMPILE_REGRESSION_TEST, and in that case we define FILLMEIN_x to non-
// working but innocuous values.
//
// #define COMPILE_REGRESSION_TEST 1

#ifdef COMPILE_REGRESSION_TEST
# define FILLMEIN_8     1, 0, 0, 0, 0, 0, 0, 0
# define FILLMEIN_16    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2
#else
# warning "You must replace the values marked FILLMEIN with real values from the TTN control panel!"
# define FILLMEIN_8 (#dont edit this, edit the lines that use FILLMEIN_8)
# define FILLMEIN_16 (#dont edit this, edit the lines that use FILLMEIN_16)
#endif

// APPEUI, DEVEUI and APPKEY
#include "secrets.h"

#ifndef SECRETS
    #define SECRETS
    
    // The following constants should be copied to secrets.h and configured appropriately
    // according to the settings from TTN Console
    
    // deveui, little-endian (lsb first)
    static const std::uint8_t deveui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    
    // appeui, little-endian (lsb first)
    static const std::uint8_t appeui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    
    // appkey: just a string of bytes, sometimes referred to as "big endian".
    static const std::uint8_t appkey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#endif

    
/****************************************************************************\
|
|	setup()
|
\****************************************************************************/

void setup() {

    pinMode(INTERFACE_SEL, INPUT);
    modbusRS485 = digitalRead(INTERFACE_SEL);
    

    // set baud rate
    if (modbusRS485) {
        Serial.begin(115200);
        log_d("Modbus interface: RS485");
    } else {
        Serial.setDebugOutput(false);
        DEBUG_PORT.begin(115200, SERIAL_8N1, DEBUG_RX, DEBUG_TX);
        DEBUG_PORT.setDebugOutput(true);
        log_d("Modbus interface: USB");
    }
    
    // wait for DEBUG_PORT to be ready
    //while (! DEBUG_PORT)
    //    yield();
    delay(500);

    sleepTimeout = sec2osticks(SLEEP_TIMEOUT_INITIAL);

    DEBUG_PRINTF_TS("setup()");

    #if defined(GET_NETWORKTIME)
        // Set time zone
        setenv("TZ", TZ_INFO, 1);
        printDateTime();
    
        // Check if clock was never synchronized or sync interval has expired 
        if ((rtcLastClockSync == 0) || ((rtc.getLocalEpoch() - rtcLastClockSync) > (CLOCK_SYNC_INTERVAL * 60))) {
            DEBUG_PRINTF("RTC sync required");
            rtcSyncReq = true;
        }
    #endif

    // set up the log; do this first.
    myEventLog.setup();
    DEBUG_PRINTF("myEventlog.setup() - done");

    // set up the sensors.
    mySensor.setup();
    DEBUG_PRINTF("mySensor.setup() - done");

    // set up lorawan.
    myLoRaWAN.setup();
    DEBUG_PRINTF("myLoRaWAN.setup() - done");

    mySensor.uplinkRequest();
}

/****************************************************************************\
|
|	loop()
|
\****************************************************************************/

void loop() {
    // the order of these is arbitrary, but you must poll them all.
    myLoRaWAN.loop();
    mySensor.loop();
    myEventLog.loop();

    #ifdef FORCE_SLEEP
        if ((os_getTime() > sleepTimeout) & !rtcSyncReq) {
            DEBUG_PRINTF_TS("Sleep timer expired!");
            DEBUG_PRINTF("Shutdown()");
            runtimeExpired = true;
            myLoRaWAN.Shutdown();
            magicFlag1 = 0;
            magicFlag2 = 0;
            ESP.deepSleep(SLEEP_INTERVAL * 1000000);
        }
    #endif
}

/****************************************************************************\
|
|	LoRaWAN methods
|
\****************************************************************************/

// our setup routine does the class setup and then registers an event handler so
// we can see some interesting things
void
cMyLoRaWAN::setup() {
    // simply call begin() w/o parameters, and the LMIC's built-in
    // configuration for this board will be used.
    this->Super::begin(myPinMap);

//    LMIC_selectSubBand(0);
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

    this->RegisterListener(
        // use a lambda so we're "inside" the cMyLoRaWAN from public/private perspective
        [](void *pClientInfo, uint32_t event) -> void {
            auto const pThis = (cMyLoRaWAN *)pClientInfo;

            // for tx start, we quickly capture the channel and the RPS
            if (event == EV_TXSTART) {
                // use another lambda to make log prints easy
                myEventLog.logEvent(
                    (void *) pThis,
                    LMIC.txChnl,
                    LMIC.rps,
                    0,
                    // the print-out function
                    [](cEventLog::EventNode_t const *pEvent) -> void {
                        myEventLog.printTx(osticks2ms(pEvent->getTime()), std::uint8_t(pEvent->getData(0)), rps_t(pEvent->getData(1)));
                    }
                );
            }
            // else if (event == some other), record with print-out function
            else {
                // do nothing.
            }
        },
        (void *) this   // in case we need it.
        );
}

// this method is called when the LMIC needs OTAA info.
// return false to indicate "no provisioning", otherwise
// fill in the data and return true.
bool
cMyLoRaWAN::GetOtaaProvisioningInfo(
    OtaaProvisioningInfo *pInfo
    ) {
    // these are the same constants used in the LMIC compliance test script; eases testing
    // with the RedwoodComm RWC5020B/RWC5020M testers.

    // initialize info
    memcpy(pInfo->DevEUI, deveui, sizeof(pInfo->DevEUI));
    memcpy(pInfo->AppEUI, appeui, sizeof(pInfo->AppEUI));
    memcpy(pInfo->AppKey, appkey, sizeof(pInfo->AppKey));

    return true;
}

// This method is called after the node has joined the network.
void
cMyLoRaWAN::NetJoin(
    void) {
    DEBUG_PRINTF_TS("NetJoin()\n");
    sleepTimeout = os_getTime() + sec2osticks(SLEEP_TIMEOUT_JOINED);
}

// This method is called after transmission has been completed.
// If enabled, the controller goes into deep sleep mode now.
void
cMyLoRaWAN::NetTxComplete(void) {
    DEBUG_PRINTF_TS("NetTxComplete()");
    #ifdef SLEEP_EN
        if (!mySensor.isUplinkPending()) {
            DEBUG_PRINTF("Shutdown()");
            myLoRaWAN.Shutdown();
            ESP.deepSleep(SLEEP_INTERVAL * 1000000);
        }
    #endif
}

// Print session info for debugging
void printSessionInfo(const cMyLoRaWAN::SessionInfo &Info)
{
    log_v("Tag:\t\t%d", Info.V1.Tag);
    log_v("Size:\t\t%d", Info.V1.Size);
    log_v("Rsv2:\t\t%d", Info.V1.Rsv2);
    log_v("Rsv3:\t\t%d", Info.V1.Rsv3);
    log_v("NetID:\t\t0x%08X", Info.V1.NetID);
    log_v("DevAddr:\t0x%08X", Info.V1.DevAddr);
    if (CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG) {
        char buf[64];
        *buf = '\0';
        for (int i=0; i<15;i++) {
            sprintf(&buf[strlen(buf)], "%02X ", Info.V1.NwkSKey[i]);
        }
        log_v("NwkSKey:\t\t%s", buf);
    }
    if (CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG) {
        char buf[64];
        *buf = '\0';
        for (int i=0; i<15;i++) {
            sprintf(&buf[strlen(buf)], "%02X ", Info.V1.AppSKey[i]);  
        }
        log_v("AppSKey:\t\t%s", buf);
    }    
}

// Print session state for debugging
void printSessionState(const cMyLoRaWAN::SessionState &State)
{
    log_v("Tag:\t\t%d", State.V1.Tag);
    log_v("Size:\t\t%d", State.V1.Size);
    log_v("Region:\t\t%d", State.V1.Region);
    log_v("LinkDR:\t\t%d", State.V1.LinkDR);
    log_v("FCntUp:\t\t%d", State.V1.FCntUp);
    log_v("FCntDown:\t\t%d", State.V1.FCntDown);
    log_v("gpsTime:\t\t%d", State.V1.gpsTime);
    log_v("globalAvail:\t%d", State.V1.globalAvail);
    log_v("Rx2Frequency:\t%d", State.V1.Rx2Frequency);
    log_v("PingFrequency:\t%d", State.V1.PingFrequency);
    log_v("Country:\t\t%d", State.V1.Country);
    log_v("LinkIntegrity:\t%d", State.V1.LinkIntegrity);
    // There is more in it...
}

// Save Info to ESP32's RTC RAM
// if not possible, just do nothing and make sure you return false
// from NetGetSessionState().
void
cMyLoRaWAN::NetSaveSessionInfo(
    const SessionInfo &Info,
    const uint8_t *pExtraInfo,
    size_t nExtraInfo
    ) {
    if (nExtraInfo > EXTRA_INFO_MEM_SIZE)
        return;
    rtcSavedSessionInfo = Info;
    rtcSavedNExtraInfo = nExtraInfo;
    memcpy(rtcSavedExtraInfo, pExtraInfo, nExtraInfo);
    magicFlag2 = MAGIC2;
    DEBUG_PRINTF_TS("NetSaveSessionInfo()");
    #ifdef _DEBUG_MODE_
        printSessionInfo(Info);
    #endif
}

// Save State in RTC RAM. Note that it's often the same;
// often only the frame counters change.
// [If not possible, just do nothing and make sure you return false
// from NetGetSessionState().]
void
cMyLoRaWAN::NetSaveSessionState(const SessionState &State) {
    rtcSavedSessionState = State;
    magicFlag1 = MAGIC1;
    DEBUG_PRINTF_TS("NetSaveSessionState()");
    #ifdef _DEBUG_MODE_
        printSessionState(State);
    #endif
}

// Either fetch SessionState from somewhere and return true or...
// return false, which forces a re-join.
bool
cMyLoRaWAN::NetGetSessionState(SessionState &State) {
    if (magicFlag1 == MAGIC1) {
        State = rtcSavedSessionState;
        DEBUG_PRINTF_TS("NetGetSessionState() - o.k.");
        #ifdef _DEBUG_MODE_
            printSessionState(State);
        #endif
        return true;
    } else {
        DEBUG_PRINTF_TS("NetGetSessionState() - failed");
        return false;
    }
}

// Get APB provisioning info - this is also used in OTAA after a succesful join.
// If it can be provided in OTAA mode after a restart, no re-join is needed.
bool
cMyLoRaWAN::GetAbpProvisioningInfo(AbpProvisioningInfo *pAbpInfo) {
    SessionState state;

    // ApbInfo:
    // --------
    // uint8_t         NwkSKey[16];
    // uint8_t         AppSKey[16];
    // uint32_t        DevAddr;
    // uint32_t        NetID;
    // uint32_t        FCntUp;
    // uint32_t        FCntDown;
    
    if ((magicFlag1 != MAGIC1) || (magicFlag2 != MAGIC2)) {
         return false;
    }
    DEBUG_PRINTF_TS("GetAbpProvisioningInfo()");

    pAbpInfo->DevAddr = rtcSavedSessionInfo.V2.DevAddr;
    pAbpInfo->NetID   = rtcSavedSessionInfo.V2.NetID;
    memcpy(pAbpInfo->NwkSKey, rtcSavedSessionInfo.V2.NwkSKey, 16);
    memcpy(pAbpInfo->AppSKey, rtcSavedSessionInfo.V2.AppSKey, 16);
    NetGetSessionState(state);
    pAbpInfo->FCntUp   = state.V1.FCntUp;
    pAbpInfo->FCntDown = state.V1.FCntDown;

    if (CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG) {
        char buf[64];
        
        *buf = '\0';
        for (int i=0; i<15;i++) {
          sprintf(&buf[strlen(buf)], "%02X ", pAbpInfo->NwkSKey[i]);  
        }
        log_v("NwkSKey:\t%s", buf);
        
        *buf = '\0';
        for (int i=0; i<15;i++) {
          sprintf(&buf[strlen(buf)], "%02X ", pAbpInfo->AppSKey[i]);  
        }
        log_v("AppSKey:\t%s", buf);
        log_v("FCntUp:\t%d", state.V1.FCntUp);
    }
    return true;
}

#if defined(GET_NETWORKTIME)
    /// Print date and time (i.e. local time)
    void printDateTime(void) {
        struct tm timeinfo;
        char tbuf[26];
        
        time_t tnow = rtc.getLocalEpoch();
        localtime_r(&tnow, &timeinfo);
        strftime(tbuf, 25, "%Y-%m-%d %H:%M:%S", &timeinfo);
        DEBUG_PRINTF("%s", tbuf);
    }

    /**
      * \fn UserRequestNetworkTimeCb
      * 
      * \brief Callback function for setting RTC from LoRaWAN network time
      * 
      * \param pVoidUserUTCTime user supplied buffer for UTC time
      * 
      * \param flagSuccess flag indicating if network time request was succesful
      */
     void UserRequestNetworkTimeCb(void *pVoidUserUTCTime, int flagSuccess) {
        // Explicit conversion from void* to uint32_t* to avoid compiler errors
        uint32_t *pUserUTCTime = (uint32_t *) pVoidUserUTCTime;

        // A struct that will be populated by LMIC_getNetworkTimeReference.
        // It contains the following fields:
        //  - tLocal: the value returned by os_GetTime() when the time
        //            request was sent to the gateway, and
        //  - tNetwork: the seconds between the GPS epoch and the time
        //              the gateway received the time request
        lmic_time_reference_t lmicTimeReference;

        if (flagSuccess != 1) {
            // Most likely the service is not provided by the gateway. No sense in trying again...
            DEBUG_PRINTF_TS("didn't succeed");
            rtcSyncReq = false;
            return;
        }

        // Populate "lmic_time_reference"
        flagSuccess = LMIC_getNetworkTimeReference(&lmicTimeReference);
        if (flagSuccess != 1) {
            DEBUG_PRINTF_TS("LMIC_getNetworkTimeReference didn't succeed");
            return;
        }

        // Update userUTCTime, considering the difference between the GPS and UTC
        // epoch, and the leap seconds
        *pUserUTCTime = lmicTimeReference.tNetwork + 315964800;

        // Add the delay between the instant the time was transmitted and
        // the current time

        // Current time, in ticks
        ostime_t ticksNow = os_getTime();
        // Time when the request was sent, in ticks
        ostime_t ticksRequestSent = lmicTimeReference.tLocal;
        uint32_t requestDelaySec = osticks2ms(ticksNow - ticksRequestSent) / 1000;
        *pUserUTCTime += requestDelaySec;

        // Update the system time with the time read from the network
        rtc.setTime(*pUserUTCTime);
    
        // Save clock sync timestamp and clear flag 
        rtcLastClockSync = rtc.getLocalEpoch();
        rtcSyncReq = false;
        DEBUG_PRINTF_TS("RTC sync completed");
        printDateTime();
    }

    void
    cMyLoRaWAN::requestNetworkTime(void) {
        LMIC_requestNetworkTime(UserRequestNetworkTimeCb, &userUTCTime);
    }
#endif

/****************************************************************************\
|
|	Sensor methods
|
\****************************************************************************/

void
cSensor::setup(std::uint32_t uplinkPeriodMs) {
    // set the initial time.
    this->m_uplinkPeriodMs = uplinkPeriodMs;
    for (int idx=0; idx<NUM_PORTS; idx++)
        this->m_tReference[idx] = millis();
    
    // Initialize your sensors here...
}

void
cSensor::loop(void) {
    auto const tNow = millis();
    

    for (uint8_t idx=0; idx<NUM_PORTS; idx++) {
        //auto const deltaT = tNow - this->m_tReference[idx];
        auto const deltaT = tNow - tReference[idx];
        if (deltaT >= this->m_uplinkPeriodMs * UplinkSchedule[idx].mult) {
            // request an uplink
            this->m_fUplinkRequest[idx] = true;

            // keep trigger time locked to uplinkPeriod
            auto const advance = deltaT / this->m_uplinkPeriodMs;
            //this->m_tReference[idx] += advance * this->m_uplinkPeriodMs;
            tReference[idx] += advance * this->m_uplinkPeriodMs;
        }

        // if an uplink was requested, do it.
        if (this->m_fUplinkRequest[idx] && !this->m_fBusy) {
            this->m_fUplinkRequest[idx] = false;
            this->doUplink(UplinkSchedule[idx].port);
        }
    }    
}

//
// Get battery voltage (Stub)
//
uint16_t
cSensor::getVoltageBattery(void)
{
    const uint16_t voltage = 3850;
     
    DEBUG_PRINTF("Battery Voltage = %dmV", voltage);

    return voltage;
}

//
// Get supply voltage (Stub)
//
uint16_t
cSensor::getVoltageSupply(void)
{
    const uint16_t voltage = 3300;
     
    DEBUG_PRINTF("Supply Voltage = %dmV", voltage);

    return voltage;
}

//
// Get temperature (Stub)
//
float
cSensor::getTemperature(void)
{
    const float temperature = 16.4;
    
    DEBUG_PRINTF("Outdoor Air Temperature = %.1f°C", temperature);
    
    return temperature;
}

    
//
// Prepare uplink data for transmission
//
void
cSensor::doUplink(int port) {
    // if busy uplinking, just skip
    if (this->m_fBusy) {
        DEBUG_PRINTF_TS("doUplink(): busy");
        return;
    }
    // if LMIC is busy, just skip
    if (LMIC.opmode & (OP_POLL | OP_TXDATA | OP_TXRXPEND)) {
        DEBUG_PRINTF_TS("doUplink(): other operation in progress");    
        return;
    }
    
    #if defined(GET_NETWORKTIME)
        //
        // Request time and date
        //
        if (rtcSyncReq) {
            myLoRaWAN.requestNetworkTime();
        }
    #endif

    LoraEncoder encoder(loraData);
    #ifdef GEN_PAYLOAD
        gen_payload(port, encoder);
    #else
        get_payload(port, encoder);
    #endif
    
    this->m_fBusy = true;
    
    if (! myLoRaWAN.SendBuffer(
        loraData, encoder.getLength(),
        // this is the completion function:
        [](void *pClientData, bool fSucccess) -> void {
            auto const pThis = (cSensor *)pClientData;
            pThis->m_fBusy = false;
        },
        (void *)this,
        /* confirmed */ true,
        /* port */ port
        )) {
        // sending failed; callback has not been called and will not
        // be called. Reset busy flag.
        this->m_fBusy = false;
    }
}
