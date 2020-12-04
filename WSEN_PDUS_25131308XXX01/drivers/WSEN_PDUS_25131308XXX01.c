/**
 ***************************************************************************************************
 * This file is part of WE sensors SDK:
 * https://www.we-online.com/sensors, https://github.com/WurthElektronik/Sensors-SDK
 *
 * THE SOFTWARE INCLUDING THE SOURCE CODE IS PROVIDED “AS IS”. YOU ACKNOWLEDGE THAT WÜRTH ELEKTRONIK
 * EISOS MAKES NO REPRESENTATIONS AND WARRANTIES OF ANY KIND RELATED TO, BUT NOT LIMITED
 * TO THE NON-INFRINGEMENT OF THIRD PARTIES’ INTELLECTUAL PROPERTY RIGHTS OR THE
 * MERCHANTABILITY OR FITNESS FOR YOUR INTENDED PURPOSE OR USAGE. WÜRTH ELEKTRONIK EISOS DOES NOT
 * WARRANT OR REPRESENT THAT ANY LICENSE, EITHER EXPRESS OR IMPLIED, IS GRANTED UNDER ANY PATENT
 * RIGHT, COPYRIGHT, MASK WORK RIGHT, OR OTHER INTELLECTUAL PROPERTY RIGHT RELATING TO ANY
 * COMBINATION, MACHINE, OR PROCESS IN WHICH THE PRODUCT IS USED. INFORMATION PUBLISHED BY
 * WÜRTH ELEKTRONIK EISOS REGARDING THIRD-PARTY PRODUCTS OR SERVICES DOES NOT CONSTITUTE A LICENSE
 * FROM WÜRTH ELEKTRONIK EISOS TO USE SUCH PRODUCTS OR SERVICES OR A WARRANTY OR ENDORSEMENT
 * THEREOF
 *
 * THIS SOURCE CODE IS PROTECTED BY A LICENSE.
 * FOR MORE INFORMATION PLEASE CAREFULLY READ THE LICENSE AGREEMENT FILE LOCATED
 * IN THE ROOT DIRECTORY OF THIS DRIVER PACKAGE.
 *
 * COPYRIGHT (c) 2019 Würth Elektronik eiSos GmbH & Co. KG
 *
 ***************************************************************************************************
 **/

#include "WSEN_PDUS_25131308XXX01.h"
#include <stdio.h>


static int8_t PDUS_I2C_read(uint8_t bytesToRead, uint8_t *valueP);


/**
* @brief  write the sensor address and read 2 or 4 bytes of data [Funtion not implemented]
* @param  Pointer to read values (16 or 32 bits) 
* @retval Error code
*/
static int8_t PDUS_I2C_read(uint8_t bytesToRead, uint8_t *valueP)
{	
	#warning "Implement the required I2C method here"
	/* 
	 * Caution: This sensor uses 5 V Vcc and Logic levels.
	 * Level conversion to 3.3 V is required to talk with a raspberry pi or any other 3.3 V MCU
	 * This sensor only supports I2C read operation and returns either 2 or 4 bytes when the sensor address is written in the I2C bus.
	 * Sending a register address is not required
	 * The first 2 bytes returned are the raw pressure value and the next 2 bytes are the raw temperature values 
	 */
	
	/* see chapter "reading digital output dat" of the PDUS user manual for the protocol */
	
	/* 
	 * 1st i2c master sends sensor's i2c address (PDUS_ADDRESS_I2C) with read bit set and waits for ACK by sensor
	 * 2nd i2c master read either 2 or 4 bytes back from the sensor; the slave (sensor) will send up to 4 bytes. master has to ACK each byte and provide clock.
	 */
	return WE_FAIL;
}

/**
* @brief  Read the pressure value
* @param  Pointer to raw pressure and temperature value (unconverted), 32 bits
* @retval Error code
*/
int8_t PDUS_readRawDataP(uint8_t *dataP)
{
  if(I2C_read(PDUS_ADDRESS_I2C, 4, dataP))
  {
    return WE_SUCCESS;
  }
  
  return WE_FAIL;
}


