#include <arduino-timer.h>
#include <string>
#include "LoRaWan_APP.h"
#include <DuckLink.h>

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

#define DUCK_ID "0000AB01"
//#define DUCK_ID "000AB02S"

// create a timer with default settings
auto timer = timer_create_default();

// Use pre-built duck link.
DuckLink duck = DuckLink();

int counter = 1;

#define timetillsleep 500 // amount of time we are awake (in ms)
#define timetillwakeup 20000 // amount of time we are asleep (in ms)

static TimerEvent_t sleep;
static TimerEvent_t wakeUp;
uint8_t lowpower = 0;

void onSleep() {
  lowpower = 1;
  // we sleep for timetillwakeup millis
  TimerSetValue( &wakeUp, timetillwakeup );
  TimerStart( &wakeUp );
}

void onWakeUp() {
  lowpower = 0;

  // we have timetillsleep millis before we go to sleep
  // send our message before that.
  TimerSetValue( &sleep, timetillsleep );
  TimerStart( &sleep );

  uint16_t voltage = getBatteryVoltage();
  
  String message = DUCK_ID + String(" counter:") + String(counter) + String(" bat:") + String(voltage);
  
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

