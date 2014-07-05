/*
*   LTTO Medic Remote
*   Copyright (C) 2014  Ryan L. "Izzy84075" Bales
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License along
*    with this program; if not, write to the Free Software Foundation, Inc.,
*    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <ez8.h>

#include "main.h"

volatile unsigned char ir_ms_counter;
volatile unsigned char ir_ms_counter_state;
volatile unsigned char ir_ms_counter_txing;
volatile unsigned char ir_ms_counter_ready;

volatile unsigned char ir_ms_counter_queue;
volatile unsigned char ir_ms_counter_queue_state;
volatile unsigned char ir_ms_counter_queue_ready;

volatile unsigned char delay_ms;
volatile unsigned char which_teams;

void init_cpu(void) {
	DI();
	
	//PA0: DEBUG, otherwise unused.
	//PA1: DIP1, input, internal pullup.
	//PA2: !RESET on startup, LED_OUT after initialization, external pullup, active low.
	//PA3: DIP2, input, internal pullup.
	//PA4: DIP3, input, internal pullup.
	//PA5: DIP4, input, internal pullup.
	
	//Set up our GPIO pins.
	PADD =   00111010;
	PAAF =   00000100;
	PAAFS1 = 00000100;
	PAAFS2 = 00000000;
	PAOC =   00000100;
	PAPUE =  00111010;
	
	//Set up our 38KHz timer.
	T1CTL1 = 0x01;				//Disable timer, Set to Continuous, Set prescaler, Set initial output state
	T1H = 0x00; T1L = 0x01;		//Set starting count (1)
	T1RH = 0x00; T1RL = 145;	//Set reload value (145)
	
	//Set up our 1ms timer.
	T0CTL1 = 0x01;
	T0H = 0x00; T0L = 0x01;
	T0R = 5530;	//5530
	T0CTL1 |= 0x80;
	IRQ0ENH |= 0x20;
	IRQ0ENL |= 0x20;
	SET_VECTOR(TIMER0, isr_tmr0);
	
	
	
	EI();
}

void interrupt isr_tmr0(void) {
	if(ir_ms_counter_txing) {	//If we've already started transmitting something...
		ir_ms_counter--;		//Decrement the counter for what we're currently sending
		if(!ir_ms_counter) {	//If the counter's expired
			if(ir_ms_counter_queue_ready) {	//Check if there's something else queued
				//Shuffle the variables out of the queue and into the current one.
				ir_ms_counter = ir_ms_counter_queue;	
				ir_ms_counter_state = ir_ms_counter_queue_state;
				ir_ms_counter_queue_ready = 0;
			} else { //Nothing else queued
				//Reset the current variables.
				ir_ms_counter_state = 0;
				ir_ms_counter_txing = 0;
				ir_ms_counter_ready = 0;
			}
		}
	} else if(ir_ms_counter_ready) {	//Not currently transmitting, but there's something queued.
		ir_ms_counter_txing = 1;		//Flag that we're transmitting now.
	}
	set_IR_state(ir_ms_counter_state);	//Turn on/off the IR generation.
}

void queue_IR(unsigned char ms, unsigned char state) {
	if(!ir_ms_counter_ready) {	//If there's nothing queued up
		//Stuff it into the current one so it takes effect next tick
		ir_ms_counter = ms;
		ir_ms_counter_state = state;
		ir_ms_counter_ready = 1;
	} else {	//There's something already transmitting, so let's put this in the queue.
		unsigned char finished = 0;
		while(!finished) {	//Loop until we actually get it into the queue.
			if(!ir_ms_counter_queue_ready) {	//Is there anything in the queue already?
				//Stuff it!
				ir_ms_counter_queue = ms;
				ir_ms_counter_queue_state = state;
				ir_ms_counter_queue_ready = 1;
				//Flag that we've put it in the queue.
				finished = 1;
			}
		}
	}
}

void set_IR_state(unsigned char state) {
	if(state) {
		//Turn on IR generation
		//TODO
	} else {
		//Turn off IR generation
		//TODO
	}
}

void main(void) {
	unsigned char input;
	unsigned char loopCounter;
	
	init_cpu();
	
	//Get the state of the DIP switches.
	input = PAIN;
	if(input & 0x02) {
		input |= 0x04;
	}
	input >>= 2;
	input = ~input & 0x0F;
	
	//Extract which team IDs to send
	switch(input & 0x03) {
		case 0x00:
			//All teams
			which_teams = 0x0F;
			delay_ms = 51;
			break;
		case 0x01:
			//T0 + T2
			which_teams = 0x05;
			break;
		case 0x02:
			//T0 + T1
			which_teams = 0x03;
			break;
		case 0x03:
			//T0 + T3
			which_teams = 0x09;
			break;
	}
	
	//Extract the delay information
	if(!delay_ms) {
		switch((input >> 2)) {
			case 0x00:
				//2 signatures per team per second.
				//4 signatures per second total.
				//15 seconds to 1 health gained.
				//Need a 218ms delay between signatures.
				delay_ms = 218;
				break;
			case 0x01:
				//4 signatures per team per second.
				//8 signatures per second total.
				//5 seconds to 1 health gained.
				//Need a 93ms delay between signatures.
				delay_ms = 93;
				break;
			case 0x02:
				//3 signatures per team per second.
				//6 signatures per second total.
				//8 seconds to 1 health gained.
				//Need a 134ms delay between signatures.
				delay_ms = 134;
				break;
			case 0x03:
				//6 signatures per team per second.
				//12 signatures per second total.
				//3 seconds to 1 health gained.
				//Need a 51ms delay between signatures.
				delay_ms = 51;
				break;
		}
	}
	
	loopCounter = 60;
	while(loopCounter) {
		if(which_teams & 0x01) {
			//Send a T0 respawn signature
			queue_IR(3, 1);
			queue_IR(6, 0);
			queue_IR(6, 1);
			queue_IR(2, 0);
			queue_IR(1, 1);
			queue_IR(2, 0);
			queue_IR(1, 1);
			queue_IR(2, 0);
			queue_IR(1, 1);
			queue_IR(2, 0);
			queue_IR(2, 1);
			queue_IR(2, 0);
			queue_IR(2, 1);
			queue_IR(delay_ms, 0);
		}
		if(which_teams & 0x02) {
			//Send a T1 respawn signature
			queue_IR(3, 1);
			queue_IR(6, 0);
			queue_IR(6, 1);
			queue_IR(2, 0);
			queue_IR(1, 1);
			queue_IR(2, 0);
			queue_IR(2, 1);
			queue_IR(2, 0);
			queue_IR(1, 1);
			queue_IR(2, 0);
			queue_IR(2, 1);
			queue_IR(2, 0);
			queue_IR(2, 1);
			queue_IR(delay_ms, 0);
		}
		if(which_teams & 0x04) {
			//Send a T2 respawn signature
			queue_IR(3, 1);
			queue_IR(6, 0);
			queue_IR(6, 1);
			queue_IR(2, 0);
			queue_IR(2, 1);
			queue_IR(2, 0);
			queue_IR(1, 1);
			queue_IR(2, 0);
			queue_IR(1, 1);
			queue_IR(2, 0);
			queue_IR(2, 1);
			queue_IR(2, 0);
			queue_IR(2, 1);
			queue_IR(delay_ms, 0);
		}
		if(which_teams & 0x08) {
			//Send a T3 respawn signature
			queue_IR(3, 1);
			queue_IR(6, 0);
			queue_IR(6, 1);
			queue_IR(2, 0);
			queue_IR(2, 1);
			queue_IR(2, 0);
			queue_IR(2, 1);
			queue_IR(2, 0);
			queue_IR(1, 1);
			queue_IR(2, 0);
			queue_IR(2, 1);
			queue_IR(2, 0);
			queue_IR(2, 1);
			queue_IR(delay_ms, 0);
		}
		
		loopCounter--;
	}
	
	//Go into low power mode here.
	//TODO
}