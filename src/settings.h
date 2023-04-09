 
//#define useModulPower   1

//#define ENABLE_JSON
//#define GEN_PAYLOAD               // Generate payload for debugging (do not read Modbus)
//#define SERIAL_RATE     115200    // Serial speed for status info
//#define SLAVE_ID        1         // Default slave ID of Growatt
//#define MODBUS_RATE     9600      // Modbus speed of Growatt, do not change

#define UPDATE_MODBUS   2         // Modbus device is read every <n> seconds
#define MODBUS_RETRIES  5         // no. of modbus retries
#define INTERFACE_SEL   13        // Modbus interface select (0: USB / 1: RS485)
#define MAX485_DE       10        // D1, DE pin on the TTL to RS485 converter
#define MAX485_RE_NEG   2         // D2, RE pin on the TTL to RS485 converter
#define MAX485_RX       27        // D5, RO pin on the TTL to RS485 converter
#define MAX485_TX       9         // D6, DI pin on the TTL to RS485 converter
#define DEBUG_TX        5
#define DEBUG_RX        26
#define STATUS_LED    LED_BUILTIN // Status LED
