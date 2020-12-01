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
 * COPYRIGHT (c) 2020 Würth Elektronik eiSos GmbH & Co. KG
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

#include "version.h"
#include "WSEN_TIDS_2521020222501.h"

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
	//On the WSEN_TIDS evaluation board, the SAO pin is set to high by default
	//Using the I2C address 1 = 0x38
	const int addr_wsen_tids = TIDS_ADDRESS_I2C_1;
	int8_t status = 0;
	uint8_t deviceIdValue = 0, retVal = WE_FAIL;

	/* initialize the platform I2C drivers */
	status = I2CInit(addr_wsen_tids);

	if (status != WE_SUCCESS)
	{
		Debug_out("Platform I2C Init error ", false);
		return;
	}

	/* first communication test */
	retVal = TIDS_getDeviceID(&deviceIdValue);

	if ((retVal == WE_SUCCESS)&&(deviceIdValue == TIDS_DEVICE_ID_VALUE))
	{
		Debug_out("WSEN_TIDS Communication success\r\n", true);
		printf("DeviceID 0x%X\r\n", deviceIdValue);
	}
	else
	{
		Debug_out("WSEN_TIDS Communication fail\r\n", false);
		return;
	}

	/*Perform software reset */
    TIDS_setSwReset(enable);
    TIDS_setSwReset(disable);

    /*Enable auto address increment */
    TIDS_setAutoIncrement(enable);

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
	TIDS_state_t status = disable;
	float tempDegC = 0;

	printf("\nStarting single conversion mode... \r\n");
	printf("Press ENTER to trigger a new conversion\n");

	while (1)
	{
		getchar();
		/*Start a conversion*/
		TIDS_setSingleConvMode(enable);

		/*Wait till the busy bit is set to 0*/
		do
		{
            TIDS_getBusyStatus(&status);
		}while(status != enable);


		TIDS_getTemperature(&tempDegC);
		printf("Temperature [C]= %f\r\n", tempDegC);

	}
}

/**
* @brief  Setup the sensor in continuous mode. Read and print the measured values at a rate of 1 Hz
* @param  no parameter.
* @retval none
*/
void continuousMode()
{

	float tempDegC = 0;

	/*Set ODR to 25Hz*/
	TIDS_setOutputDataRate(outputDataRate25HZ);

	/*Enable block data update*/
	TIDS_setBlockDataUpdate(enable);

	/*Enable continuous mode*/
    TIDS_setContinuousMode(enable);

	printf("\nStarting continuous mode...\r\n");
	while (1)
	{
		TIDS_getTemperature(&tempDegC);
		printf("Temperature [C]=%f\r\n", tempDegC);
		delay(DELAY_1_HZ_IN_MS);
	}

}

/**         EOF         */

