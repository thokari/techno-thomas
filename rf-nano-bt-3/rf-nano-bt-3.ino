#define DEBUG 0
#define USE_RADIO 0

#define BT_BAUD 57600

// LoudnessMeter
#include "LoudnessMeter.h"
#define DEBUG_VALUE 1
#define DEBUG_BAUD_RATE 57600
#define MIC_OUT A0
#define MIC_GAIN 2
#define MIC_SAMPLE_WINDOW 16 // ms
#define DEFAULT_P2P_LOW 300
#define DEFAULT_P2P_HIGH 700
#define DEFAULT_RMS_LOW 1000
#define DEFAULT_RMS_HIGH 1400
#define MAX_MAPPED_VALUE 8
LoudnessMeter mic = LoudnessMeter(MIC_OUT, MIC_GAIN, MIC_SAMPLE_WINDOW, DEFAULT_P2P_LOW, DEFAULT_P2P_HIGH, DEFAULT_RMS_LOW, DEFAULT_RMS_HIGH);
uint16_t mappedSignal;

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
  if (Serial.availableForWrite()) {
    Serial.println("Ready");
  }
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
    mic.readAudioSample();
    processSample();
    runReactiveMode_1();
    if (outputToBluetooth) {
      printToBluetooth();
    }
#if DEBUG
    printToSerialMonitor();
#endif
  } else if (mode == 1) {
    mic.readAudioSample();
    processSample();
    runReactiveMode_2();
    if (outputToBluetooth) {
      printToBluetooth();
    }
#if DEBUG
    printToSerialMonitor();
#endif
  } else if (mode == 2) {
    runFixedPatternMode_1();
  } else if (mode == 3) {
    runFixedPatternMode_2();
  }
}

void runReactiveMode_1() {
  if (numWires == 1) {
    mappedSignal > 0 ? lightWiresAtIndex(mappedSignal - 1) : lightNumWires(0);
  } else if (numWires == ACTIVE_CHANNELS) {
    lightNumWires(mappedSignal);
  } else {
    lightNumWiresUpToWire(numWires, mappedSignal);
  }
}

uint8_t counter;
void runReactiveMode_2() {
  if (mappedSignal > 6) {
    if (counter == 1) {
      uint8_t pattern[ACTIVE_CHANNELS] = { 0, 1, 0, 1, 0, 1, 0, 1 };
      lightWiresByPattern(pattern);
    } else {
      uint8_t pattern[ACTIVE_CHANNELS] = { 1, 0, 1, 0, 1, 0, 1, 0 };
      lightWiresByPattern(pattern);
    }
    counter = counter == 0 ? 1 : 0;
  }
}

void runFixedPatternMode_1() {
  for (int i = 0; i <= ACTIVE_CHANNELS - 1; i++) {
    lightWiresAtIndex(i);
    delay(currentDelay());
    lightWiresAtIndex(i + 1);
    delay(currentDelay());
  }
}

uint8_t phase = 0;
void runFixedPatternMode_2() {
  if (phase == 0) {
    uint8_t pattern[ACTIVE_CHANNELS] = { 0, 1, 0, 1, 0, 1, 0, 1 };
    lightWiresByPattern(pattern);
    delay(currentDelay());
  } else {
    uint8_t pattern[ACTIVE_CHANNELS] = { 1, 0, 1, 0, 1, 0, 1, 0 };
    lightWiresByPattern(pattern);
    delay(currentDelay());
  }
  phase = phase == 0 ? 1 : 0;
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
  Serial.print(mic.getLow());
  Serial.print(",");
  Serial.print(mic.getHigh());
  Serial.print(",");
  Serial.print(millis() - loopBegin);
  Serial.println();
}

void printToBluetooth() {
  if (Serial.availableForWrite()) {
    Serial.print("*G");
    Serial.print(mic.getSignal());
    Serial.print(",");
    Serial.print(mic.getLow());
    Serial.print(",");
    Serial.print(mic.getHigh());
    Serial.print("*");
  }
}

void processSample() {
#if DEBUG
  Serial.print(mic.getSignal());
  Serial.print(",");
#endif
  uint16_t contrainedSignal = constrain(mic.getSignal(), mic.getLow(), mic.getHigh());
  mappedSignal = map(contrainedSignal, mic.getLow(), mic.getHigh(), 0, ACTIVE_CHANNELS);
}

