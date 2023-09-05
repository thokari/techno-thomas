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

// Bluetooth
#include "BluetoothElectronics.h"
#define DEVICE_NAME "LOLIN32 Lite"
BluetoothElectronics bluetooth = BluetoothElectronics(DEVICE_NAME);

// EL Sequencer
#include "ELSequencer.h"
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
ELSequencer sequencer = ELSequencer(channelOrder, ACTIVE_CHANNELS);

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
  sequencer.begin();
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
    mappedSignal > 0 ? sequencer.lightWiresAtIndex(mappedSignal - 1) : sequencer.lightNumWires(0);
  } else if (numWires == ACTIVE_CHANNELS) {
    sequencer.lightNumWires(mappedSignal);
  } else {
    sequencer.lightNumWiresUpToWire(numWires, mappedSignal);
  }
}

uint8_t counter;
void runReactiveMode_2() {
  if (mappedSignal > 6) {
    if (counter == 1) {
      uint8_t pattern[ACTIVE_CHANNELS] = { 0, 1, 0, 1, 0, 1, 0, 1 };
      sequencer.lightWiresByPattern(pattern);
    } else {
      uint8_t pattern[ACTIVE_CHANNELS] = { 1, 0, 1, 0, 1, 0, 1, 0 };
      sequencer.lightWiresByPattern(pattern);
    }
    counter = counter == 0 ? 1 : 0;
  }
}

void runFixedPatternMode_1() {
  for (int i = 0; i <= ACTIVE_CHANNELS - 1; i++) {
    sequencer.lightWiresAtIndex(i);
    delay(currentDelay());
    sequencer.lightWiresAtIndex(i + 1);
    delay(currentDelay());
  }
}

uint8_t phase = 0;
void runFixedPatternMode_2() {
  if (phase == 0) {
    uint8_t pattern[ACTIVE_CHANNELS] = { 0, 1, 0, 1, 0, 1, 0, 1 };
    sequencer.lightWiresByPattern(pattern);
    delay(currentDelay());
  } else {
    uint8_t pattern[ACTIVE_CHANNELS] = { 1, 0, 1, 0, 1, 0, 1, 0 };
    sequencer.lightWiresByPattern(pattern);
    delay(currentDelay());
  }
  phase = phase == 0 ? 1 : 0;
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
  bluetooth.registerCommand("L", cmdSetLow);
  bluetooth.registerCommand("H", cmdSetHigh);
  bluetooth.registerCommand("D", cmdDebugOn);
  bluetooth.registerCommand("d", cmdDebugOff);
  bluetooth.registerCommand("S", cmdSetSamplingP2P);
  bluetooth.registerCommand("s", cmdSetSamplingRMS);
  bluetooth.registerCommand("N", cmdSetGain);
  bluetooth.registerCommand("1", cmdUp);
  bluetooth.registerCommand("3", cmdDown);
  bluetooth.registerCommand("2", cmdRight);
  bluetooth.registerCommand("4", cmdLeft);
}

void cmdSetLow(const String& parameter) {
  mic.setLow(parameter.toInt());
  bluetooth.sendKwlValue(mic.getLow(), "L");
}

void cmdSetHigh(const String& parameter) {
  mic.setHigh(parameter.toInt());
  bluetooth.sendKwlValue(mic.getHigh(), "H");
}

void cmdDebugOn(const String&) {
    outputToBluetooth = true;
}

void cmdDebugOff(const String&) {
    outputToBluetooth = false;
}

void cmdSetSamplingP2P(const String&) {
    mic.setMode(LoudnessMeter::PEAK_TO_PEAK);
    bluetooth.sendKwlString("P2P", "P");
}

void cmdSetSamplingRMS(const String&) {
    mic.setMode(LoudnessMeter::RMS);
    bluetooth.sendKwlString("RMS", "P");
}

void cmdSetGain(const String& parameter) {
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

void cmdUp(const String&) {
    nextMode();
}

void cmdDown(const String&) {
    prevMode();
}

void cmdRight(const String&) {
    nextSetting();
}

void cmdLeft(const String&) {
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
    bluetooth.sendKwlString("R1", "M");
    bluetooth.sendKwlValue(numWires, "S");
  } else if (mode == 1) {
    bluetooth.sendKwlString("R2", "M");
    bluetooth.sendKwlValue(numWires, "S");
  } else if (mode == 2) {
    bluetooth.sendKwlString("F1", "M");
    bluetooth.sendKwlValue(currentDelay(), "S");
  } else if (mode == 3) {
    bluetooth.sendKwlString("F2", "M");
    bluetooth.sendKwlValue(currentDelay(), "S");
  }
}

uint16_t currentDelay() {
  return fixedModeDelays[currentDelayIndex];
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
