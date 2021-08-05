#define DEBUG 1

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

#define DEFAULT_MIC_SAMPLE_WINDOW 20 // ms
#define DEFAULT_MAX_SIGNAL 255

#define CHANNEL_A 2
#define CHANNEL_B 3
#define CHANNEL_C 4
#define CHANNEL_D 5
#define CHANNEL_E 6
#define CHANNEL_F 7
#define CHANNEL_G 8
#define CHANNEL_H 9

#define ACTIVE_CHANNELS 8

const byte channelOrder[ACTIVE_CHANNELS] = { CHANNEL_A, CHANNEL_B, CHANNEL_C, CHANNEL_D, CHANNEL_E, CHANNEL_F, CHANNEL_G, CHANNEL_H };
byte mode = 0;

void setup() {
#if DEBUG
  Serial.begin(57600);
#endif
  pinMode(CHANNEL_A, OUTPUT);
  pinMode(CHANNEL_B, OUTPUT);
  pinMode(CHANNEL_C, OUTPUT);
  pinMode(CHANNEL_D, OUTPUT);
  pinMode(CHANNEL_E, OUTPUT);
  pinMode(CHANNEL_F, OUTPUT);
  pinMode(CHANNEL_G, OUTPUT);
  pinMode(CHANNEL_H, OUTPUT);

  // digitalWrite(statusChannel, HIGH);

  initRadio();
  playWireStartSequence();
}

struct RadioData {
  uint8_t value = 0;
} radioData;

void loop() {
  Serial.println("1");
  uint32_t timer = millis();
  readRadioData();
  if (mode == 0) {
    runDefaultMode(radioData.value);
    delay(DEFAULT_MIC_SAMPLE_WINDOW);
  }
#if DEBUG
  Serial.println(millis() - timer);
#endif
}

void initRadio() {
  boolean radioOn = radio.begin();
  radio.openWritingPipe(receiverAddress);
  radio.openReadingPipe(1, senderAddress);
  radio.setPayloadSize(sizeof(radioData));
  radio.startListening();
#if DEBUG
  //printf_begin();
  //radio.printPrettyDetails();
  if (!radioOn) {
    Serial.println(F("radio error"));
  }
#endif
}

void readRadioData() {
  uint8_t pipe;
  if (radio.available(&pipe)) {
    uint8_t bytes = radio.getPayloadSize();
    radio.read(&radioData, bytes);
  } else {
#if DEBUG
    Serial.println("no data");
#endif
  }
}

void runDefaultMode(int value) {
  int displayValue = map(value, 0, DEFAULT_MAX_SIGNAL, 0, ACTIVE_CHANNELS);
  displayValue = constrain(displayValue, 0, ACTIVE_CHANNELS);
  lightThreeWiresUpToIndex(displayValue);
}

void lightThreeWiresUpToIndex(int index) {
  lightNumWiresUpToIndex(3, index);
}

void lightWiresUpToIndex(int index) {
  for (int i = 0; i < ACTIVE_CHANNELS; i++) {
    int value = i < index ? HIGH : LOW;
    digitalWrite(channelOrder[i], value);
  }
}

void lightWiresAtIndex(int index) {
  for (int i = 0; i < ACTIVE_CHANNELS; i++) {
    int value = i == index - 1 ? HIGH : LOW;
    digitalWrite(channelOrder[i], value);
  }
}

void lightNumWiresUpToIndex(int num, int index) {
  for (int i = 0; i < ACTIVE_CHANNELS; i++) {
    int value = ((index > i) && (i + num >= index)) ? HIGH : LOW;
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

void playWireStartSequence() {
  for (int i = 0; i <= ACTIVE_CHANNELS; i++) {
    lightWiresUpToIndex(i);
    delay(50);
  }
  for (int i = ACTIVE_CHANNELS; i >= 0; i--) {
    lightWiresUpToIndex(i);
    delay(50);
  }
  for (int i = 0; i < 20; i++) {
    lightWiresUpToIndex(0);
    delay(25);
    lightWiresUpToIndex(ACTIVE_CHANNELS);
    delay(25);
  }
}
