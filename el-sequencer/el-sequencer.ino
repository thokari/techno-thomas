#include "timer.h"

Timer timer;

void setup() {
  PCICR |= (1 << PCIE1);
  PCMSK1 |= (1 << PCINT10); // PIN A2

  pinMode(2, OUTPUT);  // channel A  
  pinMode(3, OUTPUT);  // channel B   
  pinMode(4, OUTPUT);  // channel C
  pinMode(5, OUTPUT);  // channel D    
  pinMode(6, OUTPUT);  // channel E
  pinMode(7, OUTPUT);  // channel F
  pinMode(8, OUTPUT);  // channel G
  pinMode(9, OUTPUT);  // channel H

  pinMode(13, OUTPUT);
  Serial.begin(9600);
}

byte mode = 0; // 0 = PeakToPeak, 1 = RootMeanSquares, 2 = AveragePeak, 3 = manual
int maxInputsByMode[] = { 400, 75, 75, 450 };
int value = 0;
int xValue = 0;
int yValue = 0;
boolean buttonPressed = false;

ISR(PCINT1_vect) {
  static unsigned long lastTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastTime > 100) {
    if (digitalRead(A2) == HIGH) {
      buttonPressed = true;
    }
  }
  lastTime = currentTime;
}

# define JOYSTICK_Y A3
# define JOYSTICK_X A4

void turnOffLed() {
  digitalWrite(13, LOW);
}

void loop() {
  timer.update();
  if (buttonPressed) {
    timer.setCallback(turnOffLed);
    timer.setTimeout(200);
    timer.start();
    buttonPressed = false;
    digitalWrite(13, HIGH);
    mode = (mode + 1) % 4;
  }
  xValue = analogRead(JOYSTICK_X);
  yValue = analogRead(JOYSTICK_Y);
  
  if (mode == 0) {
    value = getPeakToPeak();
  } else if (mode == 1) {
    value = getRootMeanSquares();
  } else if (mode == 2) {
    value = getAveragePeak();
  } else if (mode == 3) {
    value = yValue - 500;
  }
  if (value >= maxInputsByMode[mode]) {
    digitalWrite(13, HIGH);
  } else {
    if (!buttonPressed) {
      digitalWrite(13, LOW);  
    }
  }
  Serial.print(maxInputsByMode[mode]);
  Serial.print(",");
  Serial.print(value);
  Serial.print(",");
  Serial.print(xValue, DEC);
  Serial.print(",");
  Serial.println(yValue, DEC);
}

#define SAMPLE_WINDOW 20 // ms
#define MIKE_OUT A7

int getPeakToPeak() {
  unsigned long startMillis = millis();
  unsigned int signalMax = 0;
  unsigned int signalMin = 1023;
  while (millis() - startMillis < SAMPLE_WINDOW) {
    int sample = analogRead(MIKE_OUT);
    if (sample > signalMax) {
      signalMax = sample;
    } else if (sample < signalMin) {
      signalMin = sample;
    }
  }
  return signalMax - signalMin;
}

#define BASELINE 382

int getRootMeanSquares() {
  unsigned long startMillis = millis();
  unsigned long sumOfSquares = 0;
  unsigned int numSamples = 0;
  while (millis() - startMillis < SAMPLE_WINDOW) {
    int sample = analogRead(MIKE_OUT) - BASELINE;
    numSamples++;
    sumOfSquares += sq(sample);
  }
  int value = round(sqrt(sumOfSquares / numSamples));
  return value;
}

int getAveragePeak() {
  unsigned long startMillis = millis();
  unsigned int numSamples = 0;
  int sum = 0;
  while (millis() - startMillis < SAMPLE_WINDOW) {
    int sample = abs(analogRead(MIKE_OUT) - BASELINE);
    numSamples++;
    sum += sample;
  }
  int value = round(sum / numSamples);
  return value;
}
