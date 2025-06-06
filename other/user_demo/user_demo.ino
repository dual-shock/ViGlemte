#include <Arduino.h>
#include <analogmuxdemux.h> 
#include "SoftwareSerial.h" 
#include <DFPlayerMini_Fast.h>
#include <TimerOne.h>
#include <Wire.h> 
#include "RTClib.h"

//sensor 0 (sunday)
// and 6 (thursday) 
// are accurate, but lower than rest, making them not always register


// * classes
class Luker {
  private:
    AnalogMux amux; 
    int sensorValues[7]; 
    float avgValues[7] = {0}; 
    bool triggered[7] = {false}; 
    const float alpha = 0.025; 

  public:
    Luker(int amux_a, int amux_b, int amux_c, int amux_com)
      : amux(amux_a, amux_b, amux_c, amux_com) {}
    int* sensors() {
      for (int i = 0; i < 7; i++) {
        sensorValues[i] = amux.AnalogRead(i);
      }
      return sensorValues;
    }
    
    byte detectOpening(float threshold = 100.0) {
      byte triggeredMask = 0;
      for (int i = 0; i < 7; i++) {
        int current = amux.AnalogRead(i);
        
        //if(i == 4){int current=amux.AnalogRead(7);}
        
        if (avgValues[i] == 0) avgValues[i] = current;
        avgValues[i] = alpha * current + (1 - alpha) * avgValues[i];
        if (!triggered[i] && (current - avgValues[i]) > threshold) {
          triggered[i] = true;
          triggeredMask |= (1 << i);
        }
        if (triggered[i] && (current - avgValues[i]) < (threshold / 2)) {
          triggered[i] = false;
        }
      }
      return triggeredMask;
    }
};



//  * Pins
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





//  * normal variables
// ...existing code...
volatile int ledWave_pos = 0;
volatile int ledWave_dir = 1;
volatile bool ledWave_active = false;
const int ledWave_steps = 8;
const unsigned long ledWave_interval = 100; // ms between steps

volatile unsigned long ledWave_lastUpdate = 0; // for timing inside ISR
const int ledWave_start = 1; // Q1
const int ledWave_end = 7;   // Q7



RTC_DS3231 rtc;
unsigned long rtc_start_time_in_secs = 0;

unsigned long tot_days, tot_seconds, tot_minutes, tot_hours;
int display_seconds, display_minutes, display_hours;

SoftwareSerial DFPlayer_serial(DFPlayer_RX_pin, DFPlayer_TX_pin);
DFPlayerMini_Fast DFPlayer;
byte volume = 0;
int old_volume = 0; 

Luker luker(amux_a_pin, amux_b_pin, amux_c_pin, amux_com_pin);

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



//  * prototyping variables
float time_multiplier = 150;
bool    rint_realtime = false, print_openings = true, print_sensors = true, 
        print_clock = false, print_volume = true; 
int doses_per_day = 1;

    // * Demo Variables
bool alarm_playing = false;

bool day1_opened = false;
int day1_open_hour = 0;
int day1_open_minutes = 0;

bool day2_opened = false;
int day2_open_hour = 0;
int day2_open_minutes = 0;
bool day2_play_alarm = false;

bool day3_opened = false;
int day3_open_hour = 0;
int day3_open_minutes = 0;
bool day3_play_alarm = false;

int mid_hour = 0;
int mid_min = 0;

bool update_player = true; 



// * Functions
void displayTime(int hours, int minutes) {
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

  //byte ledBits = 0b00000000;

  digitalWrite(latchPin, LOW);
    //shiftOut(dataPin, clockPin, MSBFIRST, ledBits);
    shiftOut(dataPin, clockPin, MSBFIRST, m2);
    shiftOut(dataPin, clockPin, MSBFIRST, segs | m1bit7);
  digitalWrite(latchPin, HIGH);

  current = (current + 1) & 0x03;
  delay(0);
}

void convertTimes(unsigned long millisValue) {
  tot_seconds = rtc_start_time_in_secs + (millisValue * time_multiplier / 1000); // add start time in millis to the total seconds
  tot_minutes = tot_seconds / 60; 
  tot_hours = tot_minutes / 60; 
  tot_days = tot_hours / 24; 
  display_hours = tot_hours % 24;
  display_minutes = tot_minutes % 60;
  display_seconds = tot_seconds % 60;
}

void updatePlayer(){
  volume = volume = map(analogRead(volume_pin), 0, 1023, 0, 30);
  if(volume < 29){
    if (old_volume != volume) {
      DFPlayer.volume(volume);
      Serial.print("\033[2K\r");
      Serial.print("Volume set to: ");
      Serial.println(volume);
      old_volume = volume;
    }
  }
}

void refreshDisplay() {
  displayTime(display_hours, display_minutes);
}

void updateLEDs(byte state) {
  digitalWrite(ledLatchPin, LOW);
  shiftOut(ledDataPin, ledClockPin, MSBFIRST, state);
  digitalWrite(ledLatchPin, HIGH);
}

