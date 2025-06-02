#include <Arduino.h>
#include <analogmuxdemux.h> //interfacer med CD4051 Mux
#include "SoftwareSerial.h" //setter opp seriell kommunikasjon med DFPlayer Mini
#include <DFPlayerMini_Fast.h>
#include <TimerOne.h>

#include <Wire.h> // I2C library for RTC

#include "RTClib.h"


class Luker {

  private:
    AnalogMux amux; 
    int sensorValues[7]; // Current sensor readings
    float avgValues[7] = {0}; // Moving average for each sensor
    bool triggered[7] = {false}; // Track if opening has been detected for each sensor
    const float alpha = 0.025; // Smoothing factor for moving average

  public:
    
    Luker(int amux_a, int amux_b, int amux_c, int amux_com)
      : amux(amux_a, amux_b, amux_c, amux_com) {}

    // Update and return the sensor values
    int* sensors() {
      for (int i = 0; i < 7; i++) {
        sensorValues[i] = amux.AnalogRead(i);
      }
      return sensorValues;
    }

    // Detects a drastic increase in light for any sensor (returns true if any triggered)
    byte detectOpening(float threshold = 100.0) {
      byte triggeredMask = 0;
      for (int i = 0; i < 7; i++) {
        int current = amux.AnalogRead(i);
        // Initialize moving average on first run
        if (avgValues[i] == 0) avgValues[i] = current;

        // Update moving average
        avgValues[i] = alpha * current + (1 - alpha) * avgValues[i];

        // Detect drastic increase
        if (!triggered[i] && (current - avgValues[i]) > threshold) {
          triggered[i] = true;
          triggeredMask |= (1 << i);
        }

        // Reset trigger when light goes back down (hysteresis)
        if (triggered[i] && (current - avgValues[i]) < (threshold / 2)) {
          triggered[i] = false;
        }
      }
      return triggeredMask;
    }
    int readPotentiometer() {
        return amux.AnalogRead(7); // Channel 7 for potentiometer
    }
};

class Alarm {
    // representerer "alarmen" med mulighet for 1 til 3 doser per dag satt opp 
    // når objektet opprettes
  private:
    int antallDoser; 
    unsigned long alarmTid[3]; // opp til 3 alarm tider (doser)

  public:
    
    Alarm() : antallDoser(1) {
      alarmTid[0] = 3600000; // default alarm tid er 1 time etter oppstart
    }

    
    void skruPaa() {
        // her skrus alarmen på
      Serial.println("Alarm på");
      
    }

    

    // setter + getter for alarmtid variabelene
    void setAlarmTid(int index, unsigned long time) {
      if (index < antallDoser) {
        alarmTid[index] = time;
      }
    }
    unsigned long getAlarmTid(int index) {
      if (index < antallDoser) {
        return alarmTid[index];
      }
      return 0;
    }
};

struct DayOpenings {
    unsigned long doses[2]; // 2 doses per day
    int count = 0;
};

class Kalender {
  private:
    DayOpenings week[7]; // 0=Monday, ..., 6=Sunday
    unsigned long averages[7][2]; // [weekday][dose]
    unsigned long startTime; 
    unsigned long openings[100]; // 100 tider om gangen i kalenderen 
    int openingCount; 
    unsigned long dayStartTime;
    bool eventTriggered; 
    Alarm* alarm; 

  public:
    
    Kalender(unsigned long currentTime, Alarm* alarmObj) : startTime(currentTime), openingCount(0), dayStartTime(currentTime), eventTriggered(false), alarm(alarmObj) {
      for (int i = 0; i < 7; ++i) {
        week[i].count = 0;
        averages[i][0] = 0;
        averages[i][1] = 0;
      }
    }

    // legger til en registrert åpning og lagrer tidspunktet i arrayet
    void leggTilAapning() {
      if (openingCount < 100) {
        unsigned long currentTime = millis() - startTime; 
        openings[openingCount] = currentTime; 
        openingCount++;
      } else {
        Serial.println("maks antall åpninger nådd");
      }
    }

    // Add an opening for a given weekday (0=Monday)
    void addOpening(int weekday, unsigned long time) {
      if (week[weekday].count < 2) {
        week[weekday].doses[week[weekday].count++] = time;
      } else {
        // Shift doses to make room for new one (FIFO)
        week[weekday].doses[0] = week[weekday].doses[1];
        week[weekday].doses[1] = time;
      }
      calculateNewAverage(weekday);
    }

    // beregner et gjennomsnitt av åpningstidene og setter alarmen til det
    // her kommer sannsynglighet matte og sånt når den tid kommer
    void kalkulertAlarmTid() {
      if (openingCount == 0) {
        return; 
      }

      unsigned long sum = 0;
      for (int i = 0; i < openingCount; i++) {
        sum += openings[i];
      }
      unsigned long averageTime = sum / openingCount;

      
      alarm->setAlarmTid(0, averageTime);
      Serial.print("kalkulert alarm tid: ");
      Serial.println(averageTime);
    }

