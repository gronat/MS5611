/*
MS5611.h
Library for barometric pressure sensor MS5611-01BA on I2C with arduino

by Petr Gronat@2014
*/
#define OSR 					3    // 0-4
/* OSR | Oversampling Ratio | ADC Conversion Time | Resolution RMS
 *     |                    |      typ.   max.    |    mbar    C
 *   0 |               256  |     0.54    0.60    |   0.065  0.012
 *   1 |               512  |     1.06    1.17    |   0.042  0.008
 *   2 |              1024  |     2.08    2.28    |   0.027  0.005
 *   3 |              2048  |     4.13    4.54    |   0.018  0.003
 *   4 |              4096  |     8.22    9.04    |   0.012  0.002
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

// Temperature sampling period threshold [milliseconds]
// Kindly read the comment bellow in getPressure() method
#define T_THR					1000
/*
TODO:
1) Separate OSR for temperature and pressure
2) Timedelay empirical formula for temperature oversampling
   Maybe a lookup table?
3) Precidion pressure celibration for high temperature
4) Default and optional OSR
5) Documentation
*/

#include "MS5611.h"
MS5611::MS5611()
{
    _temp 		= 0;
    _pressure	= 0;
    _lastTime 	= T_THR;
    for(uint8_t k=0; k<N_PROM_PARAMS; k++)
        _calib[k]=69;
}

void MS5611::begin(){
	Wire.begin();
	reset();
	delay(100);
	readCalibration();
}

// init() function is for use with other sensors on the
// same I2C bus (Wire.begin() is done externally)
void MS5611::init(){
    reset();
    delay(10); // 5 not enough

	readCalibration();
}


/* Read digital pressure and temperature data
 * ==========================================
 */


void MS5611::triggerRawPressure(){
    sendCommand(CMD_CONV_D1_BASE + OSR*CONV_REG_SIZE);
}

uint32_t MS5611::readRawPressure(){
    sendCommand(CMD_ADC_READ); 							//get ready for reading the data
    return readnBytes(NBYTES_CONV);						//reading the data

}




void MS5611::triggerRawTemperature(){
    sendCommand(CMD_CONV_D2_BASE + OSR*CONV_REG_SIZE);
}

uint32_t MS5611::readRawTemperature(){
    sendCommand(CMD_ADC_READ); 								//get ready for reading the data
    return readnBytes(NBYTES_CONV); 						//reading the data
}



/* Calculate Temperature
 * =====================
 */

int32_t MS5611::calculateTemperature(uint32_t D2){
    // Difference between actual and reference temperature
    _dT = D2-((uint32_t)_calib[5-1] * 256);
    // Actual Temperature (-40...85C with 0.01K resolution
    // Below, 'dT' and '_calib[6-1]'' must be casted in order to prevent overflow
    // A bitwise division can not be done since it is unpredictible for signed integers
    _temp = 2000 + ((int64_t)_dT * _calib[6-1])/8388608;

    return _temp;
}


/* Calculate temperature compensated pressure
 * ==========================================
 */

int32_t MS5611::calculatePressure(uint32_t D1){
    // Offset at actual temperature
    int64_t OFF  = (int64_t)_calib[2-1]*65536
                             + (int64_t)_calib[4-1]*_dT/128;

    // Sensitivity at actual temperature
    int64_t SENS = (int64_t)_calib[1-1]*32768
                             + (int64_t)_calib[3-1]*_dT/256;

    // Temperature compensated pressure (10...1200mbar with 0.01mbar resolution)
    _pressure = (D1*SENS/2097152 - OFF)/32768;
    return _pressure;
}



/* Wrapper functions
 * =================
 */

uint32_t MS5611::getRawPressure(){
    triggerRawPressure();
    delay(1+2*OSR); 									//wait at least 8.33ms for full oversampling
    // 1  | 3  | 5  | 7  | 9     calculation above
    // 0.5| 1.1| 2.1| 4.1| 8.22  datasheet response time
    return readRawPressure();
}

uint32_t MS5611::getRawTemperature(){
    triggerRawTemperature();
    delay(1+2*OSR); 										//wait at least 8.33ms
    return readRawTemperature();
}

int32_t MS5611::getTemperature(){
    // Code below can be uncommented for slight speedup:
    // NOTE: Be sure what you do! Notice that Delta 1C ~= Delta 2hPa
    //****************
    // if(abs(millis()-_lastTime)<T_THR)
        // 	return _temp;
    //_lastTime = millis();
    //****************
    uint32_t D2;
    D2  = getRawTemperature();
    return calculateTemperature(D2);
}

// One, to rule them all:
int32_t	MS5611::getPressure(){
    getTemperature(); 		//updates temperature _dT and _temp
    uint32_t D1 = getRawPressure();

    return calculatePressure(D1);
}


// The all in one wrapper:
// without Read calibration Data
int32_t MS5611::tutorial(){
    uint32_t D2;
    uint32_t D1;
    int32_t pressure;


    triggerRawTemperature();
    delay(9);
    D2 = readRawTemperature();

    triggerRawPressure();
    delay(9);
    D1 = readRawPressure();

    calculateTemperature(D2);
    pressure = calculatePressure(D1);
    return pressure;
}

/* Initialisation and general Functions
 * ====================================
 */

void MS5611::readCalibration(){
	for(uint8_t k=0;k<6;k++){
		sendCommand(CMD_PROM_READ_BASE + k*2);
                _calib[k] = (uint16_t) (readnBytes(NBYTES_PROM) & 0xFFFF); //masking out two LSB
	}
}

void MS5611::getCalibration(uint16_t *C){
	for(uint8_t k=0;k<N_PROM_PARAMS;k++)
                C[k]=_calib[k];
	return;
}

void MS5611::sendCommand(uint8_t cmd){
	Wire.beginTransmission(ADD_MS5611);
	Wire.write(cmd);
	Wire.endTransmission();
}

uint32_t MS5611::readnBytes(uint8_t nBytes){
    if ((0<nBytes) & (nBytes<5)){
		Wire.beginTransmission(ADD_MS5611);
		Wire.requestFrom((uint8_t)ADD_MS5611, nBytes);
			uint32_t data = 0;
			if(Wire.available()!=nBytes){
				Wire.endTransmission();
				return NULL;
			}
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
