const int pResistor0 = A1; 
//const int pResistor1 = A1; 


int value0;	
//int value1;				  

void setup(){
  Serial.begin(9600);
  pinMode(pResistor0, INPUT);
  //pinMode(pResistor1, INPUT);
}


void loop(){
  value0 = analogRead(pResistor0);
  //value1 = analogRead(pResistor1);
  Serial.print("Photoresistor 0 value:");
  Serial.print(value0);
  Serial.println("\r");
}