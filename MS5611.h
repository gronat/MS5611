/*
ms5611.h
Library for barometric pressure sensor MS5611-01BA on I2C with arduino

by Petr Gronat@2014
*/

// Include guard token - prevents to include header file twice
#ifndef MS5611_h
#define MS5611_h 	//create token

// Include Arduino libraries
#include "Arduino.h"
#include <Wire.h>

#define N_PROM_PARAMS 6


// address of the device MS5611
#define ADD_MS5611 0x77
// 0x77 if CSB pin is connected to GND
// 0x76 if CSB pin is connected to VCC

class MS5611{
	public:
		MS5611();		//constructor
			void 		begin();
			void            init();
			// Read digital pressure and temperature data
			void        triggerRawTemperature();
			uint32_t    readRawTemperature();
			uint32_t 	getRawTemperature();
			void        triggerRawPressure();
			uint32_t    readRawPressure();
			uint32_t 	getRawPressure();
			// Calculate Temperature
			int32_t     calculateTemperature(uint32_t D2);
			int32_t     calculatePressure(uint32_t D1);
			int32_t 	getTemperature();
			int32_t 	getPressure();
			int32_t     tutorial();
			void 		readCalibration();
			void 		getCalibration(uint16_t *);
			void 		sendCommand(uint8_t);
			uint32_t 	readnBytes(uint8_t);
	private:
			void 		reset();
		//variables
		int32_t 	_pressure;
		int32_t  	_temp;
		int32_t 	_dT;
		uint16_t 	_calib[N_PROM_PARAMS];
		uint32_t 	_lastTime;
};

#endif
