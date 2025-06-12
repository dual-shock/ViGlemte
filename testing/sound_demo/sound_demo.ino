#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST 0

int alarm_melody[] = {
  NOTE_E4,4,  NOTE_E4,4,  NOTE_F4,4,  NOTE_G4,4,//1
  NOTE_G4,4,  NOTE_F4,4,  NOTE_E4,4,  NOTE_D4,4,
  NOTE_C4,4,  NOTE_C4,4,  NOTE_D4,4,  NOTE_E4,4,
  NOTE_E4,-4, REST,8,  REST,2,

  NOTE_E4,4,  NOTE_E4,4,  NOTE_F4,4,  NOTE_G4,4,//4
  NOTE_G4,4,  NOTE_F4,4,  NOTE_E4,4,  NOTE_D4,4,
  NOTE_C4,4,  NOTE_C4,4,  NOTE_D4,4,  NOTE_E4,4,
  NOTE_D4,-4,  REST,8,  REST,2,

  NOTE_D4,4,  NOTE_D4,4,  NOTE_E4,4,  NOTE_C4,4,//8
  NOTE_D4,4,  NOTE_E4,8,  NOTE_F4,8,  NOTE_E4,4, NOTE_C4,4,
  NOTE_D4,4,  NOTE_E4,8,  NOTE_F4,8,  NOTE_E4,4, NOTE_D4,4,
  NOTE_C4,4,  REST,4,  REST,2,

  NOTE_E4,4,  NOTE_E4,4,  NOTE_F4,4,  NOTE_G4,4,//12
  NOTE_G4,4,  NOTE_F4,4,  NOTE_E4,4,  NOTE_D4,4,
  NOTE_C4,4,  NOTE_C4,4,  NOTE_D4,4,  NOTE_E4,4,
  NOTE_D4,-4,  REST,8,  REST,2
};

int full_melody[] = {
  NOTE_E4,4,  NOTE_E4,4,  NOTE_F4,4,  NOTE_G4,4,//1
  NOTE_G4,4,  NOTE_F4,4,  NOTE_E4,4,  NOTE_D4,4,
  NOTE_C4,4,  NOTE_C4,4,  NOTE_D4,4,  NOTE_E4,4,
  NOTE_E4,-4, NOTE_D4,8,  NOTE_D4,2,

  NOTE_E4,4,  NOTE_E4,4,  NOTE_F4,4,  NOTE_G4,4,//4
  NOTE_G4,4,  NOTE_F4,4,  NOTE_E4,4,  NOTE_D4,4,
  NOTE_C4,4,  NOTE_C4,4,  NOTE_D4,4,  NOTE_E4,4,
  NOTE_D4,-4,  NOTE_C4,8,  NOTE_C4,2,

  NOTE_D4,4,  NOTE_D4,4,  NOTE_E4,4,  NOTE_C4,4,//8
  NOTE_D4,4,  NOTE_E4,8,  NOTE_F4,8,  NOTE_E4,4, NOTE_C4,4,
  NOTE_D4,4,  NOTE_E4,8,  NOTE_F4,8,  NOTE_E4,4, NOTE_D4,4,
  NOTE_C4,4,  NOTE_D4,4,  NOTE_G3,2,

  NOTE_E4,4,  NOTE_E4,4,  NOTE_F4,4,  NOTE_G4,4,//12
  NOTE_G4,4,  NOTE_F4,4,  NOTE_E4,4,  NOTE_D4,4,
  NOTE_C4,4,  NOTE_C4,4,  NOTE_D4,4,  NOTE_E4,4,
  NOTE_D4,-4,  NOTE_C4,8,  NOTE_C4,2
};


#include <Pushbutton.h>

#define BUTTON_PIN 13
#define LED_PIN 12
#define BUZZER_PIN 3

Pushbutton button(BUTTON_PIN);



unsigned long lastblink_time, time_since_last_blink;
int led_state = LOW;  
int time_between_blinks = 500;


unsigned long time_since_last_note, lastnote_time, time_between_notes;
int note_time, divider;
int note_counter = 0;
int note_time_counter = 1;

int tempo=228; 
int wholenote = (60000 * 4) / tempo;
int notes=sizeof(alarm_melody)/sizeof(alarm_melody[0])/2; 

bool pause_done = false;

bool run_alarm = false;


int state = 0; 

void setup() {
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(9600);
}


void loop() {
    unsigned long tot_time_passed = millis();
    if(button.getSingleDebouncedRelease()){
        if(state==0){ 
            state = 1; 
        }
        else if(state==1){
            state = 2; 
            digitalWrite(LED_PIN, LOW);
        }
        else{ 
            state = 0; 
            digitalWrite(LED_PIN, LOW);
            noTone(BUZZER_PIN);
        }
    }

    if(state==0){
      return;
    }
    else if(state==1){ 
        time_since_last_blink = tot_time_passed - lastblink_time;
        time_since_last_note = tot_time_passed - lastnote_time;
        if (time_since_last_blink > time_between_blinks) {
            lastblink_time = tot_time_passed;
            if (led_state == LOW) {
                led_state = HIGH;
            } 
            else {
                led_state = LOW;
            }
            digitalWrite(LED_PIN, led_state);
        }   

        if(time_since_last_note > (time_between_notes * 0.9)){
            noTone(BUZZER_PIN);
        }   
        if(time_since_last_note > time_between_notes){
            lastnote_time = tot_time_passed;
            divider = alarm_melody[note_time_counter];
            if (divider > 0) {
                time_between_notes = (wholenote) / divider;
            } 
            else if (divider < 0) {
                time_between_notes = (wholenote) / abs(divider);
                time_between_notes *= 1.5;
            }
            if(alarm_melody[note_counter] == REST){noTone(BUZZER_PIN);}
            else{tone(BUZZER_PIN, alarm_melody[note_counter]);}
            note_counter = note_counter + 2; 
            note_time_counter = note_time_counter + 2; 
            if(note_counter == 124){
                note_counter = 0;
                note_time_counter = 1; 
            }
        }
    }
    else if(state==2){
        time_since_last_note = tot_time_passed - lastnote_time;
        if(time_since_last_note > (time_between_notes * 0.9)){
            noTone(BUZZER_PIN);
        }   
        if(time_since_last_note > time_between_notes){
            lastnote_time = tot_time_passed;
            divider = full_melody[note_time_counter];
            if (divider > 0) {
                time_between_notes = (wholenote) / divider;
            } 
            else if (divider < 0) {
                time_between_notes = (wholenote) / abs(divider);
                time_between_notes *= 1.5;
            }
            if(full_melody[note_counter] == REST){noTone(BUZZER_PIN);}
            else{tone(BUZZER_PIN, full_melody[note_counter]);}
            note_counter = note_counter + 2; 
            note_time_counter = note_time_counter + 2; 
            if(note_counter == 32 || note_counter == 62 || note_counter == 96 || note_counter == 126){
              state = 0;
              note_counter = 0;
              note_time_counter = 1; 
              digitalWrite(LED_PIN, LOW);
              noTone(BUZZER_PIN);
            }
        }
        return;
    }
}
