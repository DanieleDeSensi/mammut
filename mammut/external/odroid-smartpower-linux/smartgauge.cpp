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

SmartGauge::SmartGauge() :
		device(NULL), meter(NULL), measure(
				false) {
}


SmartGauge::~SmartGauge() {
    if(device){
        buf[0] = 0x00;
        memset((void*) &buf[2], 0x00, sizeof(buf) - 2);
        buf[1] = REQUEST_STARTSTOP;
        if (hid_write(device, buf, sizeof(buf)) == -1) {
            ; // Couldn't stop meter
        }
        //Meter needs some time to reset
        std::this_thread::sleep_for(std::chrono::milliseconds(POST_DELAY_MS));
        hid_close(device);
        if (meter != NULL)
            delete meter;
    }
}

void SmartGauge::requestStatus(){
	buf[0] = 0x00;
        memset((void*) &buf[2], 0x00, sizeof(buf) - 2);
	buf[1] = REQUEST_STATUS;
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
	  memset((void*) &buf[2], 0x00, sizeof(buf) - 2);
	  buf[1] = REQUEST_DATA;
	  if (hid_write(device, buf, sizeof(buf)) == -1) {
	    throw runtime_error("Request data write failed");
	  }
	  if (hid_read(device, buf, sizeof(buf)) == -1) {
	    throw runtime_error("Request data read failed");
	  }
	}
}


void SmartGauge::requestStartStop(bool started){
	if (!started) {
		buf[1] = REQUEST_STARTSTOP;
		if (hid_write(device, buf, sizeof(buf)) == -1) {
			throw runtime_error("Request start stop failed");
		}
	}

	buf[1] = REQUEST_STARTSTOP;
	if (hid_write(device, buf, sizeof(buf)) == -1) {
		throw runtime_error("Request start stop failed");
	}
	//Meter needs some time to reset
	std::this_thread::sleep_for(std::chrono::milliseconds(POST_DELAY_MS));
}

bool SmartGauge::initDevice() {
	buf[0] = 0x00;
	memset((void*) &buf[2], 0x00, sizeof(buf) - 2);
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
	buf[1] = REQUEST_STATUS;
	requestStatus();
	bool started = (buf[1] == 0x01);
	requestStartStop(started);
}

double SmartGauge::getWattHour() {
	requestData();

	if (buf[0] == REQUEST_DATA) {
		char wh[7] = { '\0' };
		strncpy(wh, (char*) &buf[26], 5);

		return atof(wh);
	}
	return -1;
}

double SmartGauge::getVolt() {
	requestData();

	if (buf[0] == REQUEST_DATA) {
		char volt[7] = { '\0' };
		strncpy(volt, (char*) &buf[2], 5);

		return atof(volt);
	}
	return -1;
}

double SmartGauge::getAmpere() {
	requestData();

	if (buf[0] == REQUEST_DATA) {
		char ampere[7] = { '\0' };
		strncpy(ampere, (char*) &buf[10], 5);

		return atof(ampere);
	}
	return -1;
}

double SmartGauge::getWatt() {
	requestData();

	if (buf[0] == REQUEST_DATA) {
		char watt[7] = { '\0' };
		strncpy(watt, (char*) &buf[18], 5);

		return atof(watt);
	}
	return -1;
}

SmartGauge::Measurement SmartGauge::getMeasurement() {
	requestData();
	Measurement measurement;
	if (buf[0] == REQUEST_DATA) {
		char volt[7] = { '\0' };
		strncpy(volt, (char*) &buf[2], 5);
		char ampere[7] = { '\0' };
		strncpy(ampere, (char*) &buf[10], 5);
		char watt[7] = { '\0' };
		strncpy(watt, (char*) &buf[18], 5);
		char wh[7] = { '\0' };
		strncpy(wh, (char*) &buf[26], 5);

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
			char volt[7] = { '\0' };
			strncpy(volt, (char*) &buf[2], 5);
			char ampere[7] = { '\0' };
			strncpy(ampere, (char*) &buf[10], 5);
			char watt[7] = { '\0' };
			strncpy(watt, (char*) &buf[18], 5);
			char wh[7] = { '\0' };
			strncpy(wh, (char*) &buf[26], 5);

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
