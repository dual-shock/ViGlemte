/* 
* ViGlemte project 

*/

#include <Arduino.h>
#include <analogmuxdemux.h> 
#include <SoftwareSerial.h> 
#include <DFPlayerMini_Fast.h>
#include <TimerOne.h>
#include <Wire.h> 
#include <RTClib.h>

// * Comments explained! (using BetterComments extension)

// * Titles / headers / subtitles, for structure
// ! Important notices, integral unfinished code needed for the program to work 
// TODO things / features / fixes to be implemented, not integral but improves quality
// ? explanations of things that need explaining, like functions or params
// general text







// TODO sensor 0 and 6 are slightly lower on average than the rest,
// TODO perhaps make the lids class individually listen to each sensor


//! THE ONLY ASSUMPTION this program makes is that the User powers on / resets arduino before taking their first
//! dose of their first day, and takes the amount of doses during that first day 
//! that they will take every next day


// * // * Pins
#define amux_a_pin 9
#define amux_b_pin 10
#define amux_c_pin 11

#define amux_com_pin A0 //analog pin A0

#define volume_pin A3 //analog pin A3

#define switch_1_pin 7
#define switch_2_pin 8

#define latchPin 3
#define dataPin 4
#define clockPin 2

#define ledDataPin A1
#define ledClockPin 6
#define ledLatchPin 5

#define DFPlayer_TX_pin 12
#define DFPlayer_RX_pin 13




// * // * classes
class Luker {
  private:
    AnalogMux amux; 
    int sensorValues[7]; 
    float avgValues[7] = {0}; 
    bool triggered[7] = {false}; 
    const float alpha = 0.5; 

  public:
    int sensors_indexed_by_day[7] = {3, 4, 1, 0, 5, 6, 7};
    // ? generally, this program prefers using the RTCLibs DateTime class
    // ? and its subsequent .dayOfTheWeek() method, the respective sensors
    // ? are therefore indexed using sensors_indexed_by_day[DateTime.dayOfTheWeek()]

    Luker(int amux_a, int amux_b, int amux_c, int amux_com)
      : amux(amux_a, amux_b, amux_c, amux_com) {}
    int* sensors() {
      for (int i = 0; i < 7; i++) {
        sensorValues[i] = amux.AnalogRead(i);
      }
      return sensorValues;
    }
    
    byte openingsByte(float threshold = 100.0) { 
      // returns a byte where each bit represents a sensor
      // X bit == 1, means X sensor has been triggered

      // is used by checking the byte each iteration of a loop
      byte triggered_byte = 0;
      for (int i = 0; i < 7; i++) {
        int current = amux.AnalogRead(i);
        if (avgValues[i] == 0) avgValues[i] = current;
        avgValues[i] = alpha * current + (1 - alpha) * avgValues[i];
        if (!triggered[i] && (current - avgValues[i]) > threshold) {
          triggered[i] = true;
          triggered_byte |= (1 << i);
        }
        if (triggered[i] && (current - avgValues[i]) < (threshold / 2)) {
          triggered[i] = false;
        }
      }
      return triggered_byte;
    }

    bool listenForOpening(byte triggered_byte, int day_index){
      // returns true if the the sensor indexed by the day_index 
      // arg is triggered in the triggered_byte

      // this uses a triggered byte as a parameter instead of just calling it because
      // its assumed that the byte will be called each loop, and multiple calls
      // will lead to skipped openings 

      // yes this could be mediated by creating another function just for 
      // checking one sensor, but i prefer having a single function

      if (triggered_byte) {
          if (triggered_byte & (1 << sensors_indexed_by_day[day_index])) {
            return true;
          }
          else {
            return false;
          }
      }
      return false;
    }
};

struct Opening {
  DateTime time; // time of the opening
  bool valid = true; // if a user doesnt take their dose for a day, well add a non valid opening
  // this will allow the AllOpenings class to iterate and calculate openings
  // for each day even if a user forgets an opening one day, EDGE CASE
  Opening() : time() {} // a default constructor so the list of all openings can be initialized
  Opening(DateTime time) : time(time) {}
  int getDay(){
    return time.dayOfTheWeek();
  }
};

class AllOpenings {
  public: 
    static const int max_openings = 100;
    int dose_count;
    Opening openings[max_openings]; // the number here is the maximum amount of openings tracked at a time
    int head = 0; 
    int tail = 0; 
    int count = 0; 
    AllOpenings() : dose_count(0) {} // default constructor for the case where no doses are taken
    AllOpenings(int dose_count) : dose_count(dose_count) {}

    void save(Opening opening){
      if(count == max_openings){
        head = (head + 1) % max_openings; // move head forward
      }else{count++;}
      openings[tail] = opening;
      tail = (tail + 1) % max_openings; // move tail forward
    }

