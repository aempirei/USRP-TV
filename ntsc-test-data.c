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
// frequency of 0Hz (DC) is just a 30MHz 14-bit DAC with a stereo output sample
// rate of 25Ms/s. This is really awesome.
// 
// Copyright(c) 2012 by Christopher Abad
// 
// EMAIL: aempirei@gmail.com aempirei@256.bz
// AIM: ambientempire

const size_t frame_scanlines = 525;
const size_t field_scanlines = 240;
const size_t vblank_scanlines = 13;
const size_t vsync_scanlines = 9;

const float half_frame_scanlines = 252.5;
const float junk_scanlines = 0.5;

const float scanline_us = 63.50;

const float us_epsilon = 0.25;

const float hsync_front_porch_us = 1.50;
const float hsync_sync_tip_us = 4.75;
const float hsync_breezeway_us = 0.50;
const float hsync_color_burst_us = 2.75;
const float hsync_back_porch_us = 1.50;

const float equalization_pulse_on = 29.25;
const float equalization_pulse_off = 2.50;

const float serration_pulse_off = 29.25; // 27.00;
const float serration_pulse_on = 2.50; // 4.75;

const float Vblack = 0.30;
const float Vwhite = 1.00;
const float Vzero = 0.00;

float hsync_us;
float hdata_us;

size_t vsync_sz;
size_t field_sz;
size_t junk_sz;
size_t vblank_sz;

float *frame(unsigned char *);  // 525 scanlines - 480i image

float *vsync();                 // 9 scanlines
float *junk();                  // 0.5 scanlines @ Vblack
float *visible_field(unsigned char *);  // 240 scanlines
float *vblank();                // 13 X vblank_scanline() ; 13 X [ 11us @ HSYNC ][ 52.5us @ Vblack ]

float *odd_field(unsigned char *);      // 262.5 scanlines - junk after field - 240 visible scanlines ; vsync ++ visible_field ++ junk ++ vblank
float *even_field(unsigned char *);     // 262.5 scanlines - junk before field - 240 visible scanlines ; vsync ++ junk ++ visible_field ++ vblank

float *equalization_pulse();    // [ 29.25us @ Vblack ][ 2.5us @ 0V ]
float *serration_pulse();       // [ 27us @ 0V ][ 4.75us @ Vblack ]

float *pre_equalization();      // 6 X equalization-pulse
float *serration();             // 6 X serration-pulse
float *post_equalization();     // 6 X equaliation-pulse

float *hsync();                 //  [ 11us @ HSYNC ] := [ 1.5us @ Vblack ][ 4.75us @ 0V ][ 0.5us @ Vblack ][ 2.75us @ Vblack ][ 1.5us @ Vblack ]
float *hdata(unsigned char *);  // 52.5us @ f(t)V

float *scanline(unsigned char *);       // [ 11us @ HSYNC ][ 52.5us @ f(t)V ] ; HSYNC ++ HDATA

float *vblank_scanline();       // [ 11us @ HSYNC ][ 52.5us @ Vblack ]

float btov(unsigned char);      // byte to voltage [0,255] -> [Vblack,Vwhite]

void init();
void usage(const char *prog);

size_t hsize();
size_t vsize();
size_t scanlinesize();

float *samplealloc(float, size_t *);

size_t tosamples(float);