void handleInput() {
  if (Serial.available()) {
    command = Serial.readString();
    if (command.startsWith("L")) {
      mic.setLow(command.substring(1).toInt());
      sendSingleKwlValue(mic.getLow(), "L");
    } else if (command.startsWith("H")) {
      mic.setHigh(command.substring(1).toInt());
      sendSingleKwlValue(mic.getHigh(), "H");
    } else if (command == "D") {
      outputToBluetooth = true;
    } else if (command == "d") {
      outputToBluetooth = false;
    } else if (command == "S") {
      mic.setMode(LoudnessMeter::PEAK_TO_PEAK);
      sendSingleKwlString("P2P", "P");
    } else if (command == "s") {
      mic.setMode(LoudnessMeter::RMS);
      sendSingleKwlString("RMS", "P");
    } else if (command.startsWith("N")) {
      int gain = command.substring(1).toInt();
      if (gain == 1) {
        mic.setGain(LoudnessMeter::LOW_GAIN);
      } else if (gain == 2) {
        mic.setGain(LoudnessMeter::MEDIUM_GAIN);
      } else if (gain == 3) {
        mic.setGain(LoudnessMeter::HIGH_GAIN);
      }
      sendSingleKwlValue(gain, "N");
    } else if (command == "1") {
      nextMode();
    } else if (command == "3") {
      prevMode();
    } else if (command == "2") {
      nextSetting();
    } else if (command == "4") {
      prevSetting();
    }
  }
}

void nextMode() {
  mode = (mode + 1) % NUM_MODES;
  printMode();
}

void prevMode() {
  if (mode == 0) {
    mode = NUM_MODES - 1;
  } else {
    mode--;
  }
  printMode();
}


void nextSetting() {
  if (mode < 2) {
    if (++numWires > ACTIVE_CHANNELS) {
      numWires = 1;
    }
    sendSingleKwlValue(numWires, "S");
  } else if (mode >= 2) {
    currentDelayIndex = (currentDelayIndex + 1) % NUM_DELAYS;
    sendSingleKwlValue(currentDelay(), "S");
  }
}

void prevSetting() {
  if (mode < 2) {
    numWires = numWires - 1;
    if (numWires == 0) {
      numWires = ACTIVE_CHANNELS;
    }
    sendSingleKwlValue(numWires, "S");
  } else if (mode >= 2) {
    if (currentDelayIndex == 0) {
      currentDelayIndex = NUM_DELAYS - 1;
    } else {
      currentDelayIndex--;
    }
    sendSingleKwlValue(currentDelay(), "S");
  }
}

void printMode() {
  if (mode == 0) {
    sendSingleKwlString("R1", "M");
    sendSingleKwlValue(numWires, "S");
  } else if (mode == 1) {
    sendSingleKwlString("R2", "M");
    sendSingleKwlValue(numWires, "S");
  } else if (mode == 2) {
    sendSingleKwlString("F1", "M");
    sendSingleKwlValue(currentDelay(), "S");
  } else if (mode == 3) {
    sendSingleKwlString("F2", "M");
    sendSingleKwlValue(currentDelay(), "S");
  }
}

uint16_t currentDelay() {
  return fixedModeDelays[currentDelayIndex];
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
  while (Serial.availableForWrite() < cmd.length());
  Serial.println(cmd);
}

void sendSingleKwlString(String input, String receiveChar) {
  String cmd = "*" + receiveChar + input + "*";
  while (Serial.availableForWrite() < cmd.length());
  Serial.println(cmd);
}

void sendSingleKwlCommand(String input) {
  String cmd = String(KWL_BEGIN) + "\n" + input + "\n" + String(KWL_END);
  while (Serial.availableForWrite() < cmd.length());
  Serial.println(cmd);
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
  mic.begin();
}

void playWireStartSequence() {
  for (int i = 0; i <=  ACTIVE_CHANNELS; i++) {
    lightNumWires(i);
    delay(100);
  }
  for (int i = ACTIVE_CHANNELS; i >= 0; i--) {
    lightNumWires(i);
    delay(100);
  }
  for (int i = 0; i < 10; i++) {
    lightNumWires(0);
    delay(50);
    lightNumWires(ACTIVE_CHANNELS);
    delay(50);
  }
}
