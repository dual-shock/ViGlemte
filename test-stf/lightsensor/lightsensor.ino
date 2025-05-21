// CIRCUIT
// 5v -> fotoresistor female
// fotoresistor male -> 5k Ohm Male
// 5k ohm Male -> A0
// 5k Ohm female -> GND


//Constants
const int pResistor = A0; // Photoresistor at Arduino analog pin A0

//Variables
int value;				  // Store value from photoresistor (0-1023)  

void setup(){
  Serial.begin(9600);

 pinMode(pResistor, INPUT);// Set pResistor - A0 pin as an input (optional)
}

void loop(){
  value = analogRead(pResistor);
  Serial.print("Photoresistor value: ");
  Serial.print(value);
  Serial.print("\t\t");
  if (value < 10){
    Serial.print("box is closed");
  }
  else{
    Serial.print("box is open");
  }
  
  Serial.print("\n");
}



// 1   600 -> 250
// 2   730 -> 270
// 3   210 -> 30   REMOVED
// 4   380 -> 100  REMOVED
// 5   460 -> 80   REMOVED
// 6   730 -> 250
// 7   700 -> 290
// 8   900 -> 600
// 9   630 -> 140
// 10  720 -> 360