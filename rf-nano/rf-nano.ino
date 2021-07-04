#define DEBUG 1
#define DEBUG_MEMORY 1

// Radio
#include "RF24.h"
#define CE_PIN 9
#define CSN_PIN 10
#ifdef DEBUG
#include "printf.h"
#endif
const RF24 radio(CE_PIN, CSN_PIN);
const uint64_t* senderAddress = 0x1010101010;
const uint64_t* receiverAddress = 0x2020202020;

// Display
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#define OLED_RESET -1
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define TEXT_SIZE SCREEN_HEIGHT / 32
#define LETTER_WIDTH TEXT_SIZE * 6
#define LETTER_HEIGHT TEXT_SIZE * 8
#define LINE_1_Y LETTER_HEIGHT / 4
#define LINE_2_Y LETTER_HEIGHT / 2 + LETTER_HEIGHT
#define LINE_3_Y (LETTER_HEIGHT / 4) * 3 + LETTER_HEIGHT * 2
const Adafruit_SSD1306 display;

// Rotary encoder
#include "RotaryEncoder.h"
#include "YetAnotherPcInt.h"
#define RTRY_ENC_SW A1
#define RTRY_ENC_CLK A2
#define RTRY_ENC_DT A3
const RotaryEncoder encoder(RTRY_ENC_CLK, RTRY_ENC_DT);
void onSwitch();
void onRotate();

// Microphone
#define MIC_OUT A0
#define MIC_SAMPLE_WINDOW 20 // ms
#define MIC_BASELINE 393

typedef struct MenuItem {
  String name;
  byte id = 0;
  int value = -1;
  MenuItem* prevSibling;
  MenuItem* nextSibling;
  MenuItem* currentChild;
};

MenuItem autoModes;
#define AUTO_MODES 1
MenuItem manualModes;
#define MANUAL_MODES 2
MenuItem settings;
#define SETTINGS 3
MenuItem stats;
#define STATS 4

MenuItem peakAverage;
#define PEAK_AVERAGE 11
MenuItem peakToPeak;
#define PEAK_TO_PEAK 12
MenuItem rms;
#define RMS 13

MenuItem peakAvgEnable;
#define PEAK_AVG_ENABLE 111
MenuItem peakAvgHigh;
#define PEAK_AVG_HIGH 112
MenuItem peakAvgLow;
#define PEAK_AVG_LOW 113

MenuItem peakToPeakEnable;
#define PEAK_TO_PEAK_ENABLE 121
MenuItem peakToPeakHigh;
#define PEAK_TO_PEAK_HIGH 122
MenuItem peakToPeakLow;
#define PEAK_TO_PEAK_LOW 123

MenuItem rmsEnable;
#define RMS_ENABLE 131
MenuItem rmsHigh;
#define RMS_HIGH 132
MenuItem rmsLow;
#define RMS_LOW 133

MenuItem resetStats;
#define RESET_STATS 41

MenuItem* currentItem;
#define MAX_NUM_ACTIVE_ITEMS 3
byte numActiveItems = MAX_NUM_ACTIVE_ITEMS;
MenuItem* activeItems[MAX_NUM_ACTIVE_ITEMS];
MenuItem* activeMode;

