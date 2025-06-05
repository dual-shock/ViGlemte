const int dataPin = A1;
const int clockPin = 5;
const int latchPin = 6;

void setup() {
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);

  updateLEDs(B11111110);  // All bits ON except Q0
}

void loop() {
  // Do nothing
}

void updateLEDs(byte state) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, state);
  digitalWrite(latchPin, HIGH);
}