/**
* @brief  Read the pressure value
* @param  Pointer to raw pressure value (unconverted), 16 bits
* @retval Error code
*/
int8_t PDUS_readRawPressure(uint8_t *dataP)
{
  if(I2C_read(PDUS_ADDRESS_I2C, 2, dataP))
  {
    return WE_SUCCESS;
  }
  
  return WE_FAIL;
}


/**
* @brief  Read the pressure and temperature values
* @param  pdus sensor type (i.e. pressure ranges) for internal conversion of pressure
* @param  Pointer to pressure value
* @retval Error code
*/
int8_t PDUS_getPressure(PDUS_Sensor_Type typ, float *presskPaP)
{
	uint8_t tmp[2] = { 0 };
	uint16_t rawPres = 0, temp = 0;
	if(PDUS_ReadRawPressure(tmp) == WE_FAIL)
	{
		return WE_FAIL;
	}
	
    rawPres = (uint16_t)(tmp[0] << 8);
	rawPres |= (uint16_t)tmp[1];
	
	/* perform conversion regarding sensor sub-type */
	switch(typ)
	{
		case pdus0:
			temp = rawPres - P_MIN_VAL_PDUS;
			*presskPaP = (((float)temp * 7.63)/(1000000)) - 0.1; 
			break;
		case pdus1:
		    temp = rawPres - P_MIN_VAL_PDUS;
			*presskPaP = (((float)temp * 7.63)/(100000)) - 1; 
			break;
		case pdus2:
		    temp = rawPres - P_MIN_VAL_PDUS;
			*presskPaP = (((float)temp * 7.63)/(10000)) - 10; 
			break;
		case pdus3:
		    temp = rawPres - P_MIN_VAL_PDUS;
			*presskPaP = (((float)temp * 3.815)/(1000)); 
			break;
		case pdus4:
			temp = rawPres - P_MIN_VAL_PDUS;
			*presskPaP = (((float)temp * 4.196)/(100)) + 100; 
			break;
		default:
			return WE_FAIL;
	}
	
	return WE_SUCCESS;
}


/**
* @brief  Read the pressure and temperature values
* @param  pdus sensor type (i.e. pressure ranges) for internal conversion of pressure and temperature
* @param  Pointer to pressure value
* @param  Pointer to temperature value
* @retval Error code
*/
int8_t PDUS_getPresAndTemp(PDUS_Sensor_Type typ, float *presskPaP, float *tempDegCP)
{
	uint8_t tmp[4] = { 0 };
	uint16_t rawPres = 0, temp = 0;
	uint16_t rawTemp = 0;
	if(PDUS_readRawDataP(tmp) == WE_FAIL)
	{
		return WE_FAIL;
	}
	
    rawPres = (uint16_t)(tmp[0] << 8);
	rawPres |= (uint16_t)tmp[1];
	
	rawTemp = (uint16_t)(tmp[2] << 8);
	rawTemp |= (uint16_t)tmp[3];
	
	temp = rawTemp - T_MIN_VAL_PDUS; /* apply temperature offset to RAW temperature */
	*tempDegCP = (((float)temp * 4.272)/(1000)); /* convert RAW temperature to °C */
	
	/* perform conversion regarding sensor sub-type */
	switch(typ)
	{
		case pdus0:
			temp = rawPres - P_MIN_VAL_PDUS;
			*presskPaP = (((float)temp * 7.63)/(1000000)) - 0.1; 
			break;
		case pdus1:
		    temp = rawPres - P_MIN_VAL_PDUS;
			*presskPaP = (((float)temp * 7.63)/(100000)) - 1; 
			break;
		case pdus2:
		    temp = rawPres - P_MIN_VAL_PDUS;
			*presskPaP = (((float)temp * 7.63)/(10000)) - 10; 
			break;
		case pdus3:
		    temp = rawPres - P_MIN_VAL_PDUS;
			*presskPaP = (((float)temp * 3.815)/(1000)); 
			break;
		case pdus4:
			temp = rawPres - P_MIN_VAL_PDUS;
			*presskPaP = (((float)temp * 4.196)/(100)) + 100; 
			break;			
		default:
			return WE_FAIL;
	}
	
	return WE_SUCCESS;
}

/**         EOF         */
