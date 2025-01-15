#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h> 
#include "util.h"
#include "mw/mw-msg.h"

#define PIN 48
#define NUMPIXELS 1
#define PIXEL_INDEX 0

class Led {
public:

	Led(){};

	Led(MwMsgSysStat *status_p){
		status = status_p;
	};

	Adafruit_NeoPixel* pixels = new Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
	MwMsgSysStat *status = NULL;

	bool enable = FALSE;

	void led_toggle();

	void led_init();
private:
};