    void printQueue() {
      for (int i = 0; i < count; i++) {
        int index = (head + i) % max_openings; // calculate the index in a circular manner
        Opening opening = openings[index];
      }
    }

};




// * // * variables

// * lids / sensor openings
AllOpenings all_openings;
Luker luker(amux_a_pin, amux_b_pin, amux_c_pin, amux_com_pin);
const int max_doses_per_day = 3; // maximum amount of doses per day, can be changed later

// * RTC / clock display 
RTC_DS3231 rtc;
DateTime now; 
const byte segmentMap[10] = {
  0b00111111, // 0
  0b00000110, // 1
  0b01011011, // 2
  0b01001111, // 3
  0b01100110, // 4
  0b01101101, // 5
  0b01111101, // 6
  0b00000111, // 7
  0b01111111, // 8
  0b01101111  // 9
};
const byte digit1MaskBits[4] = {
  0b00000000, // index 0 → digit1 ON (bit7 = 0)
  0b10000000, // index 1 → digit1 OFF (bit7 = 1)
  0b10000000, // index 2 → digit1 OFF
  0b10000000  // index 3 → digit1 OFF
};
const byte digitMask2[4] = {
  0b11111111, // digit 1 OFF
  0b11111110, // digit 2 ON (bit 0 = 0)
  0b11111101, // digit 3 ON (bit 1 = 0)
  0b11111011  // digit 4 ON (bit 2 = 0)
};

// * DFPlayer and loudspeaker
SoftwareSerial DFPlayer_serial(DFPlayer_RX_pin, DFPlayer_TX_pin);
DFPlayerMini_Fast DFPlayer;
int volume = 0;
int old_volume = 0; 
bool update_player = true; 

// * LED row 
volatile int ledWave_pos = 0;
volatile int ledWave_dir = 1;
volatile bool ledWave_active = false;
const int ledWave_steps = 8;
const unsigned long ledWave_interval = 100; // ms between steps
volatile unsigned long ledWave_lastUpdate = 0; // for timing inside ISR
const int ledWave_start = 1; // Q1
const int ledWave_end = 7;   // Q7
bool leds_on = true; 




// * // * Functions

// * lids / sensor openings

// * RTC / clock display
void updateClock() {
  int hours = now.hour(); 
  int minutes = now.minute();

  uint8_t d0 = (hours   / 10) % 10;  
  uint8_t d1 = (hours   % 10);       
  uint8_t d2 = (minutes / 10) % 10;  
  uint8_t d3 = (minutes % 10);       

  static uint8_t current = 0;

  uint8_t val   = (current == 0 ? d0
                  : current == 1 ? d1
                  : current == 2 ? d2
                  : d3);

  byte segs   = segmentMap[val];
  byte m1bit7 = digit1MaskBits[current];

  const byte COLON_BIT = (1 << 3);
  byte m2 = digitMask2[current] | COLON_BIT;

  digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, m2);
    shiftOut(dataPin, clockPin, MSBFIRST, segs | m1bit7);
  digitalWrite(latchPin, HIGH);

  current = (current + 1) & 0x03;
}

// * DFPlayer and loudspeaker
void updatePlayer(){
  volume = volume = map(analogRead(volume_pin), 0, 1023, 0, 30);
  if(volume < 29){
    if (old_volume != volume) {
      DFPlayer.volume(volume);
      old_volume = volume;
    }
  }
}

// * LED row
void updateLEDs(byte state) {
  digitalWrite(ledLatchPin, LOW);
  shiftOut(ledDataPin, ledClockPin, MSBFIRST, state);
  digitalWrite(ledLatchPin, HIGH);
}
void ledWaveTask() {
  if (!ledWave_active){ return; updateLEDs(0); }
  unsigned long now = millis();
  if (now - ledWave_lastUpdate >= ledWave_interval) {
    updateLEDs(1 << ledWave_pos);

    ledWave_pos += ledWave_dir;
    if (ledWave_pos > ledWave_end) {
      ledWave_pos = ledWave_end - 1;
      ledWave_dir = -1;
    } else if (ledWave_pos < ledWave_start) {
      ledWave_pos = ledWave_start + 1;
      ledWave_dir = 1;
    }
    ledWave_lastUpdate = now;
  }
}
void startLedWave() {
  ledWave_active = true;
  ledWave_pos = ledWave_start;
  ledWave_dir = 1;
  ledWave_lastUpdate = millis();
}
void stopLedWave() {
  ledWave_active = false;
  updateLEDs(0);
}




