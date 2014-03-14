/*
MS5611.h
Library for barometric pressure sensor SM5611 on I2C with arduino

by Petr Gronat@2014
*/
#define CMD_RESET 				0x1E
#define CMD_ADC_READ			0x00
#define CMD_CONV_D1_BASE 		0x40
#define CMD_CONV_D2_BASE 		0x50
#define CONV_REG_SIZE 			0x02
#define CMD_PROM_READ_BASE		0xA2
#define PROM_REG_SIZE			0x02
#define NBYTES_CONV 			3
#define NBYTES_PROM 			2

// temperature sampling period threshold [milliseconds]
#define T_THR					1000;
/*
TODO:
Defaul values:
Oversampling ratio for temperature and pressure
To be able to modify time threshold for temperature update T_THR
*/

#include "MS5611.h"
MS5611::MS5611(){
	_T 		= 0;
	_P 		= 0;
	_lastTime 	= T_THR;
	for(uint8_t k=0;k<N_PROM_PARAMS;k++) _C[k]=0;
}


void MS5611::begin(){
	reset();
	delay(100);
	readCalibration();
}

int32_t	MS5611::getPressure(){
	getTemperature(); 		//update temperature _dT and _T
	uint32_t D1 = getRawPressure();
	Serial.println("fooo");
	Serial.print("D1: "); Serial.println( D1 );
	Serial.print("dT: "); Serial.println( _dT );
	
	// int64_t OFF  = (int64_t)_C[2-1]*(int64_t)pow(2,16) 
	// 			 + (int64_t)_C[4-1]*(int64_t)_dT/(int64_t)pow(2,7);
	int64_t OFF  = (int64_t)_C[2-1]*65536 
				 + (int64_t)_C[4-1]*_dT/128;
	Serial.print("OFF: "); Serial.println( (int32_t) (OFF/65536 ) );
	
	int64_t SENS = (int64_t)_C[1-1]*32768 
				 + (int64_t)_C[3-1]*_dT/256;
	Serial.print("SENS: "); Serial.println( (int32_t) (SENS) );
	
	//OFF = (long long)_C[2-1] * 65536 + ((long long)_C[4-1] * _dT ) / 128;
    //SENS = (long long)_C[1-1] * 32768 + ((long long)_C[3-1] * _dT) / 256;
	//_P 	 = ( ((int64_t)D1*SENS)/2097152-OFF)/32768;
	_P = (D1*SENS/2097152 - OFF)/32768;

	return _P;
}

uint32_t MS5611::getRawPressure(){
	sendCommand(CMD_CONV_D2_BASE+0*CONV_REG_SIZE);	//read sensor, prepare a data
	delay(9); 										//wait at least 8.33us
	sendCommand(CMD_ADC_READ); 						//get ready for reading the data
	return readnBytes(NBYTES_CONV);		//reading the data
}

int32_t MS5611::getTemperature(){
	//if (abs(millis()-_lastTime)<T_THR)
	//	return _T;
	_lastTime = millis();
	uint32_t 	D2 = getRawTemperature();
	_dT = D2-((uint32_t)_C[5-1] * 256); 		//update '_dT'
	// Below, 'dT' and '_C[6-1]'' must be casted in order to prevent overflow
	// bitwise division is unpredictioble for signed integers
	_T = 2000 + ((int64_t)_dT * _C[6-1])/8388608;
	return _T;
}


uint32_t MS5611::getRawTemperature(){	
	sendCommand(CMD_CONV_D2_BASE+0*CONV_REG_SIZE);		//read sensor, prepare a data
	delay(9); 						//wait at least 8.33us
	sendCommand(CMD_ADC_READ); 		//get ready for reading the data
	return readnBytes(NBYTES_CONV); //reading the data
}

void MS5611::readCalibration(){
	for(uint8_t k=0;k<6;k++){
		sendCommand(CMD_PROM_READ_BASE + k*2);
		_C[k] = (uint16_t) (readnBytes(NBYTES_PROM) & 0xFFFF); //masking out two LSB
	}
}

void MS5611::getCalibration(uint16_t *C){
	for(uint8_t k=0;k<N_PROM_PARAMS;k++)
		C[k]=_C[k];
	return;
}

void MS5611::sendCommand(uint8_t cmd){
	Wire.beginTransmission(ADD_MS5611);
	Wire.write(cmd);
	Wire.endTransmission();
}

uint32_t MS5611::readnBytes(uint8_t nBytes){
	if (0<nBytes & nBytes<5){	
		Wire.beginTransmission(ADD_MS5611);
		Wire.requestFrom((uint8_t)ADD_MS5611, nBytes);
			uint32_t data = 0;
			if(Wire.available()!=nBytes)
				return NULL; 									// device unavailable or other error
			for (int8_t k=nBytes-1; k>=0; k--)
				data |= ( (uint32_t) Wire.read() << (8*k) ); 	// concantenate bytes
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
