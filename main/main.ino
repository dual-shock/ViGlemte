/*
* ViGlemte project
* NB: Mesteparten av kommenteringen og koden generelt er her skrevet på engelsk
* da hovedansvarlig for programmet foretrekker dette for enklere variabelnavn,
* dokumentering, lesbarhet, etc etc.

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
// TODO things / features / fixes to be implemented, not integral but QoL
// ? explanations of things that need explaining, like functions or params
// ? @param, @return, etc, parameters and return values for methods/funcs 
// general text


// TODO sensor 0 and 6 are slightly lower on average than the rest,
// TODO perhaps make the lids class individually map out averages for each sensor
// TODO to make the openingsByte method slightly more accurate, not needed 
// TODO from our testing though

//! the only assumption this program, and our prototype in general, makes is
//! that the User powers on / resets the arduino before taking their first
//! dose of their first day, and takes the amount of doses during that first day
//! that they will take every next day

// * // * Pins
#define amux_a_pin 9
#define amux_b_pin 10
#define amux_c_pin 11
#define amux_com_pin A0

#define volume_pin A3

#define switch_1_pin 7
#define switch_2_pin 8

#define clock_latch_pin 3
#define clock_data_pin 4
#define clock_clock_pin 2

#define led_latch_pin 5
#define led_data_pin A1
#define led_clock_pin 6

#define DFPlayer_TX_pin 12
#define DFPlayer_RX_pin 13
//? im hoping the pin naming together with the schematics provided
//? makes it intuitive to understand what each pin does

// * // * classes
class Luker
{

private:
	AnalogMux amux; //? creates an AnalogMux object to read the sensors
					//? of the analog multiplexer (CD4051)

	int sensor_values[7];		 //? most recent sensor values
	float avg_values[7] = {0};	 //? holder for average sensor values
	bool triggered[7] = {false}; //? map og triggers of sensors
	const float alpha = 0.5;	 //? the "smoothing" factor of the averages

public:
	int sensors_indexed_by_day[7] = {3, 4, 1, 0, 5, 6, 2};
	// ? generally, this program prefers using the RTCLibs DateTime class
	// ? and its subsequent .dayOfTheWeek() method, the respective sensors
	// ? are therefore indexed using sensors_indexed_by_day[DateTime.dayOfTheWeek()]

	/*
	* Luker class constructor
	? @param amux_a, amux_b, amux_c, amux_com: the pins used by the AnalogMux */
	Luker(int amux_a, int amux_b, int amux_c, int amux_com)
		: amux(amux_a, amux_b, amux_c, amux_com) {}

	/*
	* sensors() method for getting sensor values
	? @return: an array of the most recent sensor values */
	int *sensors()
	{
		for (int i = 0; i < 7; i++)//? for each sensor pin
		{ 
			sensor_values[i] = amux.AnalogRead(i);
		}
		return sensor_values;
	}

	/*
	* openingsByte method for getting sensor openings
	? @return: an array of the most recent opening values in a byte
		? each bit represents one sensor, so if triggered_byte[2] is 1
		? it means the 3rd sensor has been triggered*/
	byte openingsByte(float threshold = 100.0)
	{
		byte triggered_byte = 0;
		for (int i = 0; i < 7; i++)
		{
			int current = amux.AnalogRead(i);
			if (avg_values[i] == 0)
				avg_values[i] = current;
			avg_values[i] = alpha * current + (1 - alpha) * avg_values[i];
			if (!triggered[i] && (current - avg_values[i]) > threshold)
			{
				triggered[i] = true;
				triggered_byte |= (1 << i);
			}
			if (triggered[i] && (current - avg_values[i]) < (threshold / 2))
			{
				triggered[i] = false;
			}
		}
		return triggered_byte;
	}


	/*
	* openingsByte method for getting sensor openings
	? @param triggered_byte: from the openingsByte method
	? @param day_index: the index of the day to check from DateTime.dayOfTheWeek()
	? @return: a boolean of whether the selected sensor has been triggered  */
	bool listenForOpening(byte triggered_byte, int day_index)
	{
		/*
		? this uses a triggered byte as a parameter instead of just calling it because
		? its assumed that the byte will be called each loop, and multiple calls
		? will lead to skipped openings

		? yes, this could be mediated by creating another function just for
		? checking one sensor, but i prefer having a single function 
		? for checking all both for debugging and because being able to 
		? track sensors is needed for looking refills*/

		if (triggered_byte)
		{
			if (triggered_byte & (1 << sensors_indexed_by_day[day_index]))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		return false;
	}
};

