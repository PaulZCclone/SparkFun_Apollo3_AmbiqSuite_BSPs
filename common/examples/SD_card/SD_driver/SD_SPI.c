/*
 * SD_SPI.c
 *
 *  Created on: Apr 28, 2020
 *      Author: Gyoorey
 */

#include "SD_SPI.h"
#include "am_bsp.h"
#include "am_util.h"

// IO Module used
uint8_t spi_instance;
// pointer to the IO Module structure
void* spi_handle = NULL;
// SPI interface configuration
am_hal_iom_config_t spi_config;
// SPI transfer configuration
am_hal_iom_transfer_t iomTransfer = {0};
// CS pin
uint32_t cs_pin;

//dummys for full duplex
uint8_t dummy_tx[512];
uint8_t dummy_rx[512];

uint32_t _initialize_config(am_hal_iom_config_t config) {
	uint32_t retVal32 = 0;

	if(spi_handle != NULL) {
		am_hal_iom_disable(spi_handle);
		am_hal_iom_uninitialize(spi_handle);
	}

	retVal32 = am_hal_iom_initialize(spi_instance, &spi_handle);
	if (retVal32 != AM_HAL_STATUS_SUCCESS){
		return 1;
	}

	retVal32 = am_hal_iom_power_ctrl(spi_handle, AM_HAL_SYSCTRL_WAKE, false);    ////it was false
	if (retVal32 != AM_HAL_STATUS_SUCCESS){
		return 1;
	}

	retVal32 = am_hal_iom_configure(spi_handle, &spi_config);
	if (retVal32 != AM_HAL_STATUS_SUCCESS){
		return 1;
	}

	retVal32 = am_hal_iom_enable(spi_handle);
	if (retVal32 != AM_HAL_STATUS_SUCCESS){
		return 1;
	}
	return 0;
}

uint32_t _initialize_pins(uint8_t iom_instance) {
	if(iom_instance < 6) {
		am_bsp_iom_pins_enable(iom_instance, AM_HAL_IOM_SPI_MODE);
	// 	switch(iom_instance){
	// 		case 0: {cs_pin=AM_BSP_GPIO_IOM0_CS; break;}
	// 		case 1: {cs_pin=AM_BSP_GPIO_IOM1_CS; break;}
	// 		case 2: {cs_pin=AM_BSP_GPIO_IOM2_CS; break;}
	// 		case 3: {cs_pin=AM_BSP_GPIO_IOM3_CS; break;}
	// 		case 4: {cs_pin=AM_BSP_GPIO_IOM4_CS; break;}
	// 		case 5: {cs_pin=AM_BSP_GPIO_IOM5_CS; break;}
	// 	}
	// } else {
	// 	//default settings: IO Module 1
	// 	am_hal_gpio_pinconfig(AM_BSP_GPIO_IOM1_SCK,  g_AM_BSP_GPIO_IOM1_SCK);
	// 	am_hal_gpio_pinconfig(AM_BSP_GPIO_IOM1_MISO, g_AM_BSP_GPIO_IOM1_MISO);
	// 	am_hal_gpio_pinconfig(AM_BSP_GPIO_IOM1_MOSI, g_AM_BSP_GPIO_IOM1_MOSI);
	// 	//use A11 as the CS
	// 	am_hal_gpio_pincfg_t g_AM_BSP_GPIO_IOM1_CS_11 =
	// 	{
	// 	    .uFuncSel            = AM_HAL_PIN_11_NCE11,
	// 	    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
	// 	    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,
	// 	    .eGPInput            = AM_HAL_GPIO_PIN_INPUT_NONE,
	// 	    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
	// 	    .uIOMnum             = 1,
	// 	    .uNCE                = 0,
	// 	    .eCEpol              = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW
	// 	};
	// 	am_hal_gpio_pinconfig(11,   g_AM_BSP_GPIO_IOM1_CS_11);
	// 	cs_pin = 11;

		// https://github.com/sparkfun/Arduino_Apollo3/issues/9#issuecomment-625840459
	 	am_hal_gpio_pinconfig(23, g_AM_HAL_GPIO_OUTPUT);
	 	cs_pin = 23;
		return 0;
	}

	//init dummies
	uint32_t i;
	for(i=0;i<512;i++) {
		dummy_rx[i] = 0;
		dummy_tx[i] = 0xFF;
	}

	return 0;
}

uint32_t spi_initialize() {
	spi_instance = 1;
	spi_config.eInterfaceMode = AM_HAL_IOM_SPI_MODE;
	spi_config.ui32ClockFreq = AM_HAL_IOM_400KHZ; //set to low-speed mode
	spi_config.eSpiMode = AM_HAL_IOM_SPI_MODE_0; //used by SD cards
	if(_initialize_config(spi_config) != 0) {
		return 1;
	}
	//default settings
	if(_initialize_pins(255)) {
		return 1;
	}
	//set default transfer parameters that never change
	iomTransfer.ui32InstrLen = 0;
	iomTransfer.ui32Instr = 0;
	iomTransfer.bContinue = true;
	iomTransfer.ui8RepeatCount = 0;
	iomTransfer.ui8Priority = 1;
	iomTransfer.ui32PauseCondition = 0;
	iomTransfer.ui32StatusSetClr = 0;
	return 0;
}

