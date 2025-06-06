const int dataPin = A1;
const int clockPin = 6;
const int latchPin = 5;

void setup() {
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);

  updateLEDs(B11111110);  // All bits ON except Q0
}

void waveLEDs(int delayTime = 100) {
  // Forward wave: Q1 to Q7
  for (int i = 1; i <= 7; i++) {
    updateLEDs(1 << i);
    delay(delayTime);
  }

  // Reverse wave: Q6 back to Q1
  for (int i = 6; i >= 1; i--) {
    updateLEDs(1 << i);
    delay(delayTime);
  }
}

void loop() {
 //waveLEDs(100);  // Repeat wave forever
}

void updateLEDs(byte state) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, state);
  digitalWrite(latchPin, HIGH);
}