struct Opening
{
	DateTime time;	   //?  time of the opening
	bool valid = true; /* 
	? present for edge cases, if a user doesnt take their 
	? dose for a day, we'll add a non valid opening,
	? this will allow the AllOpenings class to iterate and calculate openings
	? for each day even if a user forgets an opening one day*/

	Opening() : time() {} //? a default constructor

	/*
	* Opening class constructor
	? @param time: the time of the opening, as a DateTime object */
	Opening(DateTime time) : time(time) {}
	int getDay()
	{
		return time.dayOfTheWeek();
	}
};

// ? this class stores all the openings tracked by the program using 
// ? a circular queue data structure, limited by the max_openings
// ? this limit is present cause of the limited memory space
class AllOpenings
{
public:
	static const int max_openings = 100; // ? the maximum amount of openings
	                                     // ? * limited by arduino memory
	int dose_count; //? the amount of doses ! per day !
	Opening openings[max_openings]; // ? main array of openings

	int head = 0; // ? head of the queue, oldest opening
	int tail = 0; // ? tail of the queue, newest opening
	int count = 0; // ? amount of openings in queue
	AllOpenings() : dose_count(0) {} // ? default constructor 

	/*
	* AllOpenings class constructor
	? @param dose_count: dose amount per day, used when iterating  */
	AllOpenings(int dose_count) : dose_count(dose_count) {}


	/*
	* save method which saves openings to the queue
	? @param opening: the opening object to save   */
	void save(Opening opening)
	{
		if (count == max_openings)
		{
			head = (head + 1) % max_openings; // ? if queue full, move head
		}
		else
		{
			count++; //? increase count if its not full
		}
		openings[tail] = opening; //? save opening to tail
		tail = (tail + 1) % max_openings; //? move tail
	}

	/*
	* prints the entire queue of openings
	? this is really only here right now as a basic example of iterating 
	? through the queue, mostly for myself and debugging*/
	void printQueue()
	{
		for (int i = 0; i < count; i++) //? for each item in the queue
		{
			int index = (head + i) % max_openings; 
			Opening opening = openings[index];
			// // Serial.print(opening);
		}
	}


	/*
	* calculateAlarmTimes method calculates when the alarms should play in the day
	? @param alarm_times: array of datetimes that will be configured 
	? @param now: the current time */
	void calculateAlarmTimes(DateTime *alarm_times, DateTime now)
	{
		// TODO add weight to more recent doses
			// ! how to do this? 

		// TODO add a check to see if a dose is very irregular,
			// TODO and if so done use it in calculation
			// ! mby, get a slice of each 5 dose and average, and if 
			// ! the individual dose is too far from the 5 day slice
			// ! then disregard / replace it with a normal one
		
		


		/* 
		? so, the way the dose count works is that if a user has 2 doses per 
		? it will use 2 positions in the array at a time, so an array of 
		? openings for a user that takes 2 doses per day for example will
		? will have a total of 40 actual days of data, where 
		? openings[0] + [1] is day 1, openings[2] + [3] is day 2 ...

		? this for loop then iterates through each dose of the day
		? and calculates the average time of that dose without it affecting 
		? the other daily doses*/
		for (int d = 0; d < dose_count; d++) // ? index of dose
		{							
			long total_seconds = 0; 
			int num = 0;			
			for (int i = 0; i < count; i++)
			{
				int index = (head + i) % max_openings; // ? index in queue
				if ((i % dose_count) == d && openings[index].valid)
				{ // ? if it is the N'th dose corresponding to the N'th dose of the day
					DateTime time_of_dose = openings[index].time;
					long seconds = time_of_dose.hour() * 3600L + time_of_dose.minute() * 60L + time_of_dose.second();
					total_seconds += seconds; 
					num++;
				}
			}
			long avg_seconds = total_seconds / num; // ? calculates average
			int hour = avg_seconds / 3600;
			int minute = (avg_seconds % 3600) / 60;
			int second = avg_seconds % 60; 

			// ? rounds to the nearest 15 minute interval
			long rounded_seconds = ((avg_seconds + 450) / 900) * 900;
			int rounded_hour = rounded_seconds / 3600;
			int rounded_minute = (rounded_seconds % 3600) / 60;
			int rounded_second = rounded_seconds % 60;

			// ? updates the DateTime alarm times array with the newly calculated time
			alarm_times[d] = DateTime(now.year(), now.month(), now.day(), rounded_hour, rounded_minute, rounded_second);
		}
	}
};

