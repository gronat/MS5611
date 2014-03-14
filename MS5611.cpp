/*
MS5611.h
Library for barometric pressure sensor SM5611 on I2C with arduino

by Petr Gronat@2014
*/
#define CMD_RESET 				0x1E
#define CMD_ADC_READ			0x00
#define CMD_CONV_D1_BASE 		0x40
#define CMD_CONV_D2_BASE 		0x50
#define CMD_CONV_D_STEP 		0x02
#define CMD_PROM_READ_BASE		0xA0
#define CMD_PROM_STEP 			0x02
#define NBYTES_CONV 			3
#define NBYTES_PROM 			2

// temperature sampling period threshold [milliseconds]
#define T_THR					1000


#include "MS5611.h"
MS5611::MS5611(){
	int32_t 	_TEMP = 0;
	int16_t 	_lastTime = -T_THR;
	uint16_t 	_C[6];
	for(uint8_t k=0;k<6;k++)
		_C[k]=0;
}

void MS5611::begin(){
	reset();
	delay(100);
	readCalibration();
}

int32_t MS5611::getTemperature(){
	if( (millis-lastTime)<T_THR )
		return _TEMP;
	uint32_t 	D2 = getRawTemperature();
	int32_t 	dT = (D2-C[5-1]) << 8; 		//int<<8   <==> int*2^8
	/* 
		Here we can't do the bit operation division
		trick because _TEMP is signed integer.
	*/
	_TEMP=(20000+dT*C[6-1])/(1<<23); 		//1<<23  <==> 2^23	
	return _TEMP
}

void MS5611::readCalibration(){
	for(uint8_t k=0;k<6;k++){
		sendCommand(CMPD_PROM_READ + k*CMD_PROM_STEP);
		_C[k] = (uint16_t) readnBytes(NBYTES_PROM) & 0xFFFF; //masking out two LSB
	}
}

void MS5611::getCalibration(uint16_t *C){
	for(uint8_t k=0;k<0;k++)
		C[k]=_C[k];
}

uint32_t MS5611::getRawTemperature(){	
	sendCommand(CMD_CONV_D1_BASE + 3*CMD_CONV_D_STEP);	//read sensor, prepare a data
	delay(9); 						//wait at least 8.33us
	sendCommand(CMD_ADC_READ); 		//get ready for reading the data
	return readnBytes(NBYTES_CONV); 					//reading the data
}

uint32_t MS5611::getRawPressure(){
	sendCommand(CMD_CONV_D2_BASE + 3*CMD_CONV_D_STEP);	//read sensor, prepare a data
	delay(9); 						//wait at least 8.33us
	sendCommand(CMD_ADC_READ); 		//get ready for reading the data
	return readnBytes(NBYTES_CONV);	//reading the data
}

void MS6511::sendCommand(uint8_t cmd){
	Wire.beginTransmission(ADD_MS5611);
	Wire.write(cmd);
	Wire.endTransmission();
}

uint32_t MS6511::readnBytes(uint8_t nBytes){
	if (nBytes<5 & nBytes>0){	
		uint32_t data = 99;
		Wire.beginTransmission(ADD_MS5611);
		Wire.requestFrom(ADD_MS5611, nBytes);
		// read bytes, MSB comes first
		for(uint8_t k; k<nBytes; k++){
			if (Wire.available()==0)
				return NULL; 						// error, e.g. device unplugged
			data |= Wire.read()<<8*(nBytes-k-1);	// shifting MSB to the left
		}
		Wire.endTransmission();
		return data;
	}												// too many bytes or
	return NULL; 									// no byte required
}

void MS5611::reset(){
	Wire.beginTransmission(ADD_MS5611);
	Wire.write(CMD_RESET);
	Wire.endTransmission();
}
