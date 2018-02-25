# attiny_keyfob
Sender for a simple 433 MHz OOK keydoor opener for "nice" system, or others.

# Requirements
I used Atemel Studio 7 but nothing special is used there apart from a timer interrupt, so any gcc-avr shold work together with the proper chip-depended IO header file.

## Hardware
I used an ATtiny2313 with 200 bytes of RAM an 2 kb Flash. The application uses, for my case, 38 bytes RAM and 446 bytes FLASH memory. So plenty of space left..

### Input
PORTB0 + PORTB1 : push-buttons (active-on-high)

### Output
- PORTB2: on-off keying data, connect with OOK 433 transmitter data-pin
- PORTB3: indication LED

# Software
The way this application works was inspired by the dwarf433 Arduino library, but since the generated code was too big I made this cooked-down version. There are several points to adjust the transmitted signal.

The array signals defines the bitstream to send, this example defines two of them.

```c
const uint8_t signals[][14] { 
	{ 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 2},			// '2' is the inter-symbol pause
	{ 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 2}
};
```

Next thing is to define how a zero, one and inter-symbol pause is defined in timings of transmitting a signal.

```c
const uint8_t timings[][3] = {
	{11, 6},	// ZERO: [0 => clk low, 1=> clk high]
	{5 , 12},	// ONE
	{INTER_SYMBOL_PAUSE, 0}
};
```

In this case as logic zero is defined of 11 timing-interrupt cycles of LOW (transmitter off) and 6 cycles of HIGH (transmitter on). 

Logic one is defined as 5 cycles low and 12 cycles of high.

The INTER_SYMBOL_PAUSE is a macro defining the cycles to wait between the transmission of the bursts, thus radio-silence.

The trickiest part is probably to find a good timer-overflow value to trigger the timer-ISR in a way that it is a common divider of all timings in the signal so that the timings can be defined as integers.
