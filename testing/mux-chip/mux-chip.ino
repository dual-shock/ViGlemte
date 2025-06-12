// MUX CD4051 
// A = pin 11 = S0  -->   Digital pin 13 
// B = pin 10 = S1  -->   Digital pin 12 
// C = pin 9 = S2  -->   Digital pin 11 

#include <analogmuxdemux.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

SoftwareSerial mySerial(7, 8); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

AnalogMux amux(13, 12, 11, 0);
int value0;
int value1;
int value2;
int value3;
int value4;
int value5;
int value6; 



void setup(){
    Serial.begin(9600);
    pinMode(A0, INPUT);
    mySerial.begin(9600);
    if (!myDFPlayer.begin(mySerial)) {
        Serial.println("DFPlayer Mini not detected!");
        
    }
    myDFPlayer.volume(15);
    myDFPlayer.play(2); 
}

int switch_1_state;
int switch_2_state;
void loop(){
    value0 = amux.AnalogRead(0);
    value1 = amux.AnalogRead(1);
    value2 = amux.AnalogRead(2);
    value3 = amux.AnalogRead(3);
    value4 = amux.AnalogRead(4);
    value5 = amux.AnalogRead(5);
    value6 = amux.AnalogRead(6);
    
    Serial.print("Pin 0:\t");
    Serial.print(value0);
    Serial.print("\t");
    Serial.print("Pin 1:\t");
    Serial.print(value1);
    Serial.print("\t");
    Serial.print("Pin 2:\t");
    Serial.print(value2);
    Serial.print("\t");
    Serial.print("Pin 3:\t");
    Serial.print(value3);
    Serial.print("\t"); 
    Serial.print("Pin 4:\t");
    Serial.print(value4);
    Serial.print("\t");
    Serial.print("Pin 5:\t");
    Serial.print(value5);
    Serial.print("\t");
    Serial.print("Pin 6:\t");
    Serial.print(value6);
    Serial.print("\t");

    switch_1_state = digitalRead(10); // Read the state of switch 1 connected to pin 13
    Serial.print("Switch 1 state:\t");
    if (switch_1_state == HIGH) {
        Serial.print("ON");
    } else {
        Serial.print("OFF");
    }
    switch_2_state = digitalRead(9); // Read the state of switch 1 connected to pin 13
    Serial.print("\tSwitch 2 state:\t");
    if (switch_2_state == HIGH) {
        Serial.print("ON");
    } else {
        Serial.print("OFF");
    }


    Serial.print("\n");



}