    // Calculate average for the given weekday
    void calculateNewAverage(int weekday) {
      for (int dose = 0; dose < 2; ++dose) {
        unsigned long sum = 0;
        int n = 0;
        if (week[weekday].count > dose) {
          sum += week[weekday].doses[dose];
          n++;
        }
        averages[weekday][dose] = (n > 0) ? (sum / n) : 0;
      }
    }

    // Get average for a given weekday and dose
    unsigned long getAverage(int weekday, int dose) {
      return averages[weekday][dose];
    }
};

// Pins
#define amux_a_pin 9
#define amux_b_pin 10
#define amux_c_pin 11

#define amux_com_pin 0 //analog pin A0

#define switch_1_pin 7
#define switch_2_pin 8

#define latchPin 3
#define dataPin 4
#define clockPin 2

#define DFPlayer_TX_pin 12
#define DFPlayer_RX_pin 13


// normal variables






RTC_DS3231 rtc;
unsigned long rtc_start_time_in_secs = 0; // Will be set in setup()

unsigned long tot_days, tot_seconds, tot_minutes, tot_hours;
int display_seconds, display_minutes, display_hours;

SoftwareSerial DFPlayer_serial(DFPlayer_RX_pin, DFPlayer_TX_pin);
DFPlayerMini_Fast DFPlayer;
byte volume = 0;

Luker luker(amux_a_pin, amux_b_pin, amux_c_pin, amux_com_pin);
Kalender kalender(millis(), new Alarm()); // Create Kalender object with current time and a new Alarm object

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

// Digit-select masks (active LOW):
// reg1 controls digit 1 via bit 7; reg2 controls digits 2–4 via bits 0–2

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


// prototype variables
float time_multiplier = 150;

//int start_time_in_secs = 32400;

bool print_realtime = false, print_openings = false, print_sensors = false, print_clock = false, print_volume = false; 

int doses_per_day = 2;


// --- Pillbox demo
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

bool alarm_playing = false;






void displayTime(int hours, int minutes) {
  // split into 4 display digits, with leading zero
  uint8_t d0 = (hours   / 10) % 10;  // tens of hours
  uint8_t d1 = (hours   % 10);       // ones of hours
  uint8_t d2 = (minutes / 10) % 10;  // tens of minutes
  uint8_t d3 = (minutes % 10);       // ones of minutes

  static uint8_t current = 0;

  // pick the right value for this cycle
  uint8_t val   = (current == 0 ? d0
                  : current == 1 ? d1
                  : current == 2 ? d2
                  : /* =3 */      d3);

  byte segs   = segmentMap[val];
  byte m1bit7 = digit1MaskBits[current];

  const byte COLON_BIT = (1 << 3);
  byte m2 = digitMask2[current] | COLON_BIT;

  digitalWrite(latchPin, LOW);
    // second 595 (digits 2–4)
    shiftOut(dataPin, clockPin, MSBFIRST, m2);
    // first 595 (segments in bits 0–6, bit 7=enable for digit 1)
    shiftOut(dataPin, clockPin, MSBFIRST, segs | m1bit7);
  digitalWrite(latchPin, HIGH);

  current = (current + 1) & 0x03;
  delay(0);
}

void convertTimes(unsigned long millisValue) {
  tot_seconds = rtc_start_time_in_secs + (millisValue * time_multiplier / 1000); // add start time in millis to the total seconds
  tot_minutes = tot_seconds / 60; 
  tot_hours = tot_minutes / 60; 
  tot_days = tot_hours / 24; // calculate the number of days
  display_hours = tot_hours % 24;
  display_minutes = tot_minutes % 60;
  display_seconds = tot_seconds % 60;
}

int old_volume = 0; // for volume control

void updatePlayer(){
  //volume = map(analogRead(volume_control_pin), 0, 1023, 0, 30);
  volume = map(luker.readPotentiometer(), 0, 1023, 0, 30);;

  if(volume < 29){
    if (old_volume != volume) {
      DFPlayer.volume(volume);
      Serial.print("Volume set to: ");
      Serial.println(volume);
      old_volume = volume;
    }
  }
}

void refreshDisplay() {
  // hours and minutes should be in globals that you update once/60s
  displayTime(display_hours, display_minutes);
}

void setup() {
  Serial.begin(9600);
  DFPlayer_serial.begin(9600); 
  DFPlayer.begin(DFPlayer_serial);
  //DFPlayer.volume(20);
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin , OUTPUT);
  pinMode(clockPin, OUTPUT);
  Timer1.initialize(2000);
  Timer1.attachInterrupt(refreshDisplay);

  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Set the start time from RTC
  DateTime now = rtc.now();
  rtc_start_time_in_secs = now.hour() * 3600UL + now.minute() * 60UL + now.second();
}


