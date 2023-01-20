#define DEBUG 1
#define DEBUG_MEMORY 0
#define USE_RADIO 0

#define BT_BAUD 57600

// Microphone
#define MIC_OUT A0
#define MIC_GAIN 2
#define DEFAULT_MIC_SAMPLE_WINDOW 16 // ms
#define MAX_SIGNAL 1023
#define DEFAULT_LOW 150
#define DEFAULT_HIGH 450
uint8_t micSampleWindow = DEFAULT_MIC_SAMPLE_WINDOW;
int32_t currentSum = 0;
uint16_t currentMin = MAX_SIGNAL;
uint16_t currentMax = 0;
uint16_t currentSignal = 0;
uint16_t low = DEFAULT_LOW;
uint16_t high = DEFAULT_HIGH;
uint16_t mappedSignal = 0;
uint16_t displayValue = 0;
uint16_t prevDisplayValue = 0;

// EL Sequencer
#define CHANNEL_A 7
#define CHANNEL_B 8
#define CHANNEL_C 3
#define CHANNEL_D 5
#define CHANNEL_E 6
#define CHANNEL_F 9
#define CHANNEL_G 10
#define CHANNEL_H 11
#define ACTIVE_CHANNELS 8
const uint8_t channelOrder[ACTIVE_CHANNELS] = {
  CHANNEL_A, CHANNEL_B, CHANNEL_C, CHANNEL_D, CHANNEL_E, CHANNEL_F, CHANNEL_G, CHANNEL_H
};

// Bluetooth Electronics
#define KWL_BEGIN "*.kwl"
#define KWL_END "*"

// Radio
#if USE_RADIO
#include "RF24.h"
#define CE_PIN 9
#define CSN_PIN 10
#if DEBUG
#include "printf.h"
#endif
const RF24 radio(CE_PIN, CSN_PIN);
const uint64_t senderAddress = 0x1010101010;
const uint64_t receiverAddress = 0x2020202020;
#endif

#define NUM_MODES 4
uint8_t mode = 0;
uint8_t numWires = 8;
#define NUM_DELAYS 4
uint16_t fixedModeDelays[NUM_DELAYS] = { 5, 25, 100, 250 };
uint8_t currentDelayIndex = 1;
uint32_t timer = 0;

void setup() {
  Serial.begin(BT_BAUD);
  initMicrophone();
#if USE_RADIO
  initRadio();
#endif
  pinMode(MIC_GAIN, INPUT);
  if (Serial.availableForWrite()) {
    Serial.println("Ready");
  }
  pinMode(MIC_GAIN, INPUT);
  initSequencer();
  playWireStartSequence();
}

#if USE_RADIO
struct RadioData {
  uint8_t value = 0;
} radioData;
#endif

String command = "";

boolean outputToSerial = true;
boolean outputToBluetooth = false;

uint32_t loopBegin = 0;

void loop() {
  loopBegin = millis();
  handleInput();
  if (mode == 0) {
    readAudioSample();
    processSample();
    runReactiveMode_1();
    if (outputToBluetooth) {
      printToBluetooth();
    }
#if DEBUG
    printToSerialMonitor();
#endif
  } else if (mode == 1) {
    readAudioSample();
    processSample();
//    runReactiveMode_2();
    if (outputToBluetooth) {
      printToBluetooth();
    }
#if DEBUG
    printToSerialMonitor();
#endif
  } else if (mode == 2) {
//    runFixedPatternMode_1();
  } else if (mode == 3) {
//    runFixedPatternMode_2();
  }
}

void runReactiveMode_1() {
  if (numWires == 1) {
    displayValue > 0 ? lightWiresAtIndex(displayValue - 1) : lightNumWires(0);
  } else if (numWires == ACTIVE_CHANNELS) {
    lightNumWires(displayValue);
  } else {
    lightNumWiresUpToWire(numWires, displayValue);
  }
}


void lightNumWires(int num) {
  for (int i = 0; i < ACTIVE_CHANNELS; i++) {
    int value = i < num ? HIGH : LOW;
    digitalWrite(channelOrder[i], value);
  }
}

void lightWiresAtIndex(int index) {
  for (int i = 0; i < ACTIVE_CHANNELS; i++) {
    int value = i == index ? HIGH : LOW;
    digitalWrite(channelOrder[i], value);
  }
}

void lightNumWiresUpToWire(int num, int wireNum) {
  for (int i = 0; i < ACTIVE_CHANNELS; i++) {
    int value = ((wireNum > i) && (i + num >= wireNum)) ? HIGH : LOW;
    digitalWrite(channelOrder[i], value);
  }
}

void lightRandomWires(int index) {
  if (index == ACTIVE_CHANNELS) {
    for (int i = 0; i < ACTIVE_CHANNELS; i++) {
      int value = random(0, 2) > 0 ? HIGH : LOW;
      digitalWrite(channelOrder[i], value);
    }
  }
}

void lightWiresByPattern(uint8_t pattern[ACTIVE_CHANNELS]) {
  for (int i = 0; i < ACTIVE_CHANNELS; i++) {
    int value = pattern[i] > 0 ? HIGH : LOW;
    digitalWrite(channelOrder[i], value);
  }
}

void printToSerialMonitor() {
  Serial.print(low);
  Serial.print(",");
  Serial.print(high);
  Serial.print(",");
  Serial.print(millis() - loopBegin);
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

void processSample() {
  currentSignal = currentMax - currentMin;
#if DEBUG
  Serial.print(currentSignal);
  Serial.print(",");
#endif
  mappedSignal = constrain(currentSignal, low, high);
  prevDisplayValue = displayValue;
  displayValue = map(mappedSignal, low, high, 0, ACTIVE_CHANNELS);
#if DEBUG
  Serial.print(displayValue);
  Serial.print(",");
#endif
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
    } else if (command.startsWith("S")) {
      micSampleWindow = command.substring(1).toInt();
      sendSingleKwlValue(micSampleWindow, "S");
    } else if (command.startsWith("N")) {
      int gain = command.substring(1).toInt();
      if (gain == 1) {
        pinMode(MIC_GAIN, OUTPUT);
        digitalWrite(MIC_GAIN, HIGH);
      } else if (gain == 2) {
        pinMode(MIC_GAIN, OUTPUT);
        digitalWrite(MIC_GAIN, LOW);
      } else if (gain == 3) {
        pinMode(MIC_GAIN, INPUT);
      }
      sendSingleKwlValue(gain, "N");
    }
  }
}

void initSequencer() {
  pinMode(CHANNEL_A, OUTPUT);
  pinMode(CHANNEL_B, OUTPUT);
  pinMode(CHANNEL_C, OUTPUT);
  pinMode(CHANNEL_D, OUTPUT);
  pinMode(CHANNEL_E, OUTPUT);
  pinMode(CHANNEL_F, OUTPUT);
  pinMode(CHANNEL_G, OUTPUT);
  pinMode(CHANNEL_H, OUTPUT);
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

#if USE_READIO
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
#endif

void initMicrophone() {
  analogReference(EXTERNAL);
  analogRead(MIC_OUT);
}

void playWireStartSequence() {
  for (int i = 0; i <= ACTIVE_CHANNELS; i++) {
    lightNumWires(i);
    delay(250);
  }
  for (int i = ACTIVE_CHANNELS; i >= 0; i--) {
    lightNumWires(i);
    delay(250);
  }
  for (int i = 0; i < 20; i++) {
    lightNumWires(0);
    delay(25);
    lightNumWires(ACTIVE_CHANNELS);
    delay(25);
  }
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
