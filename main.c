/*
 * DoorRemote.c
 *
 * Created: 24.02.2018 22:12:39
 * Author : norman
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#define TIMER1_PRESCALE_1 1
#define TIMER1_PRESCALE_8 2
#define TIMER1_PRESCALE_64 3
#define TIMER1_PRESCALE_256 4
#define TIMER1_PRESCALE_1024 5


#define INTER_SYMBOL_PAUSE 204				// 26 ms low between symbols
#define SIGNAL_LENGTH 14


const uint8_t signals[][SIGNAL_LENGTH] = { 
	{ 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 2},			// '2' is the inter-symbol pause
	{ 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 2}
	
};

const uint8_t timings[][3] = {
	{11, 6},	// ZERO: [0 => clk low, 1=> clk high]
	{5 , 12},	// ONE
	{INTER_SYMBOL_PAUSE, 0}
};

typedef enum {
	FrontDoor=0,
	GarageDoor,
	None
} SignalType_t;


volatile SignalType_t signalType;
volatile uint8_t currSignalBit;
volatile uint8_t sigCycleCounter;
volatile uint8_t timingIndex=0;

// this function is called when timer1 compare matches OCR1A
ISR (TIMER1_COMPA_vect) {
	uint8_t curBit;

	// put it in front of routine to get runtime-independent output-timings
	if (timingIndex==1) {
		PORTB |= (1 << PB2);
	}
	else {
		PORTB &= ~(1 << PB2);
	}

	// valid signal ?
	if (signalType != None) {
		// get cur bit value (0, 1 or 2)
		curBit = signals[signalType][currSignalBit];

		// inc cycle counter
		sigCycleCounter++;

		// once cycle counter reaches timing value (depends on bit) 
		// we set the index (currSignalBit) to the next one and reset the cycle counter
		if (sigCycleCounter >= timings[curBit][timingIndex]) {
			if (timingIndex==0x01) {
				// wrap signal-bit index ?
				//if (currSignalBit < sizeof(timings[signalType])-1) {
			
				if (currSignalBit < SIGNAL_LENGTH-1) {
					currSignalBit++;
				}
				else {
					currSignalBit=0;
				}
			}
			// reset cycle counter
			sigCycleCounter=0;

			// invert timing-index
			timingIndex ^= 0x01;
		}

		
	}
}

int main(void)
{
	/*
	the frequency of the interrupt overflow is determined by the 
	prescaler and overflow value.
	freq = clock_frequency / ( 2 * prescaler * overflow_val)
	where prescaler can be 1, 8, 64, 256, or 1024
	clock_freq is 8MHz
	and overflow_val is 16bit

	the overflow value is placed in OCR1A, the prescale is set in TCCR1B
    
	clock freq = 8MHz
    
	
	f=7692/2 -> P=260ms
	8e+6 / (7692) = 1040 : prescaler=1, overflow = 1040
	*/

	TCCR1B = (1 << WGM12) | TIMER1_PRESCALE_1;
	
	//OCR1A = (uint16_t)520;					// counter overflow register
	OCR1A = (uint16_t)1040;					// counter overflow register
	TIMSK &= ~(1 << OCIE1A);				// Output Compare Interrupt disable (timer 1, OCR1A) 

	sei();									// Set Enable Interrupts

	// PORTB = (1 << PB0) | (1 << PB1);		// pull up PB0 and PB1
	DDRB = (0 << DDB1) | (0 << DDB0) | (1 << DDB2) | (1 << DDB3);	// declare PB0 and PB1 as input and PB2 + PB3 as output
	
	signalType = None; 

	while (1) 
	{
		if (((PINB & (1 << PINB0)) == ( 1 << PINB0)) && signalType==None) {
			// button pressed (high)
			
			signalType = FrontDoor; 
			currSignalBit = 0; 
			sigCycleCounter=0;
			timingIndex=0;
			
			TCNT1 = 0;						// clear counter register
			TIMSK |= 1 << OCIE1A;			// Output Compare Interrupt Enable (timer 1, OCR1A)
			PORTB |= (1 << PB3);
		}
		else if (((PINB & (1 << PINB1)) == (1 << PINB1)) && signalType==None) {
			signalType = GarageDoor;
			currSignalBit = 0;
			sigCycleCounter=0;
			timingIndex=0;
			
			TCNT1 = 0;						// clear counter register
			TIMSK |= 1 << OCIE1A;			// Output Compare Interrupt Enable (timer 1, OCR1A)
			PORTB |= (1 << PB3);
		}
		// all buttons low 
		if ((PINB & ((1 << PINB0) | (1 << PINB1)) ) == 0 && signalType != None) {
			TIMSK &= ~(1 << OCIE1A);			// Output Compare Interrupt disabled (timer 1, OCR1A)
			signalType = None;
			PORTB &= ~(1 << PB3);
		}
	}
}

