int dataPin = A2;
int clockPin = A3;
int latchPin = A4;

void updateLEDs(byte state) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, state);
  digitalWrite(latchPin, HIGH);
}

void setup() {
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
}

void loop() {
  updateLEDs(B00000010);
  delay(1000);

  // Turn OFF all (including Q1)
  updateLEDs(B00000000);
  delay(1000);
}
