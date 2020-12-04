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
 * COPYRIGHT (c) 2020 Würth Elektronik eiSos GmbH & Co. KG
 *
 ***************************************************************************************************
 **/

#include "platform.h"
#include <wiringPiI2C.h>
#include <wiringPiSPI.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


#define DUMMY 0x00

#ifdef USE_SPI

static int spi_channel = 1;
static int spi_handle = 0; // global handle to the  spi interface

#else /* I2C is used */

static int i2c_handle = 0; // global handle to the  i2c interface

#endif // USE_SPI

/**
* @brief  Initialize the I2C Interface
* @param  I2C address
* @retval Error Code
*/
int8_t I2CInit(int address)
{
#ifdef USE_SPI
	return WE_FAIL;
	
#else /* I2C is used */
  if ((i2c_handle = wiringPiI2CSetup(address)) < 0)
  {
    fprintf(stdout, "wiringPiI2CSetup Error\n");
    return WE_FAIL;
  }

  return WE_SUCCESS;
#endif
}


int8_t SpiInit(int channel)
{
#ifdef USE_SPI
  int speed = 4000000; /* 4MHz */
  int spi_mode = 0;
  spi_channel = channel;

  if ((spi_handle = wiringPiSPISetupMode(spi_channel, speed, spi_mode)) < 0)
  {
    fprintf(stdout, "wiringPiSPISetup Error\n");
    return WE_FAIL;
  }
  return WE_SUCCESS;

#else /* I2C is used */
  return WE_FAIL;
#endif
}



/**
* @brief   Read data starting from the addressed register
* @param  -RegAdr : the register address to read from
*         -NumByteToRead : number of bytes to read
*         -pointer Data : the address store the data
* @retval Error Code
*/
int8_t ReadReg(uint8_t RegAdr, int NumByteToRead, uint8_t *Data)
{
#ifdef USE_SPI
  RegAdr |= SPI_READ_MASK;
  uint8_t fulldata[2];
  int i=0 ;
  while(i<NumByteToRead)
  {
	fulldata[0] = RegAdr+i;
	fulldata[1] = DUMMY;

	if (wiringPiSPIDataRW(spi_channel, fulldata, 2) == -1)
	{
      return WE_FAIL;
	}

	/* read out the received response of the sensor */
	Data[i] = fulldata[1];
	i++ ;
  }
  return WE_SUCCESS;

#else /* I2C is used */

  int ret = 0;

  if (NumByteToRead > 1)
  {

    /*Write the start address to initiate the read*/
    ret = wiringPiI2CWrite(i2c_handle, RegAdr);
    if (ret == -1)
    {
      return WE_FAIL;
    }
    else
    {
      /* Read the necessary number of bytes to an array */
      read(i2c_handle, Data, NumByteToRead);
    }
  }
  else
  {
    ret = wiringPiI2CReadReg8(i2c_handle, RegAdr);
    if (ret == -1)
    {
      return WE_FAIL;
    }
    else
    {
      Data[0] = (uint8_t)ret;
    }
  }

  return WE_SUCCESS;

#endif
}

/**
* @brief  Write data starting from the addressed register
* @param  -RegAdr : Address to write in
*         -NumByteToWrite : number of bytes to write
*         -pointer Data : Address of the data to be written
* @retval Error Code
*/
int8_t WriteReg(uint8_t RegAdr, int NumByteToWrite, uint8_t *Data)
{
#ifdef USE_SPI
  uint8_t fulldata[2];
  int i=0 ;
  while(i<NumByteToWrite)
  {
	fulldata[0] = RegAdr+i;
	fulldata[1] = Data [i];

	if (wiringPiSPIDataRW(spi_channel, fulldata, 2) == -1)
	{
      return WE_FAIL;
	}
	i++ ;
  }
  return WE_SUCCESS;

#else /* I2C is used */

  if (0x00 != wiringPiI2CWriteReg8(i2c_handle, RegAdr, Data[0]))
  {
    return WE_FAIL;
  }
  else
  {
    int i;
    for (i = 1; i < NumByteToWrite; i++)
    {
      if (0x00 != wiringPiI2CWrite(i2c_handle, Data[i]))
      {
        return WE_FAIL;
      }
    }
  }

  return WE_SUCCESS;
#endif
}

/**
*@brief Print a status string with formatting
*@param   - str: String to print
*         - status: ok or nok
*@retval None
*/
void Debug_out(char *str, bool status)
{
	if (status)
	{
		/* green OK */
		fprintf(stdout, COLOR_BOLDGREEN "OK   " COLOR_RESET);
	}
	else
	{
		/* red NOK */
		fprintf(stdout, COLOR_BOLDRED "NOK  " COLOR_RESET);
	}

	fprintf(stdout, str);
	fprintf(stdout, "\n");
}


/**
 * @brief Sleep for specified time
 * @param time period in ms
 * @retval None
 */
void delay(unsigned int sleepForMs)
{
	struct timespec sleeper, dummy;

	sleeper.tv_sec = (time_t)(sleepForMs / 1000);
	sleeper.tv_nsec = (long)(sleepForMs % 1000) * 1000000;

	nanosleep(&sleeper, &dummy);
}

/**         EOF         */
