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
* COPYRIGHT(c) 2019 Würth Elektronik eiSos GmbH & Co. KG
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

#include "WSEN_ITDS_2533020201601.h"
#include "version.h"

#define DELAY_1_S_IN_MS     1000


bool AbortMainLoop = false;
pthread_t thread_main;


static void Application(void);
void startHighPerformanceMode(void);
void startNormalMode(void);
void startLowPowerMode(void);


int initPlatform(void);


/**
* @brief  Setup wiring pi library which provides APIs to access I2C, SPI, GPIO etc on the Raspberry Pi platform
* @param  no parameter.
* @retval 0 on success; -1 on error
*/
int initPlatform()
{
	char initstr[64] = { '\0' };

	printf("Wuerth Elektronik eiSos Sensors SDK version %d.%d\n", WE_SENSOR_SDK_MAJOR_VERSION, WE_SENSOR_SDK_MINOR_VERSION);

	/* Setup wiring Pi library */
	if (wiringPiSetup() == -1)
	{
		sprintf(initstr, "Unable to start wiringPi libaray: %s", strerror(errno));
		Debug_out(initstr, false);
		return -1;
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
	}
	return 0;
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
* @brief  reading and printing sensor
* @param  no parameter.
* @retval none
*/
static void Application()
{

#ifdef USE_SPI
	int8_t status = 0;
	int spi_channel = 1;

	uint8_t deviceIdValue = 0;

	/* initialize the platform SPI drivers */
	status = SpiInit(spi_channel);

	if (status != WE_SUCCESS)
	{
		Debug_out("Platform SPI Init error ", false);
		return;
	}
#else /* use i2c */
	const int addr_wsen_itds = ITDS_ADDRESS_I2C_1; /* or ITDS_ADDRESS_I2C_0 */
	int8_t status = 0;
	uint8_t deviceIdValue = 0;

	/* initialize the platform I2C drivers */
	status = I2CInit(addr_wsen_itds);

	if (status != WE_SUCCESS)
	{
		Debug_out("Platform I2C Init error ", false);
		return;
	}
#endif

	uint8_t retval = 0;
	retval = ITDS_getDeviceID(&deviceIdValue);

	if ((retval == WE_SUCCESS) && (deviceIdValue == ITDS_DEVICE_ID_VALUE))
	{
		Debug_out("WSEN_ITDS Communication success\r\n", true);
		printf("DeviceID 0x%X\r\n", deviceIdValue);
	}
	else
	{
		Debug_out("WSEN_ITDS Communication fail\r\n", false);
		return;
	}

	/*select one of the three mode by uncommenting/commenting */
	startHighPerformanceMode();
	//startNormalMode();
	//startLowPowerMode();

}
/**
* @brief  High performance mode
* @param  no parameter.
* @retval none
*/
void startHighPerformanceMode()
{
	ITDS_state_t DRDY = ITDS_disable;
	int16_t XRawAcc = 0, YRawAcc = 0, ZRawAcc = 0;

	/* Sampling rate of 200 Hz */
	ITDS_setOutputDataRate(odr6);
	/* Enable high performance mode */
	ITDS_setOperatingMode(highPerformance);
	/* Enable block data update */
	ITDS_setBlockDataUpdate(ITDS_enable);
	/* Enable address auto increment */
	ITDS_setAutoIncrement(ITDS_enable);
	/* Full scale +-16g */
	ITDS_setFullScale(sixteenG);
	/* Filter bandwidth = ODR/2 */
	ITDS_setFilteringCutoff(outputDataRate_2);

	while(1)
	{
		do
		{
			/*Wait till the value is ready to read*/
			ITDS_getdataReadyState(&DRDY);
		} while (DRDY == ITDS_disable);

		ITDS_getRawAccelerationX(&XRawAcc);
		XRawAcc = XRawAcc >> 2; /* shifted by 2 as 14bit resolution is used in high performance mode */
		float XAcceleration = (float) (XRawAcc);
		XAcceleration = XAcceleration / 1000; /* mg to g */
		XAcceleration = XAcceleration * 1.952; /* Multiply with sensitivity 1.952 in high performance mode, 14bit, and full scale +-16g */
		printf("Acceleration X-axis %f g \r\n ", XAcceleration);

		ITDS_getRawAccelerationY(&YRawAcc);
		YRawAcc = YRawAcc >> 2;
		float YAcceleration = (float) (YRawAcc);
		YAcceleration = YAcceleration / 1000;
		YAcceleration = (YAcceleration * 1.952);
		printf("Acceleration Y-axis %f g \r\n ", YAcceleration);

		ITDS_getRawAccelerationZ(&ZRawAcc);
		ZRawAcc = ZRawAcc >> 2;
		float ZAcceleration = (float) (ZRawAcc);
		ZAcceleration = ZAcceleration / 1000;
		ZAcceleration = ZAcceleration * 1.952;
		printf("Acceleration Z-axis %f g \r\n ", ZAcceleration);

		delay(DELAY_1_S_IN_MS);
	}
}
/**
* @brief  Normal mode
* @param  no parameter.
* @retval none
*/
void startNormalMode()
{
	ITDS_state_t DRDY = ITDS_disable;
	int16_t XRawAcc = 0, YRawAcc = 0, ZRawAcc = 0;

	/* Sampling rate of 200 Hz */
	ITDS_setOutputDataRate(odr6);
	/* Enable normal mode*/
	ITDS_setOperatingMode(normalOrLowPower);
	ITDS_setpowerMode(normalMode);
	/* Enable block data update */
	ITDS_setBlockDataUpdate(ITDS_enable);
	/* Enable address auto increment */
	ITDS_setAutoIncrement(ITDS_enable);
	/* Full scale +-16g */
	ITDS_setFullScale(sixteenG);
	/* Filter bandwidth = ODR/2 */
	ITDS_setFilteringCutoff(outputDataRate_2);
	while(1)
	{
		do
		{
			ITDS_getdataReadyState(&DRDY);
		} while (DRDY == ITDS_disable);

		ITDS_getRawAccelerationX(&XRawAcc);
		XRawAcc = XRawAcc >> 2; /* shifted by 2 as 14bit resolution is used in normal mode */
		float XAcceleration = (float) (XRawAcc);
		XAcceleration = XAcceleration / 1000; /* mg to g */
		XAcceleration = XAcceleration * 1.952; /* Multiply with sensitivity 1.952 in normal mode, 14bit, and full scale +-16g */
		printf("Acceleration X-axis %f g \r\n", XAcceleration);

		ITDS_getRawAccelerationY(&YRawAcc);
		YRawAcc = YRawAcc >> 2; 
		float YAcceleration = (float) (YRawAcc);
		YAcceleration = YAcceleration / 1000;
		YAcceleration = (YAcceleration*1.952);
		printf("Acceleration Y-axis %f g \r\n", YAcceleration);

		ITDS_getRawAccelerationZ(&ZRawAcc);
		ZRawAcc = ZRawAcc >> 2;
		float ZAcceleration = (float) (ZRawAcc);
		ZAcceleration = ZAcceleration / 1000;
		ZAcceleration = ZAcceleration * 1.952;
		printf("Acceleration Z-axis %f g \r\n", ZAcceleration);
		delay(DELAY_1_S_IN_MS);
	}
}

/**
* @brief  Low power mode
* @param  no parameter.
* @retval none
*/
void startLowPowerMode()
{
	ITDS_state_t DRDY = ITDS_disable;
	int16_t XRawAcc = 0, YRawAcc = 0, ZRawAcc = 0;

	/* Sampling rate of 200 Hz */
	ITDS_setOutputDataRate(odr6);
	/* Enable low power mode */
	ITDS_setOperatingMode(normalOrLowPower);
	ITDS_setpowerMode(lowPower);
	/* Enable block data update */
	ITDS_setBlockDataUpdate(ITDS_enable);
	/* Enable address auto increment */
	ITDS_setAutoIncrement(ITDS_enable);
	/* Full scale +-16g */
	ITDS_setFullScale(sixteenG);
	/* Filter bandwidth = ODR/2 */
	ITDS_setFilteringCutoff(outputDataRate_2);
	while(1)
	{
		do
		{
			ITDS_getdataReadyState(&DRDY);
		} while (DRDY == ITDS_disable);

		ITDS_getRawAccelerationX(&XRawAcc);
		XRawAcc = XRawAcc >> 4; /* shifted by 4 as 12bit resolution is used in low power mode */
		float XAcceleration = (float) (XRawAcc);
		XAcceleration = XAcceleration / 1000; /* mg to g */
		XAcceleration = XAcceleration * 7.808; /* Multiply with sensitivity 7.808 in low power mode, 12 bit, and full scale +-16g */
		printf("Acceleration X-axis %f g \r\n", XAcceleration);

		ITDS_getRawAccelerationY(&YRawAcc);
		YRawAcc = YRawAcc >> 4;
		float YAcceleration = (float) (YRawAcc);
		YAcceleration = YAcceleration / 1000; 
		YAcceleration = (YAcceleration * 7.808);
		printf("Acceleration Y-axis %f g \r\n", YAcceleration);

		ITDS_getRawAccelerationZ(&ZRawAcc);
		ZRawAcc = ZRawAcc >> 4;
		float ZAcceleration = (float) (ZRawAcc);
		ZAcceleration = ZAcceleration / 1000;
		ZAcceleration = ZAcceleration * 7.808;
		printf("Acceleration Z-axis %f g \r\n", ZAcceleration);

		delay(DELAY_1_S_IN_MS);
	}
}



