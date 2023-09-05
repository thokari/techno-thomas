#include "ELSequencer.h"

ELSequencer::ELSequencer(const uint8_t channelOrder[], const uint8_t channelCount)
  : channelOrder(channelOrder), channelCount(channelCount) {}

void ELSequencer::begin() {
  initSequencer();
  playWireStartSequence();
}

void ELSequencer::lightNumWires(uint8_t num) {
  for (uint8_t i = 0; i < channelCount; i++) {
    uint8_t value = i < num ? HIGH : LOW;
    digitalWrite(channelOrder[i], value);
  }
}

void ELSequencer::lightWiresAtIndex(uint8_t index) {
  for (uint8_t i = 0; i < channelCount; i++) {
    uint8_t value = i == index ? HIGH : LOW;
    digitalWrite(channelOrder[i], value);
  }
}

void ELSequencer::lightNumWiresUpToWire(uint8_t num, uint8_t wireNum) {
  for (uint8_t i = 0; i < channelCount; i++) {
    uint8_t value = ((wireNum > i) && (i + num >= wireNum)) ? HIGH : LOW;
    digitalWrite(channelOrder[i], value);
  }
}

void ELSequencer::lightWiresByPattern(uint8_t pattern[]) {
  for (uint8_t i = 0; i < channelCount; i++) {
    uint8_t value = pattern[i] > 0 ? HIGH : LOW;
    digitalWrite(channelOrder[i], value);
  }
}

void ELSequencer::lightAll() {
  for (uint8_t i = 0; i < channelCount; i++) {
    digitalWrite(channelOrder[i], HIGH);
  }
}

void ELSequencer::lightNone() {
  for (uint8_t i = 0; i < channelCount; i++) {
    digitalWrite(channelOrder[i], LOW);
  }
}

void ELSequencer::lightRandomWires() {
  for (uint8_t i = 0; i < channelCount; i++) {
    uint8_t value = random(0, 2) > 0 ? HIGH : LOW;
    digitalWrite(channelOrder[i], value);
  }
}

void ELSequencer::initSequencer() {
  for (uint8_t i = 0; i < channelCount; i++) {
    pinMode(channelOrder[i], OUTPUT);
    digitalWrite(channelOrder[i], LOW);
  }
}

void ELSequencer::playWireStartSequence() {
  for (int i = 0; i <= channelCount; i++) {
    lightNumWires(i);
    delay(100);
  }
  for (int i = channelCount; i >= 0; i--) {
    lightNumWires(i);
    delay(100);
  }
  for (int i = 0; i < 10; i++) {
    lightNumWires(0);
    delay(50);
    lightNumWires(channelCount);
    delay(50);
  }
}