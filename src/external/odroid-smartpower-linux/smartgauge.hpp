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

#ifndef SMARTMETER_HPP_
#define SMARTMETER_HPP_

#include "hidapi.h"
#include <thread>
#include <vector>

#define MAX_STR 65

using namespace std;

class SmartGauge {

public:
	struct Measurement {
		double watt;
		double volt;
		double ampere;
		double wattHour;
	};

	SmartGauge();

	/**
	 * Initializes and resets the device.
	 * Returns false if no device could be found. True if initialization was successful.
	 * If this is done in short intervals, the hardware has troubles and sometimes delivers wrong (negativ) values.
	 */
	bool initDevice();

	/**
	 * Resets the powermeter. If this is done in short intervals, the hardware has troubles and sometimes delivers wrong (negativ) values.
	 */
	void reset();

	double getWattHour();
	double getVolt();
	double getAmpere();
	double getWatt();
	Measurement getMeasurement();
	/**
	 * interval - In which time intervals in ms is the energy consumption sampled
	 */
	void startSampling(uint intervall);

	/**
	 * Ends a measurement and returns a vector of Measurement structs.
	 *
	 */
	vector<SmartGauge::Measurement> endSampling();

	~SmartGauge();

private:
	vector<Measurement> measurements;
	hid_device* device;
	std::thread* meter;
	bool measure;
	unsigned char buf[MAX_STR];

	void collectSamples(uint intervall);
	void requestData();
	void requestStatus();
	void requestStartStop();
};

#endif /* SMARTMETER_HPP_ */
