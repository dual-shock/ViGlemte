//? board 3rd up right side (3rd in array) increases 1st up left (1st in array)
  //? but ONLY when 1st up from left ISNT connected

//? board 4th up rigt does nothing ()

//? board 1st up left is accurate, but value goes to both 1 and 5 in array

//? when 3rd up right and 1st up left connected, both are accurate
   //? but 5th in array is same as 1st

//? when 2nd up left side is connected, it works, increasees 7th in array

//? when 3rd up left is connected, it works, increases 6th in array 



// 1, 3, 2, 0, 5



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
  Serial.println("Photoresistor 0 value: \r");
  Serial.print(value0);
}

