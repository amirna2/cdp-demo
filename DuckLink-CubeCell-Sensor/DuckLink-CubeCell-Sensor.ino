#include <arduino-timer.h>
#include <string>
#include "LoRaWan_APP.h"
#include <DuckLink.h>

// for the BME280
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

#define DUCK_ID "0000AB01"
#define timetillsleep 500 // amount of time we are awake (in ms) - enough time to capture sensor data and send them.
#define timetillwakeup (60 * 1000) // amount of time we are asleep (in ms) - Typically should be 45 minutes (set to 1 minute for demo)

// create a timer with default settings
auto timer = timer_create_default();

// ------------------- Sensor handling ------------------------------------- 
// sensor data: variable length
// [header]       1 byte  - sensor bitmap
// [battery]      2 bytes - Volt ( e.g 355 -> 3.55v)
// [temp]         1 byte  - Farenheit
// [humidity]     1 byte  - percent
// [pressure]     2 bytes - hPa
// [moisture]     1 byte  - percent
// [ground temp]  1 byte  - Farenheit
// [light]        2 bytes - Lux (max 65535)

#define BAT       0x01
#define ATEMP     0x02
#define HUMID     0x04
#define PRESS     0x08

// Create bme280 I2C sensor object
Adafruit_BME280 bme;

bool always_send = true;

//const uint8_t PACKET_SIZE = 10;
//uint8_t fullPayload[PACKET_SIZE];
uint8_t *fullPayload;
uint8_t data_length = 1; // counting the header

typedef struct sensor_data {
  uint16_t pressure;
  uint16_t battery;
  int8_t air_temp;
  int8_t humidity;
} sensor_data;

sensor_data sensorData;

// Use pre-built duck link.
DuckLink duck = DuckLink();

// Low power variables
static TimerEvent_t sleep;
static TimerEvent_t wakeUp;
uint8_t lowpower = 0;



void init_bme280() {
  unsigned status = bme.begin();
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(), 16);
    while (1);
  }
  Serial.println();
}

bool get_sensor_data() {
  uint16_t p, b;
  int8_t t, h = 20;

  if ( fullPayload != NULL ) {
    data_length = 1;
    free(fullPayload);
  }

  // Acquire all sensor data
  
  // ambient air temp
  t = bme.readTemperature();
  // barometric pressure
  p = bme.readPressure() / 100.0F;
  // air humidity
  h = bme.readHumidity(); // Percentage
  // read battery voltage
  b = getBatteryVoltage();

  Serial.println("============================");

  // Build our sensor payload by comparing with previous value.
  // Only send what's changed unless always_send is set to true
  
  uint8_t header = 0x00;

  if (b != sensorData.battery || always_send) {
    header += BAT;
    data_length += 2;
    sensorData.battery = b;
    Serial.print("B = "); Serial.print((float)b / 100.00); Serial.println("v");
  }

  if (t != sensorData.air_temp || always_send) {
    data_length += 1;
    header += ATEMP;
    sensorData.air_temp = t;
    Serial.print("T = "); Serial.print(t); Serial.println(" *C");
  }

  if (h != sensorData.humidity || always_send) {
    data_length += 1;
    header += HUMID;
    sensorData.humidity = h;
    Serial.print("H = "); Serial.print(h); Serial.println(" %");
  }

  if (p != sensorData.pressure || always_send) {
    data_length += 2;
    header += PRESS;
    sensorData.pressure = p;
    Serial.print("P = "); Serial.print(p); Serial.println(" hPa");
  }

  // don't send data if nothing's changed
  if (header == 0) {
    return false;
  }
  
  // [header]       1 byte
  // [battery]      2 bytes - Volt
  // [temp]         1 byte  - Celicius
  // [humidity]     1 byte  - Percent
  // [pressure]     2 bytes - HPa
  // [moisture]     1 byte  - Percent
  // [ground temp]  1 byte  - Celcius
  // [light]        2 bytes - Lux

  fullPayload = (uint8_t *) malloc(sizeof(uint8_t) * data_length);

  int pos = 0;
  fullPayload[pos] = header;

  if ((header & BAT) == BAT) {
    fullPayload[++pos] = lowByte(sensorData.battery);
    fullPayload[++pos] = highByte(sensorData.battery);
  }
  if ((header & ATEMP) == ATEMP) {
    fullPayload[++pos] = sensorData.air_temp;
  }
  if ((header & HUMID) == HUMID) {
    fullPayload[++pos] = sensorData.humidity;
  }
  if ((header & PRESS) == PRESS) {
    fullPayload[++pos] = lowByte(sensorData.pressure);
    fullPayload[++pos] = highByte(sensorData.pressure);
  }

  Serial.print(" length:"); Serial.println(data_length);
  for (int i = 0; i < data_length; ++i) {
    Serial.print("0x");
    Serial.print(fullPayload[i], HEX);
    Serial.print(" ");
  }

  Serial.println("");
  Serial.println("============================");

  return true;
}


// Handles sleep cycle
void onSleep() {
  lowpower = 1;
  // we sleep for timetillwakeup millis
  TimerSetValue( &wakeUp, timetillwakeup );
  TimerStart( &wakeUp );
}

// Handles wake cycle
void onWakeUp() {
  lowpower = 0;

  // we have timetillsleep millis before we go to sleep
  // send our message before that.
  TimerSetValue( &sleep, timetillsleep );
  TimerStart( &sleep );

  uint16_t voltage = getBatteryVoltage();
  
  String message = String(voltage);
  
  //Leave this commented when using low power mode
  //Serial.println("[DUCK] "+ message);
  
  const byte* buffer = (byte*) message.c_str();
  // Send Data can either take a byte buffer (unsigned char) or a vector
  int err = duck.sendData(topics::status, buffer, message.length());
  if (err != DUCK_ERR_NONE) {
    return;
  }
  counter++;
}

void setup() {
  // we are using a hardcoded device id here, but it should be retrieved or given during the device provisioning
  // then converted to a byte vector to setup the duck
  std::string deviceId(DUCK_ID);
  std::vector<byte> devId;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end());

  duck.setupWithDefaults(devId);

  TimerInit( &sleep, onSleep );
  TimerInit( &wakeUp, onWakeUp );
  onSleep();
}

void loop() {
  if (lowpower) {
    //note that lowPowerHandler() runs six times before the mcu goes into lowpower mode;
    lowPowerHandler();
  }
  // these call should not wake up the cpu for anything
  timer.tick();
  duck.run();
}
