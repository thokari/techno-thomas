#define DEBUG 0
#define DEBUG_SIGNAL 0
#define DEBUG_MEMORY 0

#define BT_BAUD 57600

// Radio
#include "RF24.h"
#define CE_PIN 9
#define CSN_PIN 10
#if DEBUG
#include "printf.h"
#endif
const RF24 radio(CE_PIN, CSN_PIN);
const uint64_t senderAddress = 0x1010101010;
const uint64_t receiverAddress = 0x2020202020;

// Microphone
#define MIC_OUT A0
#define DEFAULT_MIC_SAMPLE_WINDOW 20 // ms
#define MAX_SIGNAL 1023
#define MAPPED_MAX_SIGNAL 255
#define DEFAULT_LOW 50
#define DEFAULT_HIGH 200

// Bluetooth Electronics
#define KWL_BEGIN "*.kwl"
#define KWL_END "*"

void setup() {
  Serial.begin(BT_BAUD);
  initMicrophone();
  initRadio();
  if (Serial.availableForWrite()) {
    Serial.println("Ready");
  }
}

uint8_t micSampleWindow = DEFAULT_MIC_SAMPLE_WINDOW;
int32_t currentSum = 0;
uint16_t currentMin = MAX_SIGNAL;
uint16_t currentMax = 0;
uint16_t currentSignal = 0;
uint16_t low = DEFAULT_LOW;
uint16_t high = DEFAULT_HIGH;

struct RadioData {
  uint8_t value = 0;
} radioData;

String command = "";

boolean outputToSerial = true;
boolean outputToBluetooth = false;

uint32_t loopBegin = 0;

void loop() {
  loopBegin = millis();
  handleInput();
  readAudioSample();
  radioData.value = processSignal();
  sendRadioData();

  if (outputToBluetooth) {
    printToBluetooth();
  }
#if DEBUG
  if (outputToSerial) {
    printToSerialMonitor();
  }
#endif
}

uint8_t processSignal() {
  currentSignal = currentMax - currentMin;
#if DEBUG
  Serial.print(currentSignal);
  Serial.print(",");
#endif
  uint16_t mappedSignal = constrain(currentSignal, low, high);
  mappedSignal = map(mappedSignal, low, high, 0, MAPPED_MAX_SIGNAL);
  return mappedSignal;
}

void printToSerialMonitor() {
  Serial.print(low);
  Serial.print(",");
  Serial.print(high);
  Serial.println();
}

void printToBluetooth() {
  if (Serial.availableForWrite()) {
    Serial.print("*G");
    Serial.print(currentSignal);
    Serial.print(",");
    Serial.print(low);
    Serial.print(",");
    Serial.print(high);
    Serial.print("*");
  }

}

void readAudioSample() {
  currentMin = 1023;
  currentMax = 0;
  currentSum = 0;
  uint32_t t0 = millis();
  while (millis() - t0 < micSampleWindow) {
    uint16_t value = analogRead(MIC_OUT);
    currentSum += value;
    currentMin = min(currentMin, value);
    currentMax = max(currentMax, value);
  }
}

void handleInput() {
  if (Serial.available()) {
    command = Serial.readString();
    if (command.startsWith("L")) {
      low = command.substring(1).toInt();
      sendSingleKwlValue(low, "L");
    } else if (command.startsWith("H")) {
      high = command.substring(1).toInt();
      sendSingleKwlValue(high, "H");
    } else if (command == "D") {
      outputToBluetooth = true;
    } else if (command == "d") {
      outputToBluetooth = false;
    }
  }
}

void sendSingleKwlValue(int value, String receiveChar) {
  String cmd = "*" + receiveChar + value + "*";
  if (Serial.availableForWrite()) {
    Serial.println(cmd);
  }
}

void sendSingleKwlCommand(String cmd) {
  if (Serial.availableForWrite()) {
    Serial.println(KWL_BEGIN);
    Serial.println(cmd);
    Serial.println(KWL_END);
  }
}

void initRadio() {
  boolean radioOn = radio.begin();
  radio.openWritingPipe(senderAddress);
  radio.openReadingPipe(1, receiverAddress);
  //radio.setRetries(1, 0);
  radio.setPayloadSize(sizeof(radioData));
#if DEBUG
  if (!radioOn) {
    Serial.println(F("radio error"));
  }
#endif
}

void sendRadioData() {
  radio.stopListening();
  boolean success = radio.write(&radioData, sizeof(radioData));
  //radio.startListening();
#if DEBUG
  if (!success) {
    Serial.println(F("sending failed"));
  }
#endif
}

void initMicrophone() {
  analogReference(EXTERNAL);
  analogRead(MIC_OUT);
}

// https://learn.adafruit.com/memories-of-an-arduino/measuring-free-memory
#if DEBUG_MEMORY
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}
#endif // DEBUG_MEMORY