// * // * variables

// * lids / sensor openings
const int max_doses_per_day = 3; // ? maximum amount of doses per day, can be changed later

AllOpenings all_openings; // ? main AllOpenings objects
Luker luker(amux_a_pin, amux_b_pin, amux_c_pin, amux_com_pin); //? main Luker object


// * RTC / clock display
RTC_DS3231 rtc; // ? init RTC
DateTime now; // ? make a global now time, to be updated each loop and keep 
              // ? track of real time throughout the program  
			  // ? this DateTime object has a bunch of methods
			  // ? for getting specific time values

const byte segment_byte_map[10] = {
	/* 
	? this is a map of the segments of the 7 segment display and the 
	? respective bytes that have to be shifted to make certain numbers)*/

	0b00111111, // 0
	0b00000110, // 1
	0b01011011, // 2
	0b01001111, // 3
	0b01100110, // 4
	0b01101101, // 5
	0b01111101, // 6
	0b00000111, // 7
	0b01111111, // 8
	0b01101111	// 9
};
const byte digits_byte_mask[4] = {
	/*
	? similarly, this is a map of the 4 digits of the display and the respecitve
	? bytes that have to be shifted to turn certain digits ON or OFF 
	? since the first digit is connected to the first shift register in the 
	? chain and the 3 others are connected to the other chained register
	? we use two different masks to turn the first one and three others off
	? and vice versa on the other*/
	0b11111111, // digit 1 OFF
	0b11111110, // digit 2 ON (bit 0 = 0)
	0b11111101, // digit 3 ON (bit 1 = 0)
	0b11111011	// digit 4 ON (bit 2 = 0)
};
const byte digit_one_bit_mask[4] = {
	0b00000000, // index 0 → digit1 ON (bit7 = 0)
	0b10000000, // index 1 → digit1 OFF (bit7 = 1)
	0b10000000, // index 2 → digit1 OFF
	0b10000000	// index 3 → digit1 OFF
};

// * DFPlayer and loudspeaker
SoftwareSerial DFPlayer_serial(DFPlayer_RX_pin, DFPlayer_TX_pin); 
	// ? makes a serial connection to the player
DFPlayerMini_Fast DFPlayer; // ? DFPlayer object
int volume = 0; // ? global volume variable
int old_volume = 0; // ? old volume variable so that volume only gets updated when actually changed
bool update_player = true; // ?  whether to use the player or not, used in switched and alarms

// * LED row
volatile int led_wave_pos = 0; // ? current position of wave
volatile int led_wave_dir = 1; // ? direction of wave, 1 for right, -1 for left
volatile bool led_wave_active = false; // ? whether the wave effect is acitve
const int led_wave_steps = 8; // ? amount of steps in the wave not amount of LEDs
const unsigned long led_wave_interval = 100;	   // ? the time between steps in ms
volatile unsigned long led_wave_last_update = 0; // ? last time the wave was updated
const int led_wave_start = 1;				   // ? first LED pin in the row
const int led_wave_end = 7;					   // ? last one
bool leds_on = true;			// ? equivalent to update_player, used with alarms and switches

// * // * Functions