void initMenu() {
  // top level
  autoModes.name = F("Auto");
  autoModes.id = AUTO_MODES;
  autoModes.prevSibling = &settings;
  autoModes.nextSibling = &manualModes;
  autoModes.currentChild = &peakAverage;

  manualModes.name = F("Manual");
  manualModes.id = MANUAL_MODES;
  manualModes.prevSibling = &autoModes;
  manualModes.nextSibling = &stats;

  stats.name = F("Stats");
  stats.id = STATS;
  stats.prevSibling = &manualModes;
  stats.nextSibling = &settings;

  settings.name = F("Config");
  settings.id = SETTINGS;
  settings.prevSibling = &stats;
  settings.nextSibling = &autoModes;
  settings.currentChild = &resetStats;

  // auto modes
  peakAverage.name = F("PAvg");
  peakAverage.id = PEAK_AVERAGE;
  peakAverage.prevSibling = &rms;
  peakAverage.nextSibling = &peakToPeak;
  peakAverage.currentChild = &peakAvgEnable;

  peakToPeak.name = F("P2P");
  peakToPeak.id = PEAK_TO_PEAK;
  peakToPeak.prevSibling = &peakAverage;
  peakToPeak.nextSibling = &rms;
  peakToPeak.currentChild = &peakToPeakEnable;

  rms.name = F("RMS");
  rms.id = RMS;
  rms.prevSibling = &peakToPeak;
  rms.nextSibling = &peakAverage;
  rms.currentChild = &rmsEnable;

  // peak average
  peakAvgEnable.name = F("enable");
  peakAvgEnable.id = PEAK_AVG_ENABLE;
  peakAvgEnable.prevSibling = &peakAvgLow;
  peakAvgEnable.nextSibling = &peakAvgHigh;

  peakAvgHigh.name = F("Hi");
  peakAvgHigh.id = PEAK_AVG_HIGH;
  peakAvgHigh.value = 100;
  peakAvgHigh.prevSibling = &peakAvgEnable;
  peakAvgHigh.nextSibling = &peakAvgLow;

  peakAvgLow.name = F("Low");
  peakAvgLow.id = PEAK_AVG_LOW;
  peakAvgLow.value = 40;
  peakAvgLow.prevSibling = &peakAvgHigh;
  peakAvgLow.nextSibling = &peakAvgEnable;

  // peak to peak
  peakToPeakEnable.name = F("enable");
  peakToPeakEnable.id = PEAK_TO_PEAK_ENABLE;
  peakToPeakEnable.prevSibling = &peakToPeakLow;
  peakToPeakEnable.nextSibling = &peakToPeakHigh;

  peakToPeakHigh.name = F("Hi");
  peakToPeakHigh.id = PEAK_TO_PEAK_HIGH;
  peakToPeakHigh.value = 500;
  peakToPeakHigh.prevSibling = &peakToPeakEnable;
  peakToPeakHigh.nextSibling = &peakToPeakLow;

  peakToPeakLow.name = F("Low");
  peakToPeakLow.id = PEAK_TO_PEAK_LOW;
  peakToPeakLow.value = 20;
  peakToPeakLow.prevSibling = &peakToPeakHigh;
  peakToPeakLow.nextSibling = &peakToPeakEnable;

  // rms
  rmsEnable.name = F("enable");
  rmsEnable.id = RMS_ENABLE;
  rmsEnable.prevSibling = &rmsLow;
  rmsEnable.nextSibling = &rmsHigh;

  rmsHigh.name = F("Hi");
  rmsHigh.id = RMS_HIGH;
  rmsHigh.value = 120;
  rmsHigh.prevSibling = &rmsEnable;
  rmsHigh.nextSibling = &rmsLow;

  rmsLow.name = F("Low");
  rmsLow.id = RMS_LOW;
  rmsLow.value = 0;
  rmsLow.prevSibling = &rmsHigh;
  rmsLow.nextSibling = &rmsEnable;

  // settings
  resetStats.name = F("reset");
  resetStats.id = RESET_STATS;
  resetStats.prevSibling = &resetStats;
  resetStats.nextSibling = &resetStats;
}

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
  initMicrophone();
  initEncoder();
  initMenu();
  enablePeakAvg();
  initDisplay();
}

boolean hasInputs = false;
boolean switchPressed = false;
int maxValue = 0;
int minValue = 1024;

struct RadioData {
  int value;
  int low;
  int high;
} radioData;

boolean editing = false;

void loop() {
  if (hasInputs == true) {
    hasInputs = false;
    handleButtonInputs();
  }

  switch (activeMode->id) {
    case PEAK_AVG_ENABLE:
      radioData.value = samplePeakAverage();
      radioData.low = peakAvgLow.value;
      radioData.high = peakAvgHigh.value;
      break;
    case PEAK_TO_PEAK:
      radioData.value = samplePeakToPeak();
      radioData.low = peakToPeakLow.value;
      radioData.high = peakToPeakHigh.value;
      break;
    case RMS:
      radioData.value = sampleRootMeanSquares();
      radioData.low = rmsLow.value;
      radioData.high = rmsHigh.value;
      break;
  }

  minValue = min(radioData.value, minValue);
  maxValue = max(radioData.value, maxValue);

  //sendRadioSignal();
}

