
#include "LedControl.h"


LedControl lc = LedControl(2,4,3,1); // Data pin, Clock pin, CS pin, Number of devices

/* we always wait a bit between updates of the display */
unsigned long delaytime=1000;

void setup() {
   pinMode(2, OUTPUT); // Data pin
   pinMode(3, OUTPUT); // Clock pin
   pinMode(4, OUTPUT); // CS pin
  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,2);
  /* and clear the display */
  lc.clearDisplay(0);
  Serial.begin(9600);
}

void printNumber(int v) {
    int ones;
    int tens;
    int hundreds;
    boolean negative;	

    if(v < -999 || v > 999) 
       return;
    if(v<0) {
        negative=true;
        v=v*-1;
    }
    ones=v%10;
    v=v/10;
    tens=v%10;
    v=v/10;
    hundreds=v;			
    if(negative) {
       //print character '-' in the leftmost column	
       lc.setChar(0,3,'-',false);
    }
    else {
       //print a blank in the sign column
       lc.setChar(0,3,' ',false);
    }
    //Now print the number digit by digit
    lc.setDigit(0,2,(byte)hundreds,false);
    lc.setDigit(0,1,(byte)tens,false);
    lc.setDigit(0,0,(byte)ones,false);
}

void loop() { 
   int RowBits = B10000000;
   for(int i=0; i<8; i++) {
      lc.setRow(0,0,RowBits);
      RowBits = RowBits >> 1;
      delay(delaytime);
      Serial.println(RowBits);
      lc.clearDisplay(0);
   }
   
}