// * lids / sensor openings

// * RTC / clock display
/* 
? this function updates the clock display, and is connected to the Timer1
? library, which updates it independently of the main loop. this was needed
? after we plugged all the components together, and saw that the loop ran 
? slower the more components were connected, which made the display
? update less frequently, and since the display works by flashing the digits 
? at high frequencies it was noticable that the digits were updating 
? independatly which looked really harsh */

void updateClock()
{
	int hours = now.hour();
	int minutes = now.minute(); // ? updates to the live current time

	uint8_t d0 = (hours / 10) % 10;
	uint8_t d1 = (hours % 10);
	uint8_t d2 = (minutes / 10) % 10;
	uint8_t d3 = (minutes % 10); // ? gets the digits of the current time

	static uint8_t current = 0; // ? current digit to update, starts at 0

	uint8_t val = (current == 0	  ? d0
				   : current == 1 ? d1
				   : current == 2 ? d2
								  : d3);

	byte segs = segment_byte_map[val]; // ? finds the mapped segments
	byte m1bit7 = digit_one_bit_mask[current]; 

	const byte COLON_BIT = (1 << 3); // ? lights up the colon 
	byte m2 = digits_byte_mask[current] | COLON_BIT; 
	// ? gets the mask for the current digit, and adds the colon bit

	digitalWrite(clock_latch_pin, LOW);
	shiftOut(clock_data_pin, clock_clock_pin, MSBFIRST, m2);
	shiftOut(clock_data_pin, clock_clock_pin, MSBFIRST, segs | m1bit7);
	digitalWrite(clock_latch_pin, HIGH); // ? writes it all out to the clock

	current = (current + 1) & 0x03; // ? increments the current digit
}

// * DFPlayer and loudspeaker
// ? sets the volume of the DFPlayer 
void updatePlayer()
{
	volume = map(analogRead(volume_pin), 0, 1023, 0, 30); //? maps volume pin to 0-30
	if (old_volume != volume && volume < 29) // ? only update if there is a change
	// ? we implemented this because we noticed that sending frequent updates to the player
	// ? caused it to lag when playing sounds and missing instructions
	{
		DFPlayer.volume(volume);
		old_volume = volume;
	}
}

// * LED row
// ? function for sending out bytes to the LED shift register
void updateLEDs(byte state)
{
	digitalWrite(led_latch_pin, LOW);
	shiftOut(led_data_pin, led_clock_pin, MSBFIRST, state);
	digitalWrite(led_latch_pin, HIGH);
}
// ? function for updating the LED wave effect
void ledWaveTask()
{
	if (!led_wave_active) // ? dont update if its not set to active, i.e switch is off
	{
		updateLEDs(0);
		return;
	}
	unsigned long now = millis(); // ? gets current time (doesnt have to be rtc as 
								  // ? the wave effect is not time sensitive)
	if (now - led_wave_last_update >= led_wave_interval) // ? if its past interval time
	{
		updateLEDs(1 << led_wave_pos); // ? update the LEDs with current positions

		led_wave_pos += led_wave_dir;
		if (led_wave_pos > led_wave_end)
		{
			led_wave_pos = led_wave_end - 1;
			led_wave_dir = -1;
		}
		else if (led_wave_pos < led_wave_start)
		{
			led_wave_pos = led_wave_start + 1;
			led_wave_dir = 1;
		}  // ? update the positions
		led_wave_last_update = now; 
	}
}
// ? functions to stop and start the led wave without blocking the main loop
void startLedWave()
{
	led_wave_active = true;
	led_wave_pos = led_wave_start;
	led_wave_dir = 1;
	led_wave_last_update = millis();
}
void stopLedWave()
{
	led_wave_active = false;
	updateLEDs(0);
}

