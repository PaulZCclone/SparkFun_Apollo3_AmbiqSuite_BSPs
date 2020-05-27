//*****************************************************************************
//
//! @file main.c
//!
//! @brief SD card example - by Gyoorey (https://github.com/Gyoorey)
//!
//
//*****************************************************************************

/*
Copyright (c) 2019 SparkFun Electronics

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#include "am_mcu_apollo.h" // includes am_hal_iom.h for am_hal_iom_nonblocking_transfer
#include "am_bsp.h"
#include "am_util.h"
#include "SD_driver/ffconf.h"
#include "SD_driver/ff.h"
#include "SD_driver/diskio.h"
#include "SD_driver/ff.h"
#include "SD_driver/SD.h"
#include "SD_driver/SD_SPI.h"

// variables required by FatFs
FATFS fs;
FRESULT fr;
FIL fil;
UINT bw;
BYTE work[FF_MAX_SS];


//*****************************************************************************
//
// Main
//
//*****************************************************************************
int
main(void)
{
	// Set the clock frequency.
	am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);

	// Set the default cache configuration
	am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
	am_hal_cachectrl_enable();

	// Configure the board for low power operation.
	am_bsp_low_power_init();

	//
	// Enable the XT for the RTC.
	//
	am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_XTAL_START, 0);

	//
	// Select XT for RTC clock source
	//
	am_hal_rtc_osc_select(AM_HAL_RTC_OSC_XT);

	//
	// Enable the RTC.
	//
	am_hal_rtc_osc_enable();

	//enable uart printf functionality
	am_bsp_uart_printf_enable();

	//Initialize SPI IO Module 0
	// PINS in this case on an ATP board:
	// SCK: 5
	// MOSI: 7
	// MISO: 6
	// CS: 23 (spi_initialize_instance defaults to 11)
	// see am_bsp.c for specific boards and IO Module numbers
	am_util_stdio_printf("SPI init...\n");
	if(spi_initialize_instance(0) != 0) {
		am_util_stdio_printf("SPI init error!\n");
		return 1;
	}
	spi_cs_pin_set(23); //Change CS pin from 11 to 23
	am_util_stdio_printf("SPI Ready!\n");
	am_util_delay_ms(1000);

	am_util_stdio_printf("SD Card SPI mode init...\n");
	fr = SD_init();
	if(fr) {
		am_util_stdio_printf("SD card init failed!\n");
		return 1;
	}
	am_util_stdio_printf("SD Card Ready!\n");

	//Make filesystem (deletes everything from SD card)
//	fr = f_mkfs("", 0, work, sizeof work);
//	if(fr)
//	{
//		am_util_stdio_printf("\nError making filesystem\r\n");
//		for(;;){}
//	}
//	am_util_stdio_printf("Filesystem created successfully\n");

	//Set example file name
	char file_name1[12]="Test.csv";

	// Mounts a filesystem
	am_util_stdio_printf("Mounting the filesystem...\n");
	fr= f_mount(&fs,"",1);
	if(fr)
	{
		am_util_stdio_printf("\nError mounting file system\r\n");
		am_util_stdio_printf("mounting: FRESULT %d\n", fr);
		for(;;){}
	}
	am_util_stdio_printf("Filesystem ready.\n", fr);

	//Open the example file for write
	am_util_stdio_printf("Opening Test.csv...\n");
	fr = f_open(&fil, file_name1, FA_WRITE | FA_OPEN_ALWAYS);
	if(fr)
	{
		am_util_stdio_printf("\nError opening text file\r\n");
		am_util_stdio_printf("opening: FRESULT %d\n", fr);
		for(;;){}
	}
	am_util_stdio_printf("Test.csv opened.\n");

	// write relatively big chunks
	#define WRITE_LENGTH (16*512)
	#define NUM_WRITES 128

	// define the output buffer
	uint8_t write_buf[WRITE_LENGTH] = {0};
	// fill with data...
	for(uint32_t i=0; i<WRITE_LENGTH; i+=2) {
		write_buf[i] = '8';
		write_buf[i+1] = ';';
	}
	//write data to the csv file
	am_util_stdio_printf("Writing data...\n");
	am_hal_rtc_time_t hal_time__start;
	am_hal_rtc_time_t hal_time__end;
	am_hal_rtc_time_get(&hal_time__start);
	for (int i = 0; i < NUM_WRITES; i++)
	{
		fr = f_write(&fil, write_buf, WRITE_LENGTH, &bw);
		if (fr > 0) break;
	}
	am_hal_rtc_time_get(&hal_time__end);
	if(fr)
	{
		am_util_stdio_printf("\nError write text file\r\n");
		am_util_stdio_printf("write: FRESULT %d\n", fr);
		for(;;){}
	}
	am_util_stdio_printf("Write finished successfully.\n");
	uint32_t start_centis = (hal_time__start.ui32Second * 100) + hal_time__start.ui32Hundredths;
	uint32_t end_centis = (hal_time__end.ui32Second * 100) + hal_time__end.ui32Hundredths;
	uint32_t duration;
	if (end_centis < start_centis) //Did we span a minute?
	{
		duration = end_centis + 6000 - start_centis;
	}
	else
	{
		duration = end_centis - start_centis;
	}
	duration = duration * 10; //Convert to millis
	am_util_stdio_printf("Wrote %d * %d bytes in %i millis (to the nearest 10!)\n", NUM_WRITES, WRITE_LENGTH, duration);

	//close the file
	fr = f_close(&fil);
	am_util_stdio_printf("Closing file...\n");
	if(fr)
	{
		am_util_stdio_printf("\nError close text file\r\n");
		am_util_stdio_printf("close: FRESULT %d\n", fr);
		for(;;){}
	}
	am_util_stdio_printf("Test.csv closed\n");

	//re-open it for reading
	am_util_stdio_printf("Re-open Test.csv file...\n");
	fr = f_open(&fil, file_name1, FA_READ);
	if(fr)
	{
		am_util_stdio_printf("\nError opening text file: read\r\n");
		am_util_stdio_printf("opening: FRESULT %d\n", fr);
		for(;;){}
	}
	am_util_stdio_printf("Test.csv opened.\n");

	// define a new input buffer
	uint8_t read_buff[WRITE_LENGTH] = {0};
	// read file content
	am_util_stdio_printf("Reading data...\n");
	am_hal_rtc_time_get(&hal_time__start);
	for (int i = 0; i < NUM_WRITES; i++)
	{
		fr = f_read(&fil, read_buff, WRITE_LENGTH, &bw);
		if (fr > 0) break;
	}
	am_hal_rtc_time_get(&hal_time__end);
	if(fr)
	{
		am_util_stdio_printf("\nError reading text file\r\n");
		am_util_stdio_printf("f_read: FRESULT %d", fr);
		am_util_stdio_printf("\t\tattempted to read %d bytes", WRITE_LENGTH);
		am_util_stdio_printf("\t\t%d bytes read\n", bw);
		for(;;){}
	}
	am_util_stdio_printf("Data read successfully\n");
	start_centis = (hal_time__start.ui32Second * 100) + hal_time__start.ui32Hundredths;
	end_centis = (hal_time__end.ui32Second * 100) + hal_time__end.ui32Hundredths;
	if (end_centis < start_centis) //Did we span a minute?
	{
		duration = end_centis + 6000 - start_centis;
	}
	else
	{
		duration = end_centis - start_centis;
	}
	duration = duration * 10; //Convert to millis
	am_util_stdio_printf("Read %d * %d bytes in %i millis (to the nearest 10!)\n", NUM_WRITES, WRITE_LENGTH, duration);

	// am_util_stdio_printf("Data:\n");
	// for(int i=0; i<WRITE_LENGTH; i++) {
	// 	am_util_stdio_printf("%c",read_buff[i]);
	// }
	// am_util_stdio_printf("\n");

	am_util_stdio_printf("Closing file...\n");
	fr = f_close(&fil);
	if(fr)
	{
		am_util_stdio_printf("\nError close text file\r\n");
		am_util_stdio_printf("close: FRESULT %d\n", fr);
		for(;;){}
	}
	am_util_stdio_printf("Test.csv closed\n");
}