void loop() {
  DateTime now = rtc.now();
  if(print_realtime){
    Serial.print(now.year(), DEC);
    Serial.print("-");
    Serial.print(now.month(), DEC);
    Serial.print("-");
    Serial.print(now.day(), DEC);
    Serial.print("-");
    Serial.print(now.hour(), DEC);
    Serial.print("-");
    Serial.print(now.minute(), DEC);
    Serial.print("-");
    Serial.print(now.second(), DEC);
    Serial.print(" / ");
  }


  convertTimes(millis());
  updatePlayer();
  byte triggeredMask = luker.detectOpening();
  // --- Switch event detection ---
  static int prev_switch_1 = -1;
  static int prev_switch_2 = -1;
  int switch_1 = digitalRead(switch_1_pin);
  int switch_2 = digitalRead(switch_2_pin);
  if (prev_switch_1 == -1) prev_switch_1 = switch_1; // Initialize on first run
  if (prev_switch_2 == -1) prev_switch_2 = switch_2;
  
  if (switch_1 != prev_switch_1) {
    Serial.print("\033[2K\r");
    Serial.print("Switch 1 toggled: ");
    Serial.println(switch_1 == HIGH ? "ON" : "OFF");
    prev_switch_1 = switch_1;
  }
  if (switch_2 != prev_switch_2) {
    Serial.print("\033[2K\r");
    Serial.print("Switch 2 toggled: ");
    Serial.println(switch_2 == HIGH ? "ON" : "OFF");
    prev_switch_2 = switch_2;
  }

 
  if (triggeredMask) {
    for (int i = 0; i < 7; i++) {
      if (triggeredMask & (1 << i)) {
        // Calculate weekday (0=Monday, 6=Sunday)
        int weekday = tot_days % 7;
        // Save opening time in seconds since midnight
        unsigned long opening_time = display_hours * 3600 + display_minutes * 60 + display_seconds;
        kalender.addOpening(weekday, opening_time);

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

  if(print_volume){

    
    Serial.print("Volume: ");
    Serial.print("[");
    if (volume < 10) Serial.print(" ");
    Serial.print(volume);
    Serial.print("]");
  }

  if(print_clock){
    Serial.print("  Clock: [");
    if (tot_days < 10) Serial.print(" ");
    Serial.print(tot_days);
    Serial.print(":");
    if (display_hours < 10) Serial.print(" ");
    Serial.print(display_hours);
    Serial.print(":");
    if (display_minutes < 10) Serial.print(" ");
    Serial.print(display_minutes);
    Serial.print(":");
    if (display_seconds < 10) Serial.print(" ");
    Serial.print(display_seconds);
    Serial.print("]");    
  }

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

  //Serial.print("\r"); // Keeps the status line updating in place (if you want to keep it)







// --- Pillbox demo logic ---

// 1. Detect opening of day 1 box (sensor 5) on day 0
if (!day1_opened && tot_days == 0) {
    //byte triggeredMask = luker.detectOpening();
    if (triggeredMask & (1 << 5)) {
        day1_opened = true;
        day1_open_hour = display_hours;
        day1_open_minutes = display_minutes;
        Serial.print("\033[2K\r");
        Serial.print("Day 1 (Sensor 5) opened at [");
        if (display_hours < 10) Serial.print("0");
        Serial.print(display_hours);
        Serial.print(":");
        if (display_minutes < 10) Serial.print("0");
        Serial.print(display_minutes);
        Serial.print(":");
        if (display_seconds < 10) Serial.print("0");
        Serial.print(display_seconds);
        Serial.println("]");
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
        Serial.println();
        Serial.print("ALARM: Playing sound for today's dose! at ");
        Serial.print(display_hours);
        Serial.print(":");
        Serial.print(display_minutes);
        DFPlayer.loop(2);
        alarm_playing = true;
    }


    //byte triggeredMask = luker.detectOpening();
    if ((triggeredMask & (1 << 3)) && alarm_playing) {
      Serial.println();
        Serial.print("ALARM STOPPED: Box opened for day 2 (Sensor 3). at: ");
        Serial.print(display_hours);
        Serial.print(":");
        Serial.print(display_minutes);
        DFPlayer.stop();
        alarm_playing = false;
        day2_opened = true;
          day2_open_hour = display_hours; // Set the alarm time to the same as day 1 opening
          day2_open_minutes = display_minutes;
    }
}



if (day2_opened && !day3_opened) {

    if(tot_days == 2){
      if(display_hours >= (day2_open_hour + day1_open_hour)/2 && display_minutes >= (day2_open_minutes + day1_open_minutes)/2) {
        if(day3_opened == false){
          day3_play_alarm = true;
        }
      }
    }
    
    if (day3_play_alarm && !alarm_playing && tot_days == 2) {
      Serial.println();
        Serial.print("ALARM: Playing sound for today's dose! at");
        Serial.print(display_hours);
        Serial.print(":");
        Serial.print(display_minutes);
        DFPlayer.play(2);
        alarm_playing = true;
    }


    //byte triggeredMask = luker.detectOpening();
    if ((triggeredMask & (1 << 2)) && alarm_playing) {
      Serial.println();
        Serial.print("ALARM STOPPED: Box opened for day 3 (Sensor 5). at ");
        Serial.print(display_hours);
        Serial.print(":");
        Serial.print(display_minutes);
        DFPlayer.stop();
        alarm_playing = false;
        day3_opened = true;
    }
}
}