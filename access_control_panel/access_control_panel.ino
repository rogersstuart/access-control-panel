/*
Mid City Tower Access Control Project
NFC Reader Program

Written By: Stuart Rogers
*/

//#include <Arduino_FreeRTOS.h>
#include <nfc.h>

//led pins
#define RED_LED_PIN 6
#define GREEN_LED_PIN 5
#define BLUE_LED_PIN 3

//Led Animation Sequences
//[][0] pwm setting, [][1] time in milliseconds
//all sequences must follow a rgb pattern
//the first value of the pwm setting array must contain the sequence length (number of patterns)
//the first value of the timing array will be ignored
//the last rgb value of the sequence will be held
                                     
const uint16_t general_system_failure[][2] = {{2,0}, {254,250},{0,250},{0,250},
													 {0, 200},{0,200},{0,200}};
                                         
// const uint16_t card_presented[][2]         = {{2, 0},{0, 200},{25,200},{18,200},
// 													 {0, 25} ,{0, 25}, {0, 25}};
														 
const uint16_t card_presented[][2]         = {{1, 0},{142,1},{112,1},{0,1}};
                                  
const uint16_t card_approved[][2]          = {{2, 0},{0, 2000},{0, 2000},{254, 2000},
													 {0, 1},  {0, 1}, {0, 1}};
														 
const uint16_t alt_card_approved[][2]          = {{2, 0},{0, 2000},{254, 2000},{0, 2000},
													     {0, 1},  {0, 1}, {0, 1}};
                                 
// const uint16_t student_card_approved[][2]  = {{2, 0},{0, 2000},{0, 2000},{45, 2000},
// 													 {0, 100}, {0, 100},  {0, 100}};
                                     
const uint16_t card_declined[][2]		   = {{2, 0},{254, 2000},{0, 2000},{0, 2000},
													 {0 , 1}, {0, 1}, {0, 1}};
														 
const uint16_t power_on_animation[][2]     = {{4, 0},{254, 250},{0 , 250} ,{0 , 250},
													 {127, 500},{127, 500},{0 , 500},
													 {85, 1000} ,{85, 1000} ,{85, 1000},
													 {0 , 1} ,{0 , 1} ,{0 , 1}};

NFC_Module nfc;

uint32_t comm_timeout_timer, tap_timeout_timer;

uint32_t timeout_time = 2000;

uint8_t reader_id = '0'; //this can be any ascii value from 0 - 9

uint8_t master_ready_command = 'M';
uint8_t master_acknowledge_command = 'A';

uint8_t skip_panel_command = 'B';

uint8_t nfc_buffer[32];

uint8_t card_present = 0;

boolean master_ready = false;

void setup()
{ 
  Serial.begin(9600);
  
  animateLED(power_on_animation, 1, false);
  
  setupNFC();
}

void loop()
{
	if(nfc.get_version() == 0){animateLED(general_system_failure, 10, false); asm volatile ("  jmp 0");} //make sure the nfc module is functioning
	
	if(!card_present)
	{
		card_present = nfc.InListPassiveTarget(nfc_buffer);
		tap_timeout_timer = millis();

                //animateLED(card_presented, 1, false);
	}
	
	while(Serial.available() > 0)
		if(Serial.read() == master_ready_command)
		{
			if(card_present)
			{
				if(!handleRequest())
					card_present = false;
			}
			else
				Serial.write(skip_panel_command);
		}
	
	if(card_present)
		if((uint32_t)((long)millis()-tap_timeout_timer) >= timeout_time)
		{
			card_present = false;
			animateLED(general_system_failure, 5, false);
		}
}

uint8_t handleRequest()
{	
	animateLED(card_presented, 1, false);
	
	Serial.write(reader_id);
	
	comm_timeout_timer = millis();
	while(Serial.available() == 0) //wait for master acknowledge
		if((uint32_t)((long)millis()-comm_timeout_timer) >= timeout_time)
			return 1;
			
	if(Serial.read() != master_acknowledge_command)
		return 1;
	
	Serial.write(nfc_buffer[0]); //send uid length
	
	comm_timeout_timer = millis();
	while(Serial.available() == 0) //wait for master acknowledge
		if((uint32_t)((long)millis()-comm_timeout_timer) >= timeout_time)
			return 1;
			
	if(Serial.read() != master_acknowledge_command)
		return 1;
	
	Serial.write(nfc_buffer+1, nfc_buffer[0]); //send uid
	
	comm_timeout_timer = millis();
	while(Serial.available() == 0) //wait for response
		if((uint32_t)((long)millis()-comm_timeout_timer) >= timeout_time)
			return 1;
	
	uint8_t auth_code = Serial.read();
	switch(auth_code)
	{
		case 68: animateLED(card_declined,		   1, false); break; //ascii D
		case 69: animateLED(card_approved,		   1, false); break; //ascii E
		case 70: animateLED(alt_card_approved,     1, false); break; //ascii F
		default: return 1;
	}
	
	return 0;
}

//this is so old and poorly written, please fix me!

//fade is not implemented yet
void animateLED(const uint16_t animation_sequence[][2], int iterations, boolean enable_fade)
{
	int red_counter_maximum =   animation_sequence[0][0]*3-2;
	int green_counter_maximum = animation_sequence[0][0]*3-1;
	int blue_counter_maximum =  animation_sequence[0][0]*3;
	
	for(int iteration_counter = 0; iteration_counter < iterations; iteration_counter++)
	{
		int red_counter = -2;
		int green_counter = -1;
		int blue_counter = 0;
		
		uint32_t time;
		
		uint32_t red_advance_time = 0;
		uint32_t green_advance_time = 0;
		uint32_t blue_advance_time = 0;
		
		int completed = 0;
		
		while(completed < 2)
		{
			
			time = millis();
			
			if(time >= red_advance_time)
			{
				if(red_counter > -3)
				{
					red_counter+=3;
					
					if(red_counter > red_counter_maximum)
					red_counter = -3;
					else
					{
						analogWrite(RED_LED_PIN,  animation_sequence[red_counter][0]); //write next red value
						red_advance_time = time + animation_sequence[red_counter][1]; //calculate the timing
					}
				}
				else
				completed++;
			}
			
			if(time >= green_advance_time)
			{
				if(green_counter > -3)
				{
					green_counter+=3;
					if(green_counter > green_counter_maximum)
					{
						green_counter = -3;
					}
					else
					{
						analogWrite(GREEN_LED_PIN, animation_sequence[green_counter][0]); //write next green value
						green_advance_time = time + animation_sequence[green_counter][1]; //calculate the timing
					}
				}
				else
				completed++;
			}
			
			if(time >= blue_advance_time)
			{
				if(blue_counter > -3)
				{
					blue_counter+=3;
					if(blue_counter > blue_counter_maximum)
					blue_counter = -3;
					else
					{
						analogWrite(BLUE_LED_PIN, animation_sequence[blue_counter][0]); //write next blue value
						blue_advance_time = time + animation_sequence[blue_counter][1]; //calculate the timing
					}
				}
				else
				completed++;
			}
		}
	}
}

void setupNFC()
{
  nfc.begin();
  nfc.SAMConfiguration();
}