int main(int argc, char **argv) {

    unsigned char *data;
    unsigned char *p;
    size_t data_sz;
    size_t p_sz;
    int w, h, n;

    float *frame_data;

    init();

    if (argc != 2) {
        usage(*argv);
        exit(EXIT_FAILURE);
    }

    data = loadfile(argv[1], &data_sz);

    if (data == NULL) {
        perror("loadfile()");
        exit(EXIT_FAILURE);
    }

    sscanf((char *)data, "P5 %i %i 255%n", &w, &h, &n);

    if (w != hsize()) {
        fprintf(stderr, "image has incorrect width of %d instead of %d\n", w, hsize());
        exit(EXIT_FAILURE);
    }

    if (h != vsize()) {
        fprintf(stderr, "image has incorrect height of %d instead of %d\n", h, vsize());
        exit(EXIT_FAILURE);
    }

    if (data[n] != '\n') {
        fprintf(stderr, "unexpected byte (%d) where beginning of image should be\n", data[n]);
        exit(EXIT_FAILURE);
    }

    p = data + n + 1;
    p_sz = data_sz - n - 1;

    if (p_sz != w * h) {
        fprintf(stderr, "incorrect data size of %d instead of %d\n", p_sz, w * h);
        exit(EXIT_FAILURE);
    }

    frame_data = frame(p);

    if (frame_data != NULL) {
        int sz = sizeof(float) * frame_scanlines * scanlinesize();

        n = write2(1, frame_data, sz);
        if (n != sz) {
            perror("write2()");
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "missing output frame\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

size_t hsize() {
    return tosamples(hdata_us);
}

size_t vsize() {
    return field_scanlines * 2;
}

size_t scanlinesize() {
    return tosamples(scanline_us);
}

void usage(const char *prog) {
    printf("\nusage: %s image\n\n", prog);
    printf("\timage must be a %dx%d grayscale pnm\n", hsize(), vsize());
    putchar('\n');
}

void init() {

    hsync_us = hsync_front_porch_us + hsync_sync_tip_us + hsync_breezeway_us + hsync_color_burst_us + hsync_back_porch_us;
	 hdata_us = scanline_us - hsync_us;

	 vsync_sz = tosamples(scanline_us * vsync_scanlines);
	 field_sz = tosamples(scanline_us * field_scanlines);
	 junk_sz = tosamples(scanline_us * junk_scanlines);
	 vblank_sz = tosamples(scanline_us * vblank_scanlines);

}

float btov(unsigned char b) {

    float x, BLACK, WHITE, v;

    x = (float)b / 255.0;

    WHITE = x;
    BLACK = (1.0 - x);

    v = WHITE * Vwhite + BLACK * Vblack;

    return v;
}

float *frame(unsigned char *data) {

    // 525 scanlines - 480i image

    size_t half_frame_sz = tosamples(scanline_us * half_frame_scanlines);

    size_t sz;
    float *p;

    p = samplealloc(scanline_us * frame_scanlines, &sz);

    memcpy(p                , odd_field (data          ), half_frame_sz * sizeof(float));
    memcpy(p + half_frame_sz, even_field(data + hsize()), half_frame_sz * sizeof(float));

    return p;

}

float *samplealloc(float us, size_t * sz) {
    *sz = tosamples(us);
    return (float *)malloc(sizeof(float) * *sz);
}

float *vsync() {

    static int flag = 1;
    static size_t sz;
    static float *p;

    // 9 scanlines

    if (flag) {
        flag = 0;
        p = samplealloc(scanline_us * vsync_scanlines, &sz);
        memcpy(p                     , pre_equalization() , 3 * scanlinesize() * sizeof(float));
        memcpy(p + 3 * scanlinesize(), serration()        , 3 * scanlinesize() * sizeof(float));
        memcpy(p + 6 * scanlinesize(), post_equalization(), 3 * scanlinesize() * sizeof(float));
    }

    return p;
}

float *junk() {

    // 0.5 scanlines @ Vblack

    static int flag = 1;
    static size_t sz;
    static float *p;

    if (flag) {

        int n;

        flag = 0;

        p = samplealloc(scanline_us * 0.5, &sz);

        for (n = 0; n < sz; n++)
            p[n] = Vblack;
    }

    return p;
}

size_t tosamples(float us) {
    return (int)rint(us / us_epsilon);
}

float *visible_field(unsigned char *data) {

    // 240 scanlines x scanline() ; 240 X [ 11us @ HSYNC ][ 52.5us @ f(t)V ]

    size_t sz;
    float *p;

    int n;

    p = samplealloc(scanline_us * field_scanlines, &sz);

    for (n = 0; n < field_scanlines; n++)
        memcpy(p + n * scanlinesize(), scanline(data + n * 2 * hsize()), sizeof(float) * scanlinesize());

    return p;

}

float *vblank() {

    // 13 X vblank_scanline() ; 13 X [ 11us @ HSYNC ][ 52.5us @ Vblack ]

    static int flag = 1;
    static size_t sz;
    static float *p;

    if (flag) {

        int n;

        flag = 0;

        p = samplealloc(scanline_us * vblank_scanlines, &sz);

        for (n = 0; n < vblank_scanlines; n++)
            memcpy(p + n * scanlinesize(), vblank_scanline(), sizeof(float) * scanlinesize());
    }

    return p;

}

float *odd_field(unsigned char *data) {

    // 262.5 scanlines - junk after field - 240 visible scanlines ; vsync ++ visible_field ++ junk ++ vblank

    size_t sz;
    float *p;

    p = samplealloc(scanline_us * half_frame_scanlines, &sz);

    memcpy(p                                , vsync()            , sizeof(float) * vsync_sz);
    memcpy(p + vsync_sz                     , vblank()           , sizeof(float) * vblank_sz);
    memcpy(p + vsync_sz + vblank_sz         , vblank()           , sizeof(float) * vblank_sz);
    memcpy(p + vsync_sz + junk_sz + field_sz, vblank()           , sizeof(float) * vblank_sz);

    // memcpy(p + vsync_sz                      , junk()             , sizeof(float) * junk_sz);

    memcpy(p + vsync_sz + tosamples(9.0 * scanline_us), visible_field(data), sizeof(float) * field_sz);

    return p;
}

float *even_field(unsigned char *data) {

    // 262.5 scanlines - junk before field - 240 visible scanlines ; vsync ++ junk ++ visible_field ++ vblank

    size_t sz;
    float *p;

    p = samplealloc(scanline_us * half_frame_scanlines, &sz);

    memcpy(p                                , vsync()            , sizeof(float) * vsync_sz);
    memcpy(p + vsync_sz                     , vblank()           , sizeof(float) * vblank_sz);
    memcpy(p + vsync_sz + vblank_sz         , vblank()           , sizeof(float) * vblank_sz);
    memcpy(p + vsync_sz + junk_sz + field_sz, vblank()           , sizeof(float) * vblank_sz);

    // memcpy(p + vsync_sz                     , junk()                       , sizeof(float) * junk_sz);

    memcpy(p + vsync_sz + tosamples(21.5 * scanline_us), visible_field(data + hsize()), sizeof(float) * field_sz);

    return p;

}

float *equalization_pulse() {

    // [ 29.25us @ Vblack ][ 2.5us @ 0V ]

    static int flag = 1;
    static size_t sz;
    static float *p;

    if (flag) {

        int n;
		  int m;

        flag = 0;

        p = samplealloc(scanline_us * 0.5, &sz);

		  m = 0;

        for (n = 0; n < tosamples(equalization_pulse_on); n++, m++) p[m] = Vblack;
        for (n = 0; n < tosamples(equalization_pulse_off); n++, m++) p[m] = Vzero;
    }

    return p;
}

float *serration_pulse() {
    // [ 27us @ 0V ][ 4.75us @ Vblack ]

    static int flag = 1;
    static size_t sz;
    static float *p;

    if (flag) {

        int n;
		  int m;

        flag = 0;

        p = samplealloc(scanline_us * 0.5, &sz);

		  m = 0;

        for (n = 0; n < tosamples(serration_pulse_off); n++, m++) p[m] = Vzero;
        for (n = 0; n < tosamples(serration_pulse_on); n++, m++) p[m] = Vblack;
    }

    return p;

}

float *pre_equalization() {
    // 6 X equalization-pulse
    static int flag = 1;
    static size_t sz;
    static float *p;

    // 3 scanlines
    if (flag) {
        int n;

        flag = 0;
        p = samplealloc(scanline_us * 3, &sz);

        for (n = 0; n < 6; n++)
            memcpy(p + (n * scanlinesize() / 2), equalization_pulse(), sizeof(float) * scanlinesize() / 2);
    }

    return p;
}

float *serration() {
    // 6 X serration-pulse
    static int flag = 1;
    static size_t sz;
    static float *p;

    // 3 scanlines
    if (flag) {
        int n;

        flag = 0;
        p = samplealloc(scanline_us * 3, &sz);

        for (n = 0; n < 6; n++)
            memcpy(p + (n * scanlinesize() / 2), serration_pulse(), sizeof(float) * scanlinesize() / 2);
    }

    return p;

}

float *post_equalization() {
    // 6 X equaliation-pulse
    return pre_equalization();
}

float *hsync() {

    //  [ 11us @ HSYNC ] := [ 1.5us @ Vblack ][ 4.75us @ 0V ][ 0.5us @ Vblack ][ 2.75us @ Vblack ][ 1.5us @ Vblack ]

    static int flag = 1;
    static size_t sz;
    static float *p;

    if (flag) {

        int n, m;

        flag = 0;

        p = samplealloc(hsync_us, &sz);

        m = 0;

        for (n = 0; n < tosamples(hsync_front_porch_us); n++, m++) p[m] = Vblack;
        for (n = 0; n < tosamples(hsync_sync_tip_us   ); n++, m++) p[m] = Vzero;
        for (n = 0; n < tosamples(hsync_breezeway_us  ); n++, m++) p[m] = Vblack;
        for (n = 0; n < tosamples(hsync_color_burst_us); n++, m++) p[m] = Vblack;
        for (n = 0; n < tosamples(hsync_back_porch_us ); n++, m++) p[m] = Vblack;
    }

    return p;
}

float *hdata(unsigned char *data) {

    // 52.5us @ f(t)V

    size_t sz;
    float *p;

    int n;

    p = samplealloc(hdata_us, &sz);

    for (n = 0; n < sz; n++)
        p[n] = btov(data[n]);

    return p;
}

float *scanline(unsigned char *data) {

    // [ 11us @ HSYNC ][ 52.5us @ f(t)V ] ; HSYNC ++ HDATA

    size_t sz;
    float *p;

    p = samplealloc(scanline_us, &sz);

    memcpy(p, hsync(), sizeof(float) * tosamples(hsync_us));
    memcpy(p + tosamples(hsync_us), hdata(data), sizeof(float) * tosamples(hdata_us));

    return p;
}

float *vblank_scanline() {

    // [ 11us @ HSYNC ][ 52.5us @ Vblack ] ; HSYNC ++ Vblack*

    static int flag = 1;
    static size_t sz;
    static float *p;

    if (flag) {
        int n;

        flag = 0;
        p = samplealloc(scanline_us, &sz);

        for (n = 0; n < sz; n++)
            p[n] = Vblack;

        memcpy(p, hsync(), sizeof(float) * tosamples(hsync_us));
    }

    return p;
}
