/**
 ***************************************************************************************************
 * This file is part of WE sensors SDK:
 * https://www.we-online.com/sensors
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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <pthread.h>
#include <string.h>

#include "WSEN_PADS_2511020213301.h"
#include "version.h"

#define DELAY_1_HZ_IN_MS     1000

bool AbortMainLoop = false;
pthread_t thread_main;


static void Application(void);
void continuousMode(void);
void singleConversionMode(void);
int initPlatform(void);


/**
* @brief  Setup wiring pi library which provides APIs to access I2C, SPI, GPIO etc on the Raspberry Pi platform
* @param  no parameter.
* @retval 0 on success; -1 on error
*/
int initPlatform()
{
	int ret = -1;
	char initstr[64] = { '\0' };

	printf("Wuerth Elektronik eiSos Sensors SDK version %d.%d\n", WE_SENSOR_SDK_MAJOR_VERSION, WE_SENSOR_SDK_MINOR_VERSION);

	/*Setup wiring Pi library*/
	if (wiringPiSetup() == -1)
	{
		sprintf(initstr, "Unable to start wiringPi libaray: %s", strerror(errno));
		Debug_out(initstr, false);
		ret = -1;
	}
	else
	{
		/* wiring PI started successfully */
		/* tested with wiringPi version 2.46 */
		int wiring_version_major;
		int wiring_version_minor;
		wiringPiVersion(&wiring_version_major, &wiring_version_minor);
		sprintf(initstr, "Platform init: WiringPi v%d.%d", wiring_version_major, wiring_version_minor);
		Debug_out(initstr, true);
		ret = 0;
	}

	return ret;
}

/**
* @brief  Main thread to run the application
* @param  no parameter.
* @retval Does not return
*/
void *main_Thread()
{

	/* initialize platform */
	if (initPlatform() != 0)
	{
		return 0;
	}
	/* sample application - reading and printing sensor values */
	Application();

	AbortMainLoop = true;
	return 0;
}


/**
* @brief  Main funtion - starts the main thread
* @param  no parameter.
* @retval Does not return
*/
int main()
{
	AbortMainLoop = false;

	if (pthread_create(&thread_main, NULL, &main_Thread, NULL))
	{
		printf("Failed to start thread_main");
		return false;
	}
	while (1)
	{
		delay(1000);
		if (AbortMainLoop == true)
		{
			/* jump out of the main loop and exit program */
			return 0;
		}
	}

	return 0;
}

/**
* @brief  reading and printing sensor values in single Conversion or Continuous Mode
* @param  no parameter.
* @retval none
*/
static void Application()
{

#ifdef USE_SPI
	int8_t status = 0;
	int spi_channel = 1;

	uint8_t deviceIdValue = 0;
	int8_t retVal = -1;

	/* initialize the platform SPI drivers */
	status = SpiInit(spi_channel);

	if (status != WE_SUCCESS)
	{
	Debug_out("Platform SPI Init error ", false);
	return;
	}
#else
	//On the WSEN_PADS evaluation board, the SAO pin is set to high by default
	//Using the I2C address 1 = 0x5D
	const int addr_wsen_pads = PADS_ADDRESS_I2C_1;
	int8_t status = 0;
	uint8_t deviceIdValue = 0, retVal = WE_FAIL;

	/* initialize the platform I2C drivers */
	status = I2CInit(addr_wsen_pads);

	if (status != WE_SUCCESS)
	{
		Debug_out("Platform I2C Init error ", false);
		return;
	}
#endif
	/* first communication test */
	retVal = PADS_getDeviceID(&deviceIdValue);
	if ((retVal == WE_SUCCESS)&&(deviceIdValue == PADS_DEVICE_ID_VALUE))
	{
		Debug_out("WSEN_PADS Communication success\r\n", true);
		printf("DeviceID 0x%X\r\n", deviceIdValue);
	}
	else
	{
		Debug_out("WSEN_PADS Communication fail\r\n", false);
		return;
	}


	/*set ODR to Power down*/
	PADS_setOutputDataRate(outputDataRatePowerDown);

	/*set low pass filter to BW=ODR/20*/
	PADS_setLowPassFilterConf(lPfilterBW2);

	/*enable low pass filter */
	PADS_setLowPassFilter(enable);

	/*enable Block Data Update*/
	PADS_setBlockDataUpdate(enable);

	/*select Power Mode [0:low Current Mode; 1:low noise Mode]*/
	PADS_setPowerMode(lowNoise);

	/*enable the Auto Increment */
	PADS_setAutoIncrement(enable);

	/*Select one of the two mode by uncommenting/commenting*/
	//continuousMode();
	singleConversionMode();

}

/**
* @brief  Setup the sensor in single conversion mode and trigger a readout on Enter key press
* @param  no parameter.
* @retval none
*/

void singleConversionMode()
{
    float temperatureData = 0;
	float pressureData = 0;

	printf("\nStarting single conversion mode... \r\n");
	printf("Press ENTER to trigger a new conversion\n");

	while (1)
	{
		getchar();
		/*Start a conversion*/
		PADS_setSingleConvMode(enable);

		delay(15);

		PADS_getPressure(&pressureData);
		printf("Pressure [kPa]= %f\r\n", pressureData);

		PADS_getTemperature(&temperatureData);
		printf("Temperature [C]= %f\r\n", temperatureData);

	}
}

/**
* @brief  Setup the sensor in continuous mode. Read and print the measured values at a rate of 1 Hz
* @param  no parameter.
* @retval measured Pressure and temperature
*/
void continuousMode()
{

	float temperatureData = 0;
	float pressureData = 0;

	/*Set ODR to 1Hz to start continuous mode*/
	PADS_setOutputDataRate(outputDataRate1HZ);

	printf("\nStarting continuous mode...\r\n");
	while (1)
	{
		PADS_getPressure(&pressureData);
		printf("Pressure [kPa]=%f\r\n", pressureData);

		PADS_getTemperature(&temperatureData);
		printf("Temperature [C]=%f\r\n", temperatureData);

		delay(DELAY_1_HZ_IN_MS);
	}

}

/**         EOF         */
