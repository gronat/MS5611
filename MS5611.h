/*
ms5611.h
Library for barometric pressure sensor SM5611 on I2C with arduino

by Petr Gronat@2014
*/

// Include guard token - prevents to include header file twice
#ifndef MS5611_h
#define MS5611_h 	//create token

// Include Arduino libraries
#include "Arduino.h"
#include <Wire.h>


// address of the device MS5611
#define ADD_MS5611 0x77 	// can be 0x76 if CSB pin is connected to GND

class MS5611{
	public:
		MS5611();		//constructor
			void 		begin();
			int32_t 	getTemperature();
			void 		readCalibration();
			void 		getCalibration(uint16_t *);
			uint32_t 	getRawTemperature();
			uint32_t 	getRawPressure();
	private:
			void 		reset();
			void 		sendCommand(uint8_t);
			uint32_t 	readnBytes(uint8_t);
		//variables
		int32_t  	_TEMP;
		uint16_t* 	_C;
		int16_t 	_lastTime;
};

#endif