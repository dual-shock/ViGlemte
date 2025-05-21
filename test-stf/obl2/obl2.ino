#define buttonOnePin 2
#define buttonTwoPin 4 
#define buttonThreePin 7
#define buttonFourPin 8 

#define yellowLedPin 12
#define alternatingLedPin 13

class Pushbutton{
  public:
    uint8_t pin;
    uint8_t state;
    uint16_t prevMillis;
    Pushbutton(uint8_t pinParam){
      pin = pinParam;
      pinMode(pin, INPUT_PULLUP);
    }
    bool getDebounce(bool isPressed){
      uint16_t timeMillis = millis();
      switch(state){
        case 0:
          if(!isPressed){
            prevMillis = timeMillis;
            state = 1;
          }
          break;
        case 1:
          if(isPressed){
            state = 0; 
          }
          else if((uint16_t)  timeMillis - prevMillis >= 15){
            state = 2;
          }
          break;
        case 2:
          if(isPressed){
            prevMillis = timeMillis;
            state = 3;
          }
          break;
        case 3:
          if(!isPressed){
            state = 2; 
          }
          else if((uint16_t)timeMillis - prevMillis >= 15){
            state = 0;
            return true;
          }
          break;
      }
      return false;
    }
    bool isPressed(){
      return digitalRead(pin) != 1;
    }
};

Pushbutton buttonOne(buttonOnePin);
Pushbutton buttonTwo(buttonTwoPin);
Pushbutton buttonThree(buttonThreePin);
Pushbutton buttonFour(buttonFourPin);

class SequenceTracker{
  public: 
    int lastSequence[8] = {0,0,0,0,0,0,0,0}; //filled to make shifting easier
    int correctSequence[8] = {4,2,4,1,3,3,2,1};
  bool checkSequence(){
    if(memcmp(lastSequence, correctSequence, sizeof(correctSequence)) == 0){
      Serial.println("last sequence was a match, open!");
      digitalWrite(alternatingLedPin, HIGH);
    }
    else{
      Serial.println("last sequence was not a match, stay locked.");
      digitalWrite(alternatingLedPin, LOW);
    }
  }
  void addToSequence(int newNum){
    for(int i = 0; i < 7; i++){
      lastSequence[i] = lastSequence[i+1];
    }
    lastSequence[7] = newNum;
    checkSequence();
    Serial.println("4 2 4 1 3 3 2 1");
    printLastSequence();
    Serial.println();
  }
  void printLastSequence(){
    for(int i = 0; i < 8; i++){
      Serial.print(lastSequence[i]);
      Serial.print(" ");
    }
    Serial.print("\n");
  }
};


SequenceTracker sequenceTracker;

void setup()
{
Serial.begin(9600);
pinMode(alternatingLedPin, OUTPUT);
pinMode(yellowLedPin, OUTPUT);
Serial.println("current last sequence! press the four buttons to enter code.");
sequenceTracker.printLastSequence();
}

void loop()
{
  if(buttonOne.getDebounce(buttonOne.isPressed())){
    Serial.println("Button One Pressed");
    sequenceTracker.addToSequence(1);
  }
  if(buttonTwo.getDebounce(buttonTwo.isPressed())){
    Serial.println("Button Two Pressed");  
    sequenceTracker.addToSequence(2);
  }
  if(buttonThree.getDebounce(buttonThree.isPressed())){
    Serial.println("Button Three Pressed");  
    sequenceTracker.addToSequence(3);
  }
  if(buttonFour.getDebounce(buttonFour.isPressed())){
    Serial.println("Button Four Pressed");  
    sequenceTracker.addToSequence(4);
  }
  if(buttonOne.isPressed() || buttonTwo.isPressed() || buttonThree.isPressed() || buttonFour.isPressed()){
    digitalWrite(yellowLedPin, HIGH);
  }
  else{
    digitalWrite(yellowLedPin, LOW);
  }
}