/*
 * Copyright (c) 2013 Robert Seilbeck. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef H_SMARTMETER
#define H_SMARTMETER

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fstream>
#include <thread>
#include <cassert>
#include <chrono>
#include <stdexcept>
#include "smartgauge.hpp"

using namespace std;

#define REQUEST_DATA        0x37
#define REQUEST_STARTSTOP   0x80
#define REQUEST_STATUS      0x81
#define REQUEST_ONOFF       0x82
#define REQUEST_VERSION     0x83
#define POST_DELAY_MS       100

SmartGauge::SmartGauge():
		device(NULL), meter(NULL), measure(false) {
  ;
}


SmartGauge::~SmartGauge() {
    if(device){
        buf[0] = 0x00;
        memset((void*) &buf[2], 0x00, sizeof(buf) - 2);
	requestStartStop();
        hid_close(device);
        if (meter != NULL)
            delete meter;
    }
}

void SmartGauge::requestStatus(){
	buf[0] = 0x00;
	buf[1] = REQUEST_STATUS;
	memset((void*) &buf[2], 0x00, sizeof(buf) - 2);
	if (hid_write(device, buf, sizeof(buf)) == -1) {
		throw runtime_error("Request status write failed");
	}
	if (hid_read(device, buf, sizeof(buf)) == -1) {
		throw runtime_error("Request status read failed");
	}
}


void SmartGauge::requestData(){
	buf[0] = 0x00;
	while(buf[0] != REQUEST_DATA){
	  buf[1] = REQUEST_DATA;
	  memset((void*) &buf[2], 0x00, sizeof(buf) - 2);
	  if (hid_write(device, buf, sizeof(buf)) == -1) {
	    throw runtime_error("Request data write failed");
	  }
	  if (hid_read(device, buf, sizeof(buf)) == -1) {
	    throw runtime_error("Request data read failed");
	  }
	  //std::this_thread::sleep_for(std::chrono::milliseconds(POST_DELAY_MS));
	}
}


void SmartGauge::requestStartStop(){
	buf[0] = 0x00;
	buf[1] = REQUEST_STARTSTOP;
	memset((void*) &buf[2], 0x00, sizeof(buf) - 2);
	if (hid_write(device, buf, sizeof(buf)) == -1) {
		throw runtime_error("Request start stop failed");
	}
	//Meter needs some time to reset
	std::this_thread::sleep_for(std::chrono::milliseconds(POST_DELAY_MS));
}

bool SmartGauge::initDevice() {
	device = hid_open(0x04d8, 0x003f, NULL);
	if (device) {
		hid_set_nonblocking(device, false);
	} else {
		return false;
	}
	reset();
	return true;
}

void SmartGauge::reset() {
	requestStatus();
	bool started = (buf[1] == 0x01);
	assert(buf[2] == 0x01);
	if(started){
	  requestStartStop();
	}	
	requestStartStop();
}

double SmartGauge::getWattHour() {
	requestData();

	if (buf[0] == REQUEST_DATA) {
		char wh[8] = { '\0' };
		strncpy(wh, (char*) &buf[24], 7);

		return atof(wh);
	}
	return -1;
}

double SmartGauge::getVolt() {
	requestData();

	if (buf[0] == REQUEST_DATA) {
		char volt[8] = { '\0' };
		strncpy(volt, (char*) &buf[2], 5);

		return atof(volt);
	}
	return -1;
}

double SmartGauge::getAmpere() {
	requestData();

	if (buf[0] == REQUEST_DATA) {
		char ampere[8] = { '\0' };
		strncpy(ampere, (char*) &buf[10], 5);

		return atof(ampere);
	}
	return -1;
}

double SmartGauge::getWatt() {
	requestData();

	if (buf[0] == REQUEST_DATA) {
		char watt[8] = { '\0' };
		strncpy(watt, (char*) &buf[17], 6);

		return atof(watt);
	}
	return -1;
}

SmartGauge::Measurement SmartGauge::getMeasurement() {
	requestData();
	Measurement measurement;
	if (buf[0] == REQUEST_DATA) {
		char volt[8] = { '\0' };
		strncpy(volt, (char*) &buf[2], 5);
		char ampere[8] = { '\0' };
		strncpy(ampere, (char*) &buf[10], 5);
		char watt[8] = { '\0' };
		strncpy(watt, (char*) &buf[17], 6);
		char wh[8] = { '\0' };
		strncpy(wh, (char*) &buf[24], 7);

		measurement.volt = atof(volt);
		measurement.ampere = atof(ampere);
		measurement.watt = atof(watt);
		measurement.wattHour = atof(wh);
	}
	return measurement;
}


void SmartGauge::startSampling(	uint intervall) {
	measurements.clear();
	if (measure == true) {
		throw runtime_error("An old measurement still runs. End it first before starting a new one.");
		return;
	}
	measure = true;
	meter = new thread(&SmartGauge::collectSamples, this, intervall);
}

vector<SmartGauge::Measurement> SmartGauge::endSampling() {
	measure = false;
	meter->join();
	return measurements;
}

void SmartGauge::collectSamples(uint intervall) {
	while (measure) {
		Measurement measurement;
		requestData();
		if (buf[0] == REQUEST_DATA) {
			char volt[8] = { '\0' };
			strncpy(volt, (char*) &buf[2], 5);
			char ampere[8] = { '\0' };
			strncpy(ampere, (char*) &buf[10], 5);
			char watt[8] = { '\0' };
			strncpy(watt, (char*) &buf[17], 6);
			char wh[8] = { '\0' };
			strncpy(wh, (char*) &buf[24], 7);

			measurement.volt = atof(volt);
			measurement.ampere = atof(ampere);
			measurement.watt = atof(watt);
			measurement.wattHour = atof(wh);
		}
		measurements.push_back(measurement);
		std::this_thread::sleep_for(std::chrono::milliseconds(intervall));
	}
}

#endif