void handleButtonInputs() {
  if (switchPressed) {
    switchPressed = false;
    if (editing) {
      editing = false;
      renderCurrentItem();
    } else if (!editing && currentItem->value > -1) {
      editing = true;
      renderCurrentItem();
    } else if (!editing) {
      switch (currentItem->id) {
        case AUTO_MODES:
        case SETTINGS:
        case PEAK_AVERAGE:
        case PEAK_TO_PEAK:
        case RMS:
          currentItem = currentItem->currentChild;
          renderMenu();
          break;
        case STATS:
          renderStats();
          break;
        case PEAK_AVG_ENABLE:
          enablePeakAvg();
          renderMenu();
          break;
        case PEAK_TO_PEAK_ENABLE:
          enablePeakToPeak();
          renderMenu();
          break;
        case RMS_ENABLE:
          enableRms();
          renderMenu();
          break;
        case RESET_STATS:
          minValue = 1024;
          maxValue = 0;
          currentItem = &settings;
          renderMenu();
          break;
      }
    }
  } else {

    switch (encoder.getDirection()) {
      case RotaryEncoder::Direction::CLOCKWISE:
#ifdef DEBUG
        Serial.println(F("cw"));
#endif
        if (editing) {
          currentItem->value = min(currentItem->value + 1, 999);
          renderCurrentItem();
        } else {
          switch (currentItem->id) {
            default:
              currentItem = currentItem->nextSibling;
              renderMenu();
          }
        }
        break;
      case RotaryEncoder::Direction::COUNTERCLOCKWISE:
#ifdef DEBUG
        Serial.println(F("ccw"));
#endif
        if (editing) {
          currentItem->value = max(currentItem->value - 1, 0);
          renderCurrentItem();
        } else {
          switch (currentItem->id) {
            default:
              currentItem = currentItem->prevSibling;
              renderMenu();
          }
        }
        break;
      case RotaryEncoder::Direction::NOROTATION:
#ifdef DEBUG
        Serial.println(F("nr"));
#endif
        // ?
        break;
    }
  }
}

boolean isItemActive(MenuItem* item) {
  boolean result = false;
  for (int i = 0; i < numActiveItems; i++) {
    if (activeItems[i] == item) {
      result = true;
      break;
    }
  }
  return result;
}

void enablePeakAvg() {
  activeMode = &peakAverage;
  currentItem = &autoModes;
  activeItems[0] = &autoModes;
  activeItems[1] = &peakAverage;
  activeItems[2] = &peakAvgEnable;
  numActiveItems = 3;
  minValue = 1024;
  maxValue = 0;
}

void enablePeakToPeak() {
  activeMode = &peakToPeak;
  currentItem = &autoModes;
  activeItems[0] = &autoModes;
  activeItems[1] = &peakToPeak;
  activeItems[2] = &peakToPeakEnable;
  numActiveItems = 3;
  minValue = 1024;
  maxValue = 0;
}

void enableRms() {
  activeMode = &rms;
  currentItem = &autoModes;
  activeItems[0] = &autoModes;
  activeItems[1] = &rms;
  activeItems[2] = &rmsEnable;
  numActiveItems = 3;
  minValue = 1024;
  maxValue = 0;
}

void sendRadioSignal() {
  radio.openWritingPipe(senderAddress);
  radio.openReadingPipe(1, receiverAddress);
  radio.setPayloadSize(sizeof(radioData));
  radio.stopListening();
  bool success = radio.write(&radioData, sizeof(radioData));
  //radio.startListening();
#ifdef DEBUG
  if (!success) {
    Serial.println("radio error");
  }
#endif
}

int samplePeakToPeak() {
  unsigned long startMillis = millis();
  unsigned int signalMax = 0;
  unsigned int signalMin = 1023;
  while (millis() - startMillis < MIC_SAMPLE_WINDOW) {
    int sample = analogRead(MIC_OUT);
    if (sample > signalMax) {
      signalMax = sample;
    } else if (sample < signalMin) {
      signalMin = sample;
    }
  }
  return signalMax - signalMin;
}

int samplePeakAverage() {
  unsigned long startMillis = millis();
  unsigned int numSamples = 0;
  int sum = 0;
  while (millis() - startMillis < MIC_SAMPLE_WINDOW) {
    int sample = analogRead(MIC_OUT) - MIC_BASELINE;
    sample = abs(sample);
    numSamples++;
    sum += sample;
  }
  int value = round(sum / numSamples);
  return value;
}