// * // * Setup
void setup()
{
	pinMode(clock_latch_pin, OUTPUT);
	pinMode(clock_data_pin, OUTPUT);
	pinMode(clock_clock_pin, OUTPUT);
	pinMode(led_clock_pin, OUTPUT);
	pinMode(led_data_pin, OUTPUT);
	pinMode(led_latch_pin, OUTPUT);
	pinMode(amux_a_pin, OUTPUT);
	pinMode(amux_b_pin, OUTPUT);
	pinMode(amux_c_pin, OUTPUT);
	pinMode(volume_pin, INPUT); 
	
	Serial.begin(9600);
	DFPlayer_serial.begin(9600);

	if (!DFPlayer.begin(DFPlayer_serial)) // ? inits DFPlayer
	{ // ? if init fails
		Serial.println("DFPlayer Mini not found!");
		Serial.flush();
	}

	Wire.begin();
	if (!rtc.begin()) // ? inits RTC
	{ // ? if init fails
		Serial.println("RTC not found!");
		Serial.flush();
	}
	if (rtc.lostPower())
	{
		// ? if rtc battery lost power, set the time to the compile time
		rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
	}
	rtc.disable32K(); // ? not needed
	now = rtc.now();
	DateTime start_time = now; // ? a time for when the actual program starts

	Timer1.initialize(2000);
	Timer1.attachInterrupt(updateClock); // ? inits and attaches the clock update
	
	// ? Ensure things arent running
	stopLedWave();
	DFPlayer.stop();

	if (digitalRead(switch_1_pin) == LOW)
	{
		DFPlayer.volume(0);	   // ? turn all sound OFF if switch is off
		update_player = false; 
	}
	else
	{
		volume = map(analogRead(volume_pin), 0, 1023, 0, 30);
		DFPlayer.volume(volume); // ? vice versa
		update_player = true;
	} 

	if (digitalRead(switch_2_pin) == LOW)
	{
		updateLEDs(B00000000); // ? turn all LEDs OFF if switch is off
		leds_on = false;	   
	}
	else
	{
		leds_on = true; // ? vice versa
	} 

	bool turn_on_condition = true; 
		// TODO add a turn on condition like pushing a button

	while (!turn_on_condition)
	{
		break;
	}


	// ? the program uses one day to configure the openings where the alarm 
	// ? doesnt actually play, this day is contained in the setup() function
	// ? if certain requirements arent met by the time the first day is over 
	// ? the program will consider the next day a configuration day and so on 


	bool first_day_over = false; // ? whether configuration day is over
	int day_to_check = start_time.dayOfTheWeek(); // ? gets the day of the week the config day is

	Opening first_day_openings[max_doses_per_day]; // ? array for the configuration day openings

	int dose_count = 0; // ? counts openings on the configuration day, this 
	// ? will be used to determine the daily dose count for the user
	while (!first_day_over)
	{ // ? only stop config day if this is met 

		// TODO add a check to see if the openings are attached to ALL opening, i.e a refill is happening
		// TODO if the sensor after the opening of the respecive day is opened within
		// TODO 5 minutes of the first opening, disregard that first opening

		now = rtc.now(); // ? updates time

		bool day_sensor_opened = luker.listenForOpening(luker.openingsByte(), day_to_check);
		// ? only count openings of the relevant day, if detected add them

		if (start_time.dayOfTheWeek() == now.dayOfTheWeek()) // ? still on config day 
		{ 
			if (day_sensor_opened)
			{
				first_day_openings[dose_count] = Opening(now); 
				dose_count++;
				// ? if an opening is detected, add it to all openings and increment dose
				if (dose_count == max_doses_per_day)
				// ? if the max is reached end config day 
				{ 
					first_day_over = true;
				}
			}
		}
		else if (start_time.dayOfTheWeek() != now.dayOfTheWeek())
		{ // ? when config day is over
			if (dose_count == 0)
			{ // ? if there are no openings
				start_time = now;
				day_to_check = start_time.dayOfTheWeek(); 
				// ? reset the config day to be this next day, dont exit setup
			}
			else if (dose_count > 0)
			{
				first_day_over = true; 
				// ? if a dose has been detected, exit the config day
			}
		}
	}

	all_openings = AllOpenings(dose_count); // ? create the openings object
	for (int i = 0; i < dose_count; i++)
	{
		all_openings.save(first_day_openings[i]); 
		// ? and add all the config day opening to this
	}

}