void ledWaveTask() {
  if (!ledWave_active) return;
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





void setup() {
  Serial.begin(9600);
  DFPlayer_serial.begin(9600); 
  DFPlayer.begin(DFPlayer_serial);
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin , OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(ledClockPin, OUTPUT);
  pinMode(ledDataPin, OUTPUT);
  pinMode(ledLatchPin, OUTPUT);
  updateLEDs(B11111110);

  DFPlayer.loop(2);

  Timer1.initialize(2000);
  Timer1.attachInterrupt(refreshDisplay);
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  DateTime now = rtc.now();
  rtc_start_time_in_secs = now.hour() * 3600UL + now.minute() * 60UL + now.second();
  startLedWave();
}



void loop() {
    ledWaveTask();

    // * looping values
    DateTime now = rtc.now();
    convertTimes(millis());
    
    byte triggeredMask = luker.detectOpening();

    if(update_player){updatePlayer();}

    // * swtich detections
    static int prev_switch_1 = -1;
    static int prev_switch_2 = -1;
    int switch_1 = digitalRead(switch_1_pin);
    int switch_2 = digitalRead(switch_2_pin);

    if (prev_switch_1 == -1) prev_switch_1 = switch_1; 
    if (prev_switch_2 == -1) prev_switch_2 = switch_2;
    if (switch_1 != prev_switch_1) {
        Serial.print("Switch 1 (sound) toggled: ");
        Serial.println(switch_1 == HIGH ? "ON" : "OFF");
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
        Serial.print("Switch 2 (lights) (unused) toggled: ");
        Serial.println(switch_2 == HIGH ? "ON" : "OFF");
        prev_switch_2 = switch_2;
    }


    // * Sensor printing
    if(print_sensors){
      Serial.print("  Luker: [");
      int* sensors = luker.sensors();
      for (int i = 0; i < 7; i++) {
        if (sensors[i] < 10) {
          Serial.print("  ");
        } else if (sensors[i] < 100) {
          Serial.print(" ");
        }
        Serial.print(sensors[i]);
        if (i < 6) Serial.print(",");
      }
      Serial.print("]");
          
    }
    if (print_openings) {
      for (int i = 0; i < 7; i++) {
        if (triggeredMask & (1 << i)) {
          // Calculate weekday (0=Monday, 6=Sunday)
          int weekday = tot_days % 7;
          // Save opening time in seconds since midnight
          unsigned long opening_time = display_hours * 3600 + display_minutes * 60 + display_seconds;

          Serial.print("\033[2K\r");
          Serial.print("Sensor ");
          Serial.print(i);
          Serial.print(" detected an opening at time [");
          if (display_hours < 10) Serial.print("0");
          Serial.print(display_hours);
          Serial.print(":");
          if (display_minutes < 10) Serial.print("0");
          Serial.print(display_minutes);
          Serial.print(":");
          if (display_seconds < 10) Serial.print("0");
          Serial.print(display_seconds);
          Serial.println("]");
          break;
        }
      }
    }



    // * demo logic
    if (!day1_opened && tot_days == 0) {
        if (triggeredMask & (1 << 5)) {
            day1_opened = true;
            day1_open_hour = display_hours;
            day1_open_minutes = display_minutes;
            Serial.print("Day 1 (Sensor 5) opened at [");
            Serial.print(display_hours);
            Serial.print(":");
            Serial.print(display_minutes);
            Serial.print(":");
            Serial.print(display_seconds);
            Serial.print("]");
            Serial.println("  no alarm will play today. ");
            day1_opened = true;
        }
    }

    if (day1_opened && !day2_opened) {
        if(tot_days == 1){
            if(display_hours >= day1_open_hour && display_minutes >= day1_open_minutes) {
                if(day2_opened == false){
                    day2_play_alarm = true;
                }
            }
        }
        if (day2_play_alarm && !alarm_playing && tot_days == 1) {
            Serial.print("day 2 alarm playing! at: [");
            Serial.print(display_hours);
            Serial.print(":");
            Serial.print(display_minutes);
            Serial.print(":");
            Serial.print(display_seconds);
            Serial.print("]");
            Serial.println("  waiting for day 2 box lid (sensor 3) to open.");
            DFPlayer.loop(2);
            alarm_playing = true;
        }
        if ((triggeredMask & (1 << 3)) && alarm_playing) {
            DFPlayer.stop();
            Serial.print("day 2 sensor (3) detected opening, alarm stopped! at: [");
            Serial.print(display_hours);
            Serial.print(":");
            Serial.print(display_minutes);
            Serial.print(":");
            Serial.print(display_seconds);
            Serial.print("]");
            day2_open_hour = display_hours;
            day2_open_minutes = display_minutes;
            int total1 = day1_open_hour * 60 + day1_open_minutes;
            int total2 = day2_open_hour * 60 + day2_open_minutes;
            if (total2 < total1) {
              total2 += 24 * 60;
            }
            int mid = (total1 + total2) / 2;
            mid = mid % (24 * 60); 
            mid_hour = mid / 60;
            mid_min = mid % 60;
            Serial.print("  day 3 alarm will play at: [");
            Serial.print(mid_hour);
            Serial.print(":");
            Serial.print(mid_min);
            Serial.println("]");
            
            alarm_playing = false;
            day2_opened = true;

        }
    }

    if (day2_opened && !day3_opened) {
        if(tot_days == 2){
            if(display_hours >= mid_hour && display_minutes >= mid_min) {
                if(day3_opened == false){
                    day3_play_alarm = true;
                }
            }
        }
        if (day3_play_alarm && !alarm_playing && tot_days == 2) {
            Serial.print("day 3 alarm playing! at: [");
            Serial.print(display_hours);
            Serial.print(":");
            Serial.print(display_minutes);
            Serial.print(":");
            Serial.print(display_seconds);
            Serial.print("]");
            Serial.println("  waiting for day 3 box lid (sensor 2) to open.");
            DFPlayer.loop(2);
            alarm_playing = true;
        }
        if ((triggeredMask & (1 << 2)) && alarm_playing) {
            DFPlayer.stop();
            Serial.print("day 3 sensor (2) detected opening, alarm stopped! at: [");
            Serial.print(display_hours);
            Serial.print(":");
            Serial.print(display_minutes);
            Serial.print(":");
            Serial.print(display_seconds);
            Serial.print("]");
            Serial.println("  3 day demo is now finished. to reset, unplug/reset the arduino");
            alarm_playing = false;
            day3_opened = true;
        }
    }


    Serial.print("\r");
}



