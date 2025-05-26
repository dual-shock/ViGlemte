#include <Arduino.h>
#include <analogmuxdemux.h> //interfacer med CD4051 Mux
#include "SoftwareSerial.h" //setter opp seriell kommunikasjon med DFPlayer Mini
#include <DFPlayerMini_Fast.h>

class Luker {

  private:
    AnalogMux amux; 
    int sensorValues[7]; // Current sensor readings
    float avgValues[7] = {0}; // Moving average for each sensor
    bool triggered[7] = {false}; // Track if opening has been detected for each sensor
    const float alpha = 0.1; // Smoothing factor for moving average

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
    bool detectOpening(float threshold = 100.0) {
      bool anyTriggered = false;
      for (int i = 0; i < 7; i++) {
        int current = amux.AnalogRead(i);
        // Initialize moving average on first run
        if (avgValues[i] == 0) avgValues[i] = current;

        // Update moving average
        avgValues[i] = alpha * current + (1 - alpha) * avgValues[i];

        // Detect drastic increase
        if (!triggered[i] && (current - avgValues[i]) > threshold) {
          triggered[i] = true;
          anyTriggered = true; // At least one sensor triggered
        }

        // Reset trigger when light goes back down (hysteresis)
        if (triggered[i] && (current - avgValues[i]) < (threshold / 2)) {
          triggered[i] = false;
        }
      }
      return anyTriggered;
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

class Kalender {
    // representerer den interne kalenderen i pilleboksen
  private:
    unsigned long startTime; 
    unsigned long openings[100]; // 100 tider om gangen i kalenderen 
    int openingCount; 
    unsigned long dayStartTime;
    bool eventTriggered; 
    Alarm* alarm; 

  public:
    
    Kalender(unsigned long currentTime, Alarm* alarmObj) : startTime(currentTime), openingCount(0), dayStartTime(currentTime), eventTriggered(false), alarm(alarmObj) {}

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


    void printAapninger() {
      Serial.println("Åpningstider:");
      for (int i = 0; i < openingCount; i++) {
        Serial.print("Tid: ");
        Serial.println(openings[i]);
      }
    }

    // resetter 24 timers klokken for ryddighetens skyld
    void resetDag() {
      unsigned long currentTime = millis();
      if (currentTime - dayStartTime >= 86400000) { // 24 timer = 86400000 ms
        dayStartTime = currentTime;
        eventTriggered = false; 
        Serial.println("24 timer passert");
      }
    }

    // beregner et gjennomsnitt av åpningstidene og setter alarmen til det
    // her kommer sannsynglighet matte og sånt når den tid kommer
    void kalkulerAlarmTid() {
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

    // sjekker i loopen etter et tidspunkt og kjører en event på tidspunktet
    // bruker for å kjøre alarmen når tiden er inne
    void sjekkEvent(unsigned long eventTime, void (*eventFunction)()) {
      unsigned long currentTime = millis();
      unsigned long elapsedTime = currentTime - dayStartTime;

      if (elapsedTime >= eventTime && !eventTriggered) {
        eventTriggered = true;
        eventFunction(); 
      }
    }
};

// Pins
#define amux_a_pin 13
#define amux_b_pin 12
#define amux_c_pin 11

#define amux_com_pin 0 //analog pin A0

#define switch_1_pin 10 
#define switch_2_pin 9 

#define volume_control_pin A5

#define DFPlayer_TX_pin 3
#define DFPlayer_RX_pin 2


// normal variables
unsigned long tot_seconds, tot_minutes, tot_hours;
int display_seconds, display_minutes, display_hours;

SoftwareSerial DFPlayer_serial(DFPlayer_RX_pin, DFPlayer_TX_pin);
DFPlayerMini_Fast DFPlayer;
byte volume = 0;

Luker luker(amux_a_pin, amux_b_pin, amux_c_pin, amux_com_pin);

// prototype variables
float time_multiplier = 1000.0;





void convertTimes(unsigned long millisValue) {
  tot_seconds = millisValue * time_multiplier / 1000; 
  tot_minutes = tot_seconds / 60; 
  tot_hours = tot_minutes / 60; 

  display_hours = tot_hours % 24;
  display_minutes = tot_minutes % 60;
  display_seconds = tot_seconds % 60;
}

void updatePlayer(){
  volume = map(analogRead(volume_control_pin), 0, 1023, 0, 30);
  if(volume < 29){DFPlayer.volume(volume);}
}



void setup() {
  Serial.begin(9600);
  DFPlayer_serial.begin(9600); 
  DFPlayer.begin(DFPlayer_serial);
  DFPlayer.loop(2); 
}


void loop() {
  convertTimes(millis());
  updatePlayer();

  // Print volume with leading space if needed (always 2 chars)
  Serial.print("volume: ");
  if (volume < 10) Serial.print(" ");
  Serial.print("[");
  Serial.print(volume);
  Serial.print("]");

  Serial.print("  Clock: [");
  if (display_hours < 10) Serial.print(" ");
  Serial.print(display_hours);
  Serial.print(":");
  if (display_minutes < 10) Serial.print(" ");
  Serial.print(display_minutes);
  Serial.print(":");
  if (display_seconds < 10) Serial.print(" ");
  Serial.print(display_seconds);
  Serial.print("]");

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

  // Print time since last registered opening
  static unsigned long lastOpeningTime = 0;
  static bool firstRun = true;
  if (luker.detectOpening()) {
    lastOpeningTime = millis();
    firstRun = false;
  }
  Serial.print("  Time since last opening: ");
  if (firstRun) {
    Serial.print("N/A");
  } else {
    unsigned long elapsed = (millis() - lastOpeningTime) / 1000;
    Serial.print(elapsed);
    Serial.print("s");
  }

  Serial.print("\r");
}