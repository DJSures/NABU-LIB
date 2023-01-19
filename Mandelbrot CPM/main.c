// Mandelbrot.c - Render the Mandelbrot Set with a NABU PC in 64 x 48 multicolor mode
// Copyright Mike Debreceni 2022

#define BIN_TYPE BIN_CPM
#define FONT_LM80C

#include <stdio.h>
#include "../NABULIB/NABU-LIB.h"

#define MAX_ITERATION 50
#define X_RES 64.0
#define Y_RES 48.0

int iterationCount(float cr, float ci) {

	int i = 0;
	float zr=0, zi=0;
	float temp;

	while ((zr * zr + zi * zi < 4) && (i < MAX_ITERATION)) {

		temp = zr * zr - zi * zi + cr;
		zi = 2 * zr * zi + ci;
		zr = temp;
		i++;
	}

	// float m = cr * cr + ci * ci * 100;
	//i = m;

	return i;
}

int iterToColor(int i) {

	int color = 1; // BLACK by default

	if (i < MAX_ITERATION) 
		color = i % 14 + 2;  // range 2-15
	
	return color;
}

float pixel_x_to_cr(int x, float cr_min, float cr_max) {

	float cr = x / X_RES * (cr_max - cr_min) + cr_min;

	return cr;
}

float pixel_y_to_ci(int y, float ci_min, float ci_max) {

	float ci = y / Y_RES * (ci_max - ci_min) + ci_min;

	return ci;
}

void doit() {

	float cr_min = -2;
	float cr_max = 2;
	float ci_min = -2;
	float ci_max = 2;

	vdp_init(VDP_MODE_MULTICOLOR, VDP_DARK_BLUE, false, false, false);

	for (int y=0; y<48; y++) {

		float ci = pixel_y_to_ci(y, ci_min, ci_max);

		for (int x=0; x<64; x++) {

			vdp_plotColor(x, y, VDP_WHITE);

			float cr = pixel_x_to_cr(x, cr_min, cr_max);
			
			int i = iterationCount(cr, ci);
			
			int color = iterToColor(i);
			
			vdp_plotColor(x, y, color);

			if (isKeyPressed() == 0x20)
			  return;				
		}
	}

	getChar();
}

void main() {

  initNABULib();

  puts("");
  puts("Mike Debreceni Mandelbrot");
  puts("-------------------------");
  puts("");
  puts("Render the Mandelbrot Set with a NABU");
  puts("PC in 64 x 48 multicolor mode");
  puts("");
  puts("Press <SPACE> to exit during demo.");
  puts("");
  puts("Would you like to continue? (y/n)");

  while (true) {

    uint8_t c = getChar();

    if (c == 'y' || c == 'Y') {

      break;
    } else if (c == 'n' || c == 'N') {
   
      puts("Returning to CPM...");

      return;
    }
  }

  doit();

  vdp_initTextMode(VDP_WHITE, VDP_DARK_BLUE, false);
}
