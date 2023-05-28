#define FONT_LM80C
#define DISABLE_CURSOR
#define BIN_TYPE BIN_HOMEBREW

#include "../../NABULIB/NABU-LIB.h"
#include "../../NABULIB/patterns.h"

// decode and print joystick byte
void printJoystick(uint8_t b)
{
	char state[6];

	state[0] = '.';
	state[1] = '.';
	state[2] = '.';
	state[3] = '.';
	state[4] = '.';
	state[5] = 0;

	if (b & Joy_Left) {
		state[0] = 'L';
	}
	if (b & Joy_Down) {
		state[1] = 'D';
	}
	if (b & Joy_Right) {
		state[2] = 'R';
	}
	if (b & Joy_Up) {
		state[3] = 'U';
	}
	if (b & Joy_Button) {
		state[4] = 'F';
	}
	vdp_print(state);
}

// print hex byte
void printHexByte(uint8_t b)
{
	uint8_t high = b >> 4;
	uint8_t low = b & 0x0F;
	char val[3];
	val[2] = 0;
	if (low < 10) {
		low += 0x30;
	}
	if (low > 9 && low <= 15) {
		low += 0x37;
	}
	if (high < 10) {
		high += 0x30;
	}
	if (high > 9 && high <= 15) {
		high += 0x37;
	}
	val[0] = high;
	val[1] = low;
	vdp_print(val);
}

void main() {
	// init the nabu lib which does interrupts and all that jazz
	initNABULib();

	// Set the text mode to foreground color white, bg black
	vdp_initTextMode(0xf, 0x0, true);
	vdp_loadASCIIFont(ASCII);

	// print title
	vdp_setCursor2(14, 0);
	vdp_print("Input Tester");

	// print Paddle 1 header
	vdp_setCursor2(5, 5);
	vdp_print("Paddle 1: ");

	// print Paddle 2 header
	vdp_setCursor2(23, 5);
	vdp_print("Paddle 2: ");

	// print Paddle 3 header
	vdp_setCursor2(5, 10);
	vdp_print("Paddle 3: ");

	// print Paddle 4 header
	vdp_setCursor2(23, 10);
	vdp_print("Paddle 4: ");

	// print Joystick 1 header
	vdp_setCursor2(2, 15);
	vdp_print("Joystick 1: ");

	// print Joystick 2 header
	vdp_setCursor2(21, 15);
	vdp_print("Joystick 2: ");

	// print Keyboard header
	vdp_setCursor2(14, 20);
	vdp_print("Keyboard: 00");

	while (true) {
		// read paddle 1 value
		uint8_t p1 = getPaddleValue(0);
		// read paddle 2 value
		uint8_t p2 = getPaddleValue(1);
		// read paddle 3 value
		uint8_t p3 = getPaddleValue(2);
		// read paddle 4 value
		uint8_t p4 = getPaddleValue(3);

		// read  joystick 1 status
		uint8_t j1 = getJoyStatus(0);
		// read  joystick 2 status
		uint8_t j2 = getJoyStatus(1);

		// display paddle 1 value
		vdp_setCursor2(15, 5);
		printHexByte(p1);
		// display paddle 2 value
		vdp_setCursor2(33, 5);
		printHexByte(p2);
		// display paddle 3 value
		vdp_setCursor2(15, 10);
		printHexByte(p3);
		// display paddle 4 value
		vdp_setCursor2(33, 10);
		printHexByte(p4);
		// display joystick 1 state
		vdp_setCursor2(14, 15);
		printJoystick(j1);
		// display joystick 2 state
		vdp_setCursor2(33, 15);
		printJoystick(j2);

		// display last keyboard keycode
		if (isKeyPressed()) {
			vdp_setCursor2(24, 20);
			printHexByte(getChar());
		}
	}
}