//? After setup, we are left with a global all_openinings oject
//? with an array of the config days' openings

// * // * Main loop global variables
DateTime old_day = now; // ? used to keep track of when new days start
int current_dose_index = 0; // ? used to keep track of which dose we are on in a day 
bool alarm_playing = false; // ? used to keep track of whether the alarm is playing
DateTime alarm_times[max_doses_per_day]; // ? array for storing the alarm times that get updated

void loop()
{
	// TODO add a check to see if the openings are attached to ALL opening, i.e a refill is happening
	// TODO if the sensor after the opening of the respecive day is opened within
	// TODO 5 minutes of the first opening, disregard that first opening

	// * looping values
	now = rtc.now();
	byte opening_byte = luker.openingsByte();
	// ? updates the time, and gets any detected openings

	// * swtich actions
	static int prev_switch_1 = -1;
	static int prev_switch_2 = -1;
	int switch_1 = digitalRead(switch_1_pin);
	int switch_2 = digitalRead(switch_2_pin);
	// ? gets the state of the switches
	if (prev_switch_1 == -1)
		prev_switch_1 = switch_1;
	if (prev_switch_2 == -1)
		prev_switch_2 = switch_2;
	if (switch_1 != prev_switch_1)
	{
		prev_switch_1 = switch_1;
		if (switch_1 == LOW)
		{
			DFPlayer.volume(0);
			update_player = false;
		}
		else
		{
			volume = map(analogRead(volume_pin), 0, 1023, 0, 30);
			update_player = true;
			DFPlayer.volume(volume);
		}
	} // ? as in setup, if the switch is OFF volume is off, and vice versa
	if (switch_2 != prev_switch_2)
	{
		prev_switch_2 = switch_2;
		if (switch_2 == LOW)
		{
			updateLEDs(B00000000);
			leds_on = false;
		}
		else
		{
			leds_on = true;
		}
	} // ? as in setup, if the switch is OFF LEDs are off, and vice versa

	if (update_player)
	{
		updatePlayer();
	}
	if (leds_on)
	{
		ledWaveTask();
	} 
	// ? only actually update the player and LEDs if the switches are ON

	if (old_day.dayOfTheWeek() != now.dayOfTheWeek())
	{// ? a new day has started
		// TODO make sure dose_count of doses has been passed for the day, no more no less
		// TODO if less than dose_count, add non valid openings for the rest of the day 
		// TODO to make the calculateAlarmTimes method work correctly 

		all_openings.calculateAlarmTimes(alarm_times, now);
		// ? calculates new alarm times at the end of the day, to be used 
		// ? in the next day 

		old_day = now; 
		current_dose_index = 0; // ? updates day
	}

	if (current_dose_index < all_openings.dose_count)
	{ // ? if there are still untaken doses for the day
		DateTime alarm_time = alarm_times[current_dose_index]; // ? the alarm time for the current dose
		
		if (!alarm_playing && now >= alarm_time) // ? if the alarm isnt playing and its alarm time
		{ // ? play alarm
			startLedWave();
			DFPlayer.loop(3); 
			alarm_playing = true; // ? plays our alarm sound and lights
		}

		
		if (alarm_playing)
		{ // ? if the alarm is going off, wait for an opening of the right day
			if (luker.listenForOpening(opening_byte, now.dayOfTheWeek()))
			{ // ? when the opening is detected
				stopLedWave();
				DFPlayer.stop();
				alarm_playing = false;
				// ? stop the alarm 
				delay(200);
				DFPlayer.play(4);
				// ? play the sound we have for completing a dose
				// ? this is for feedback and gratification to the user
				byte led_mask = 1 << now.dayOfTheWeek();
				for (int i = 0; i < 3; i++)
				{
					updateLEDs(led_mask);
					delay(200);
					updateLEDs(0);
					delay(200);
				} // ? also flash the LEDs a bit to reinforce the feedback
				all_openings.save(Opening(now)); // ? save the opening
				current_dose_index++; // ? increment dose
			}
		}
	}
}
