#define DEBUG 0

// EL Sequencer
#define CHANNEL_A 2
#define CHANNEL_B 3
#define CHANNEL_C 4
#define CHANNEL_D 5
#define CHANNEL_E 6
#define CHANNEL_F 7
#define CHANNEL_G 8
#define CHANNEL_H 9
#define ACTIVE_CHANNELS 8
const uint8_t channelOrder[ACTIVE_CHANNELS] = {
  CHANNEL_A, CHANNEL_B, CHANNEL_C, CHANNEL_D, CHANNEL_E, CHANNEL_F, CHANNEL_G, CHANNEL_H
};

// Microphone
#define MIC_OUT A7
#define MIC_GAIN A5
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

// Bluetooth
#define BT_BAUD 57600
#define KWL_BEGIN "*.kwl"
#define KWL_END "*"
String command = "";
boolean outputToBluetooth = false;

void setup() {
  Serial.begin(BT_BAUD);
  initMicrophone();
  initSequencer();
  playWireStartSequence();
}

#define NUM_MODES 4
uint8_t mode = 0;
uint8_t numWires = 8;
#define NUM_DELAYS 4
uint16_t fixedModeDelays[NUM_DELAYS] = { 5, 25, 100, 250 };
uint8_t currentDelayIndex = 1;
uint32_t timer = 0;

void loop() {
  timer = millis();
  readFromSerial();
  handleInput();
  if (mode == 0) {
    readAudioSample();
    processSample();
    runReactiveMode_1();
    if (outputToBluetooth) {
      printToBluetooth();
    }
#if DEBUG
    printToSerial();
#endif
  } else if (mode == 1) {
    readAudioSample();
    processSample();
    runReactiveMode_2();
    if (outputToBluetooth) {
      printToBluetooth();
    }
#if DEBUG
    printToSerial();
#endif
  } else if (mode == 2) {
    runFixedPatternMode_1();
  } else if (mode == 3) {
    runFixedPatternMode_2();
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

uint8_t counter;
void runReactiveMode_2() {
  if (displayValue > 6) {
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


void runFixedPatternMode_3() {
  for (int i = 0; i <= ACTIVE_CHANNELS - 1; i++) {
    lightNumWires(i);
    delay(currentDelay());
  }
  for (int i = ACTIVE_CHANNELS; i >= 1; i--) {
    lightNumWiresUpToWire(i - 1, ACTIVE_CHANNELS);
    delay(currentDelay());
  }
}

uint16_t currentDelay() {
  return fixedModeDelays[currentDelayIndex];
}

void printToSerial() {
  if (Serial.availableForWrite()) {
    Serial.print(low);
    Serial.print(",");
    Serial.print(high);
    Serial.print(",");
    Serial.print(millis() - timer);
    Serial.println();
    Serial.flush();
  }
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
    Serial.println();
    Serial.flush();
  }
}

void readFromSerial() {
  if (Serial.available()) {
    command = Serial.readString();
#if DEBUG
    Serial.println(command);
#endif
  }
}

void handleInput() {
  if (command.startsWith("L")) {
    low = command.substring(1).toInt();
    sendSingleKwlValue(low, "L");
  } else if (command.startsWith("H")) {
    high = command.substring(1).toInt();
    sendSingleKwlValue(high, "H");
  } else if (command.startsWith("D")) {
    outputToBluetooth = true;
  } else if (command.startsWith("d")) {
    outputToBluetooth = false;
  } else if (command.startsWith("W")) {
    micSampleWindow = command.substring(1).toInt();
    sendSingleKwlValue(micSampleWindow, "W");
  } else if (command == "1") {
    nextMode();
  } else if (command == "3") {
    prevMode();
  } else if (command == "2") {
    nextSetting();
  } else if (command == "4") {
    prevSetting();
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
  command = "";
}

void nextMode() {
  mode = (mode + 1) % NUM_MODES;
  printMode();
}

void prevMode() {
  if (--mode < 0) {
    mode = NUM_MODES - 1;
  }
  printMode();
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
    currentDelayIndex = currentDelayIndex - 1;
    if (currentDelayIndex < 0) {
      currentDelayIndex = NUM_DELAYS - 1;
    }
    sendSingleKwlValue(currentDelay(), "S");
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

void initMicrophone() {
  pinMode(MIC_OUT, INPUT);
  pinMode(MIC_GAIN, INPUT);
}

void playWireStartSequence() {
  for (int i = 0; i <= ACTIVE_CHANNELS; i++) {
    lightNumWires(i);
    delay(50);
  }
  for (int i = ACTIVE_CHANNELS; i >= 0; i--) {
    lightNumWires(i);
    delay(50);
  }
  for (int i = 0; i < 20; i++) {
    lightNumWires(0);
    delay(25);
    lightNumWires(ACTIVE_CHANNELS);
    delay(25);
  }
}
