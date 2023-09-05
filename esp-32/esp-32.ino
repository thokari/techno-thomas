#define DEBUG 0
#define DEBUG_BAUD_RATE 57600
#define USE_RADIO 0

// LoudnessMeter
#include "LoudnessMeter.h"
#define MIC_OUT 34
#define MIC_GAIN 35
#define MIC_SAMPLE_WINDOW 16  // ms
#define DEFAULT_P2P_LOW 1000
#define DEFAULT_P2P_HIGH 4000
#define DEFAULT_RMS_LOW 1000
#define DEFAULT_RMS_HIGH 1400
#define MAX_MAPPED_VALUE 8
LoudnessMeter mic = LoudnessMeter(
  MIC_OUT, MIC_GAIN, MIC_SAMPLE_WINDOW,
  DEFAULT_P2P_LOW, DEFAULT_P2P_HIGH,
  DEFAULT_RMS_LOW, DEFAULT_RMS_HIGH);
uint16_t mappedSignal;

// EL Sequencer
#define CHANNEL_A 13
#define CHANNEL_B 15
#define CHANNEL_C 2
#define CHANNEL_D 0
#define CHANNEL_E 4
#define CHANNEL_F 16
#define CHANNEL_G 17
#define CHANNEL_H 5
#define ACTIVE_CHANNELS 8
const uint8_t channelOrder[ACTIVE_CHANNELS] = {
  CHANNEL_A, CHANNEL_B, CHANNEL_C, CHANNEL_D, CHANNEL_E, CHANNEL_F, CHANNEL_G, CHANNEL_H
};

// Bluetooth
#include "BluetoothElectronics.h"
#define DEVICE_NAME "LOLIN32 Lite"
BluetoothElectronics bluetooth = BluetoothElectronics(DEVICE_NAME);

#define NUM_MODES 4
uint8_t mode = 0;
uint8_t numWires = 8;
#define NUM_DELAYS 6
uint16_t fixedModeDelays[NUM_DELAYS] = { 5, 10, 25, 50, 100, 250 };
uint8_t currentDelayIndex = 1;
uint32_t timer = 0;

#if USE_RADIO
struct RadioData {
  uint8_t value = 0;
} radioData;
#endif

void setup() {
  Serial.begin(DEBUG_BAUD_RATE);
  registerBluetoothCommands();
  bluetooth.begin();
  mic.begin();
#if USE_RADIO
  initRadio();
#endif
  initSequencer();
  playWireStartSequence();
  Serial.println("Setup complete");
}

boolean outputToBluetooth = false;

uint32_t loopBegin = 0;

void loop() {
  loopBegin = millis();
  bluetooth.handleInput();
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
  String data = String(mic.getSignal()) + "," + String(mic.getLow()) + "," + String(mic.getHigh());
  bluetooth.sendKwlString(data, "G");
}

void processSample() {
// #if DEBUG
//   Serial.print(mic.getSignal());
//   Serial.print(",");
// #endif
  uint16_t contrainedSignal = constrain(mic.getSignal(), mic.getLow(), mic.getHigh());
  mappedSignal = map(contrainedSignal, mic.getLow(), mic.getHigh(), 0, ACTIVE_CHANNELS);
}

void registerBluetoothCommands() {
  bluetooth.registerCommand("L", handleLCommand);
  bluetooth.registerCommand("H", handleHCommand);
  bluetooth.registerCommand("D", handleDCommand);
  bluetooth.registerCommand("d", handledCommand);
  bluetooth.registerCommand("S", handleSCommand);
  bluetooth.registerCommand("s", handlesCommand);
  bluetooth.registerCommand("N", handleNCommand);
  bluetooth.registerCommand("1", handle1Command);
  bluetooth.registerCommand("3", handle3Command);
  bluetooth.registerCommand("2", handle2Command);
  bluetooth.registerCommand("4", handle4Command);
}

void handleLCommand(const String& parameter) {
  mic.setLow(parameter.toInt());
  bluetooth.sendKwlValue(mic.getLow(), "L");
}

void handleHCommand(const String& parameter) {
  mic.setHigh(parameter.toInt());
  bluetooth.sendKwlValue(mic.getHigh(), "H");
}

void handleDCommand(const String&) {
    outputToBluetooth = true;
}

void handledCommand(const String&) {
    outputToBluetooth = false;
}

void handleSCommand(const String&) {
    mic.setMode(LoudnessMeter::PEAK_TO_PEAK);
    bluetooth.sendKwlString("P2P", "P");
}

void handlesCommand(const String&) {
    mic.setMode(LoudnessMeter::RMS);
    bluetooth.sendKwlString("RMS", "P");
}

void handleNCommand(const String& parameter) {
    int gain = parameter.toInt();
    if (gain == 1) {
        mic.setGain(LoudnessMeter::LOW_GAIN);
    } else if (gain == 2) {
        mic.setGain(LoudnessMeter::MEDIUM_GAIN);
    } else if (gain == 3) {
        mic.setGain(LoudnessMeter::HIGH_GAIN);
    }
    bluetooth.sendKwlValue(gain, "N");
}

void handle1Command(const String&) {
    nextMode();
}

void handle3Command(const String&) {
    prevMode();
}

void handle2Command(const String&) {
    nextSetting();
}

void handle4Command(const String&) {
    prevSetting();
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
    bluetooth.sendKwlValue(numWires, "S");
  } else if (mode >= 2) {
    currentDelayIndex = (currentDelayIndex + 1) % NUM_DELAYS;
    bluetooth.sendKwlValue(currentDelay(), "S");
  }
}

void prevSetting() {
  if (mode < 2) {
    numWires = numWires - 1;
    if (numWires == 0) {
      numWires = ACTIVE_CHANNELS;
    }
    bluetooth.sendKwlValue(numWires, "S");
  } else if (mode >= 2) {
    if (currentDelayIndex == 0) {
      currentDelayIndex = NUM_DELAYS - 1;
    } else {
      currentDelayIndex--;
    }
    bluetooth.sendKwlValue(currentDelay(), "S");
  }
}

void printMode() {
  if (mode == 0) {
    Serial.println("Mode is 0");
    bluetooth.sendKwlString("R1", "M");
    bluetooth.sendKwlValue(numWires, "S");
  } else if (mode == 1) {
    Serial.println("Mode is 1");
    bluetooth.sendKwlString("R2", "M");
    bluetooth.sendKwlValue(numWires, "S");
  } else if (mode == 2) {
    Serial.println("Mode is 2");
    bluetooth.sendKwlString("F1", "M");
    bluetooth.sendKwlValue(currentDelay(), "S");
  } else if (mode == 3) {
    Serial.println("Mode is 3");
    bluetooth.sendKwlString("F2", "M");
    bluetooth.sendKwlValue(currentDelay(), "S");
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

void playWireStartSequence() {
  for (int i = 0; i <= ACTIVE_CHANNELS; i++) {
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
