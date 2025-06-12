

#include <Arduino.h>

class Luke {
    // Representerer lokket på pilleboksen med en fotoresistor for å oppdage tilstanden
  private:
    const int lightSensorPin; 
    const int threshold;      // terskel for fotoresistor state
    bool prevState;     

  public:
    
    Luke(int pin, int thresholdValue) : lightSensorPin(pin), threshold(thresholdValue), prevState(false) {
      pinMode(lightSensorPin, INPUT);
    }

    // boolsk variabel, representerer om lokket er åpent eller lukket
    bool Aapen() {
      int sensorValue = analogRead(lightSensorPin);
      return sensorValue > threshold; 
    }

    // registrer åpninger en og en 
    bool RegistrerAapning() {
      bool currState = Aapen();
      if (currState && !prevState) { 
        prevState = currState;
        return true;
      }
      prevState = currState; 
      return false; 
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


Luke pillboxLid(A0, 10); 
Alarm alarm; 
Kalender kalender(millis(), &alarm); 

void setup() {
  Serial.begin(9600);
}

void loop() {

  if (pillboxLid.RegistrerAapning()) {
    Serial.println("åpning registrert");
    kalender.leggTilAapning(); 
    kalender.kalkulerAlarmTid(); 
  }


  if (pillboxLid.Aapen()) {
    Serial.println("pilleboks åpen ");
  } else {
    Serial.println("pilleboks lukket");
  }

  kalender.resetDag();

  kalender.sjekkEvent(alarm.getAlarmTid(0), []() { alarm.skruPaa(); });

  delay(500); 
}