int sampleRootMeanSquares() {
  unsigned long startMillis = millis();
  unsigned long sumOfSquares = 0;
  unsigned int numSamples = 0;
  while (millis() - startMillis < MIC_SAMPLE_WINDOW) {
    int sample = analogRead(MIC_OUT) - MIC_BASELINE;
    numSamples++;
    sumOfSquares += sq(sample);
  }
  int avg = sumOfSquares / numSamples;
  int value = sqrt(avg);
  return round(value);
}

void renderStats() {
  display.clearDisplay();
  display.setTextColor(1);
  display.setCursor(0, 0);
  display.print(F("Mode: "));
  display.println(activeItems[numActiveItems - 2]->name);
  display.print(F("min: "));
  display.println(minValue);
  display.print("max: ");
  display.println(maxValue);
  display.display();
}

void renderMenu() {
  display.clearDisplay();
  printItem(currentItem->prevSibling, LINE_1_Y, false);
#ifdef DEBUG_MEMORY
  Serial.println(freeMemory());
#endif
  printItem(currentItem, LINE_2_Y, true);
#ifdef DEBUG_MEMORY
  Serial.println(freeMemory());
#endif
  printItem(currentItem->nextSibling, LINE_3_Y, false);
#ifdef DEBUG_MEMORY
  Serial.println(freeMemory());
#endif
  display.display();
}

void renderCurrentItem() {
  printItem(currentItem, LINE_2_Y, true);
  display.display();
}

void printItem(MenuItem* item, byte y, boolean cursor) {
  byte x = LETTER_WIDTH * 2;
  byte width = item->name.length() * LETTER_WIDTH + (LETTER_WIDTH - 1) * TEXT_SIZE;
  display.fillRect(0, y - 1, SCREEN_WIDTH, LETTER_HEIGHT + 2, 0);
  display.setCursor(x, y);
  if (cursor) {
    if (editing) {
      byte xe = SCREEN_WIDTH - ((int)(log10(currentItem->value)) + 3) * LETTER_WIDTH;
      display.fillTriangle(xe, LINE_2_Y + LETTER_HEIGHT - 1, xe, LINE_2_Y + 1, xe + LETTER_WIDTH, LINE_2_Y + LETTER_HEIGHT / 2, 1);
    } else {
      display.fillTriangle(0, y + LETTER_HEIGHT - 1, 0, y + 1, LETTER_WIDTH, y + LETTER_HEIGHT / 2, 1);
    }
  }
  if (isItemActive(item)) {
    display.setTextColor(0, 1);
    display.fillRect(x - TEXT_SIZE, y - 1, item->name.length() * LETTER_WIDTH + TEXT_SIZE * 2, LETTER_HEIGHT + 2, 1);
  } else {
    display.setTextColor(1, 0);
  }
  display.setCursor(x, y);
  display.print(item->name);
  if (item->value > -1) {
    x = SCREEN_WIDTH - ((int)log10(item->value) + 1) * LETTER_WIDTH;
    display.setCursor(x, y);
    display.print(item->value);
  }
}

void onSwitch() {
#ifdef DEBUG_MEMORY
  Serial.println(freeMemory());
#endif
  hasInputs = true;
  switchPressed = true;
}

void onRotate() {
  hasInputs = true;
  encoder.tick();
#ifdef DEBUG_MEMORY
  Serial.println(freeMemory());
#endif
}

void initMicrophone() {
  analogReference(EXTERNAL);
  analogRead(MIC_OUT);
}

void initEncoder() {
  pinMode(RTRY_ENC_SW, INPUT_PULLUP);
  PcInt::attachInterrupt(RTRY_ENC_SW, onSwitch, FALLING);
  PcInt::attachInterrupt(RTRY_ENC_CLK, onRotate, CHANGE);
  PcInt::attachInterrupt(RTRY_ENC_DT, onRotate, CHANGE);
}

void initDisplay() {
  display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(TEXT_SIZE);
  display.setTextColor(1);
#ifdef DEBUG
  display.setCursor(0, LINE_1_Y);
  display.print(F("Line 1"));
  display.setCursor(0, LINE_2_Y);
  display.print(F("Line 2"));
  display.setCursor(0, LINE_3_Y);
  display.print(F("Line 3"));
  display.drawLine(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, 1);
  display.drawLine(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 1, 0, 1);
#endif
  display.display();
}

// https://learn.adafruit.com/memories-of-an-arduino/measuring-free-memory
#ifdef DEBUG_MEMORY
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
#endif