// * // * Setup
void setup() {
  


  pinMode(latchPin, OUTPUT);
  pinMode(dataPin , OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(ledClockPin, OUTPUT);
  pinMode(ledDataPin, OUTPUT);
  pinMode(ledLatchPin, OUTPUT);
  

  Serial.begin(9600);
  DFPlayer_serial.begin(9600); 
  if(!DFPlayer.begin(DFPlayer_serial)) {
      Serial.println("DFPlayer Mini not found!");
      Serial.flush();
  }


  Wire.begin();
  if(!rtc.begin()) {
      Serial.println("RTC not found!");
      Serial.flush();
  }
  if(rtc.lostPower()) {
      // if rtc lost power, set the time to compile time
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  rtc.disable32K();

  now = rtc.now();
  DateTime start_time = now; 

  Timer1.initialize(2000);
  Timer1.attachInterrupt(updateClock);


  
  // ? Ensure things arent running
  stopLedWave();
  DFPlayer.stop();
  if(digitalRead(switch_1_pin) == LOW){
      DFPlayer.volume(0); // turn all sound OFF 
      update_player = false; // global status bool that updates sound OFF
  }else{
      volume = map(analogRead(volume_pin), 0, 1023, 0, 30); 
      DFPlayer.volume(volume); // turn sound ON 
      update_player = true;} // global status bool that updates sound ON, where sound gets turned on

  if(digitalRead(switch_2_pin) == LOW){
      updateLEDs(B00000000); // turn all LEDs off 
      leds_on = false;  // global status bool that updates LEDs OFF
  }else{
      leds_on = true; } // global status bool that updates LEDs ON, where LEDs get turned on

  

  bool turn_on_condition = true;
  while(turn_on_condition){ //TODO make this condition tied to a button or something
    // wait until condition met, i.e box "turned on"
  } 
  
  bool first_day_over = false;
  int day_to_check = start_time.dayOfTheWeek(); // the config day to check, indexed by sensor
  


  Opening first_day_openings[max_doses_per_day]; 
  // the number here is the maxmimum amount of doses per day
  // this is where the first doses of the first config day are stored

  int dose_count = 0; 
  while(!first_day_over){ // while we're on the first day, only runs once as its in setup()
    
    //TODO add a check to see if the openings are attached to ALL openings, 
    //TODO if the sensor after the opening of the respecive day is opened within 
    //TODO 5 minutes of the first opening, disregard that first opening
    
    now = rtc.now();


    bool day_sensor_opened = luker.listenForOpening(luker.openingsByte(), day_to_check);
    // bool if the sensor for the configuration day has detected an opening


    if(start_time.dayOfTheWeek() == now.dayOfTheWeek()){ // first day hasnt passed
      if(day_sensor_opened){
        first_day_openings[dose_count] = Opening(now);
        dose_count++;
        if(dose_count == max_doses_per_day){ // maximum amount of doses per day reached
          first_day_over = true;
        }
      }
    }
    else if(start_time.dayOfTheWeek() != now.dayOfTheWeek()){ // if a day has passed
      if(dose_count == 0){ // but no individual doses counted
        start_time = now; 
        day_to_check = start_time.dayOfTheWeek();// no individual openings detected, no data to work on, so reset 
        // start time so we stay on the config day
      }
      else if(dose_count > 0){
        // the first day is over, and we have doses to work with
        first_day_over = true; // set the first day as over
      }
    }
  }

  all_openings = AllOpenings(dose_count); // initialize the all openings class with the max doses per day
  for(int i = 0; i < dose_count; i++){
    all_openings.save(first_day_openings[i]); // save the first day openings to the all openings class
  }


}

//? After setup, we are left with a global all_openinings oject
//? with an array of the first openings, matching the count of dose_count


void loop() {
    

    // * looping values
    now = rtc.now();

    
    byte triggeredMask = luker.openingsByte();

    if(update_player){updatePlayer();}
    if(leds_on){ledWaveTask();}

    // * swtich detections
    static int prev_switch_1 = -1;
    static int prev_switch_2 = -1;
    int switch_1 = digitalRead(switch_1_pin);
    int switch_2 = digitalRead(switch_2_pin);

    if (prev_switch_1 == -1) prev_switch_1 = switch_1; 
    if (prev_switch_2 == -1) prev_switch_2 = switch_2;
    if (switch_1 != prev_switch_1) {
        prev_switch_1 = switch_1;
        if(switch_1 == LOW){
            DFPlayer.volume(0);
            update_player = false;
        }
        else{
            volume = map(analogRead(volume_pin), 0, 1023, 0, 30);
            update_player = true;
            DFPlayer.volume(volume);
        }

    }
    if (switch_2 != prev_switch_2) {
        prev_switch_2 = switch_2;
        if(switch_2 == LOW){
            updateLEDs(B00000000);
            leds_on = false;           
        }
        else{
            leds_on = true; 

        }
    }


}