uint32_t spi_initialize_instance(uint8_t iom_instance) {
	spi_instance = iom_instance;
	spi_config.eInterfaceMode = AM_HAL_IOM_SPI_MODE;
	spi_config.ui32ClockFreq = AM_HAL_IOM_400KHZ; //set to low-speed mode
	spi_config.eSpiMode = AM_HAL_IOM_SPI_MODE_0; //used by SD cards
	if(_initialize_config(spi_config) != 0) {
		return 1;
	}
	if(_initialize_pins(iom_instance)) {
		return 1;
	}
	//set default transfer parameters that never change
	iomTransfer.ui32InstrLen = 0;
	iomTransfer.ui32Instr = 0;
	iomTransfer.bContinue = true;
	iomTransfer.ui8RepeatCount = 0;
	iomTransfer.ui8Priority = 1;
	iomTransfer.ui32PauseCondition = 0;
	iomTransfer.ui32StatusSetClr = 0;
	return 0;
}

uint32_t spi_deinitialize() {
	if(spi_handle != NULL) {
		am_hal_iom_disable(spi_handle);
		am_hal_iom_uninitialize(spi_handle);
		return 0;
	}
	return 1;
}

void _transfer_tx(void *buf_out, void *buf_in, size_t count)
{
//	iomTransfer.ui32NumBytes = count;
//	iomTransfer.pui32TxBuffer = (uint32_t *)buf_out;
//	iomTransfer.pui32RxBuffer = (uint32_t *)buf_in;
//	iomTransfer.eDirection = AM_HAL_IOM_TX;
//	uint32_t status = am_hal_iom_blocking_transfer(spi_handle, &iomTransfer);
	iomTransfer.ui32NumBytes = count;
	iomTransfer.pui32TxBuffer = (uint32_t *)buf_out;
	iomTransfer.pui32RxBuffer = (uint32_t *)dummy_rx;
	iomTransfer.eDirection = AM_HAL_IOM_FULLDUPLEX;
	uint32_t status = am_hal_iom_spi_blocking_fullduplex(spi_handle, &iomTransfer);
	if(status != 0){
		am_util_stdio_printf("txs: %d\n", status);
	}
}

void _transfer_rx(void *buf_out, void *buf_in, size_t count)
{
//	iomTransfer.ui32NumBytes = count;
//	iomTransfer.pui32TxBuffer = (uint32_t *)buf_out;
//	iomTransfer.pui32RxBuffer = (uint32_t *)buf_in;
//	iomTransfer.eDirection = AM_HAL_IOM_RX;
//	uint32_t status = am_hal_iom_blocking_transfer(spi_handle, &iomTransfer);
	iomTransfer.ui32NumBytes = count;
	iomTransfer.pui32TxBuffer = (uint32_t *)dummy_tx;
	iomTransfer.pui32RxBuffer = (uint32_t *)buf_in;
	iomTransfer.eDirection = AM_HAL_IOM_FULLDUPLEX;
	uint32_t status = am_hal_iom_spi_blocking_fullduplex(spi_handle, &iomTransfer);
	if(status != 0){
		am_util_stdio_printf("rxs: %d\n", status);
	}
}

void _transfer_fd(void *buf_out, void *buf_in, uint32_t count) {
	iomTransfer.ui32NumBytes = count;
	iomTransfer.pui32TxBuffer = (uint32_t *)buf_out;
	iomTransfer.pui32RxBuffer = (uint32_t *)buf_in;
	iomTransfer.eDirection = AM_HAL_IOM_FULLDUPLEX;
	uint32_t status = am_hal_iom_spi_blocking_fullduplex(spi_handle, &iomTransfer);
	if(status != 0){
		am_util_stdio_printf("fds: %d\n", status);
	}
}

uint32_t spi_setspeed(uint32_t speed)
{
	spi_config.ui32ClockFreq = speed;
	return _initialize_config(spi_config);
}

void spi_cs_pin_set(uint8_t new_pin) {
	am_hal_gpio_pinconfig(new_pin,   g_AM_HAL_GPIO_OUTPUT_12);
	cs_pin = new_pin;
}

void spi_cs_set() {
	am_hal_gpio_state_write(cs_pin, AM_HAL_GPIO_OUTPUT_SET);
}

void spi_cs_clear() {
	am_hal_gpio_state_write(cs_pin, AM_HAL_GPIO_OUTPUT_CLEAR);
}
