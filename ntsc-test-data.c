#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>
#include <math.h>

#include "shared.h"

// USRP-TV - Using the USRP N200 as a composite video output NTSC signal generator.
// 
// It turns out that the USRP N200 with the LFTX card configured to a centerr
// frequency of 0Hz (DC) is just a 30MHz 14-bit DAC with a stereo output sample
// rate of 25Ms/s. This is really awesome.
// 
// Copyright(c) 2012 by Christopher Abad
// 
// EMAIL: aempirei@gmail.com aempirei@256.bz
// AIM: ambientempire

const int frame_scanlines = 525;
const float us_epsilon = 0.25;

const int field_scanlines = 240;

const float scanline_us = 63.50;

const float hsync_front_porch_us = 1.50;
const float hsync_sync_tip_us    = 4.75;
const float hsync_breezeway_us   = 0.50;
const float hsync_color_burst_us = 2.75;
const float hsync_back_porch_us  = 1.50;

const float Vblk = 0.30;
const float Vwhite = 1.00;
const float Vzero = 0.00;

float hsync_us;
float hdata_us;

float *frame(unsigned char *); // 525 scanlines - 480i image

float *vsync(); // 9 scanlines
float *junk(); // 0.5 scanlines @ Vblk
float *visible_field(unsigned char *); // 240 scanlines
float *vblank(); // 13 X vblank_scanline() ; 13 X [ 11us @ HSYNC ][ 52.5us @ Vblk ]

float *odd_field(unsigned char *);  // 262.5 scanlines - junk after field - 240 visible scanlines ; vsync ++ visible_field ++ junk ++ vblank
float *even_field(unsigned char *); // 262.5 scanlines - junk before field - 240 visible scanlines ; vsync ++ junk ++ visible_field ++ vblank

float *equalization_pulse(); // [ 29.25us @ Vblk ][ 2.5us @ 0V ]
float *serration_pulse();    // [ 27us @ 0V ][ 4.75us @ Vblk ]

float *pre_equalization();  // 6 X equalization-pulse
float *serration();         // 6 X serration-pulse
float *post_equalization(); // 6 X equaliation-pulse

float *hsync(); //  [ 11us @ HSYNC ] := [ 1.5us @ Vblk ][ 4.75us @ 0V ][ 0.5us @ Vblk ][ 2.75us @ Vblk ][ 1.5us @ Vblk ]
float *hdata(unsigned char *); // 52.5us @ f(t)V

float *scanline(unsigned char *); // [ 11us @ HSYNC ][ 52.5us @ f(t)V ] ; HSYNC ++ HDATA

float *vblank_scanline(); // [ 11us @ HSYNC ][ 52.5us @ Vblk ]

float btov(unsigned char); // byte to voltage [0,255] -> [Vblk,Vwhite]

void init();
void usage(const char *prog);

unsigned int hsize();
unsigned int vsize();

int main(int argc, char **argv) {

	unsigned char *data;
	unsigned char *p;
	size_t data_sz;
	size_t p_sz;
	int w, h, n;

	init();

	if(argc != 2) {
		usage(*argv);
		exit(EXIT_FAILURE);
	}

	data = loadfile(argv[1], &data_sz);

	if(data == NULL) {
		perror("loadfile()");
		exit(EXIT_FAILURE);
	}

	sscanf((char *)data, "P5 %i %i 255%n", &w, &h, &n);

	if(w != hsize()) {
		fprintf(stderr, "image has incorrect width of %d instead of %d\n", w, hsize());
		exit(EXIT_FAILURE);
	}

	if(h != vsize()) {
		fprintf(stderr, "image has incorrect height of %d instead of %d\n", h, vsize());
		exit(EXIT_FAILURE);
	}

	if(data[n] != '\n') {
		fprintf(stderr, "unexpected byte (%d) where beginning of image should be\n", data[n]);
		exit(EXIT_FAILURE);
	}

	p = data + n + 1;
	p_sz = data_sz - n - 1;

	if(p_sz != w * h) {
		fprintf(stderr, "incorrect data size of %d instead of %d\n", p_sz, w * h);
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

unsigned int hsize() {
	return (int)rint(hdata_us / us_epsilon);
}

unsigned int vsize() {
	return field_scanlines * 2;
}

void usage(const char *prog) {
	printf("\nusage: %s image\n\n", prog);
	printf("\timage must be a %dx%d grayscale pnm\n", hsize(), vsize());
	putchar('\n');
}

void init() {
	hsync_us = hsync_front_porch_us + hsync_sync_tip_us + hsync_breezeway_us + hsync_color_burst_us + hsync_back_porch_us;
	hdata_us = scanline_us - hsync_us;
}

float btov(unsigned char b) {

	float x, BLACK, WHITE, v;

	x = (float)b / 255.0;

	WHITE = x;
	BLACK = (1.0 - x);

	v = WHITE * Vwhite + BLACK * Vblk;

	return v;
}

float *frame(unsigned char *) {
	// 525 scanlines - 480i image
}

float *vsync() {
	// 9 scanlines
}

float *junk() {
	// 0.5 scanlines @ Vblk
}

float *visible_field(unsigned char *) {
	// 240 scanlines
}

float *vblank() {
	// 13 X vblank_scanline() ; 13 X [ 11us @ HSYNC ][ 52.5us @ Vblk ]
}

float *odd_field(unsigned char *) {
	// 262.5 scanlines - junk after field - 240 visible scanlines ; vsync ++ visible_field ++ junk ++ vblank
}

float *even_field(unsigned char *) {
	// 262.5 scanlines - junk before field - 240 visible scanlines ; vsync ++ junk ++ visible_field ++ vblank
}

float *equalization_pulse() {
	// [ 29.25us @ Vblk ][ 2.5us @ 0V ]
}

float *serration_pulse() {
	// [ 27us @ 0V ][ 4.75us @ Vblk ]
}

float *pre_equalization() {
	// 6 X equalization-pulse
}
float *serration() {
	// 6 X serration-pulse
}
float *post_equalization() {
	// 6 X equaliation-pulse
}

float *hsync() {
	//  [ 11us @ HSYNC ] := [ 1.5us @ Vblk ][ 4.75us @ 0V ][ 0.5us @ Vblk ][ 2.75us @ Vblk ][ 1.5us @ Vblk ]
}
float *hdata(unsigned char *) {
	// 52.5us @ f(t)V
}

float *scanline(unsigned char *) {
	// [ 11us @ HSYNC ][ 52.5us @ f(t)V ] ; HSYNC ++ HDATA
}

float *vblank_scanline() {
	// [ 11us @ HSYNC ][ 52.5us @ Vblk ]
}


