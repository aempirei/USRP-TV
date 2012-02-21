#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>
#include <math.h>

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

float *vsync(); // 9 scanlines
float *junk(); // 0.5 @ Vblk
float *vblank(); // 13 X vblank_scanline() ; 13 X [ 11us @ HSYNC ][ 52.5us @ Vblk ]

float *odd_field(unsigned char *); // 262.5 - junk after field
float *even_field(unsigned char *); // 262.5 - junk before field

float *equalization_pulse(); // [ 29.25us @ Vblk ][ 2.5us @ 0V ]
float *serration_pulse(); // [ 27us @ 0V ][ 4.75us @ Vblk ]

float *pre_equalization(); // 6 X equalization-pulse
float *serration(); // 6 X serration-pulse
float *post_equalization(); // 6 X equaliation-pulse

float *hdata(unsigned char *); // 52.5us @ f(t)V

float *hsync(); //  [ 11us @ HSYNC ] := [ 1.5us @ Vblk ][ 4.75us @ 0V ][ 0.5us @ Vblk ][ 2.75us @ Vblk ][ 1.5us @ Vblk ]

const int field_data_scanlines = 240;

const float scanline_us = 63.50;

const float hsync_front_porch_us = 1.50;
const float hsync_sync_tip_us    = 4.75;
const float hsync_breezeway_us   = 0.50;
const float hsync_color_burst_us = 2.75;
const float hsync_back_porch_us  = 1.50;

const float hsync_us = hsync_front_porch_us + hsync_sync_tip_us + hsync_breezeway_us + hsync_color_burst_us + hsync_back_porch_us;

const float hdata_us = scanline_us - hsync_us;

float *scanline(unsigned char *); // [ 11us @ HSYNC ][ 52.5us @ f(t)V ]

float *vblank_scanline(); // [ 11us @ HSYNC ][ 52.5us @ Vblk ]

void usage(const char *prog) {
}

int main(int argc, char **argv) {

	exit(EXIT_SUCCESS);
}

