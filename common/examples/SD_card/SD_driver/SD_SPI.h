/*
 * SD_SPI.h
 *
 * 	Implements basic SPI functions related to SD card usage.
 *
 * 	The driver is not complete as it does not support all IO Modules' pin configurations.
 *
 *  Created on: Apr 28, 2020
 *      Author: Gyorgy Kalmar
 */

#ifndef SD_DRIVER_SD_SPI_H_
#define SD_DRIVER_SD_SPI_H_

#include "am_bsp.h"
#include "am_util.h"

//*****************************************************************************
//
//! @brief Initialize the default SPI interface for SD card usage.
//!
//! default settings:
//! IO Module 1
//! CS: 23 (was A11)
//! SCK: 8
//! MOSI: 10
//! MISO: 9
//!
//! @return 0 if success.
//
//*****************************************************************************
uint32_t spi_initialize();

//*****************************************************************************
//
//! @brief Initialize the given SPI interface for SD card usage.
//!
//!	Pin config will be the default setup for the given IO Module.
//! These are defined in the board's .bsp file by calling am_bsp_iom_pins_enable(iom_instance)
//!
//! @param iom_instance: IO Module number
//!
//! @return 0 if success.
//
//*****************************************************************************
uint32_t spi_initialize_instance(uint8_t iom_instance);

//*****************************************************************************
//
//! @brief De-Initialize the given SPI interface.
//!
//! @return 0 if success.
//
//*****************************************************************************
uint32_t spi_deinitialize();

//*****************************************************************************
//
//! @brief Blocking send of byte(s) through SPI
//!
//!	@param buf_out: TX buffer
//!	@param buf_in: RX buffer, should be NULL as fullduplex not implemented
//!	@param count: number of bytes transferred
//!
//
//*****************************************************************************
void _transfer_tx(void *buf_out, void *buf_in, size_t count);


//*****************************************************************************
//
//! @brief Blocking receive of byte(s) through SPI
//!
//!	@param buf_out: TX buffer, should be NULL as fullduplex not implemented
//!	@param buf_in: RX buffer
//!	@param count: number of bytes transferred
//!
//
//*****************************************************************************
void _transfer_rx(void *buf_out, void *buf_in, size_t count);


//*****************************************************************************
//
//! @brief Blocking full duplex transfer: Only one byte supported
//!
//!	@param buf_out: TX buffer
//!	@param buf_in: RX buffer
//!
//
//*****************************************************************************
void _transfer_fd(void *buf_out, void *buf_in, uint32_t count);


//*****************************************************************************
//
//! @brief Set SPI clock frequency.
//!
//!	@param speed: Desired frequency to be set.
//!
//! @return 0 if success.
//
//*****************************************************************************
uint32_t spi_setspeed(uint32_t speed);


//*****************************************************************************
//
//! @brief Lets user to set manually controlled CS pin
//!	Use only in that cases when your setup requires an arbitrary pin to be used
//! as a CS pin. The original CS pin will retain its functionality but a new,
//! extra pin is controlled manually as well.
//
//*****************************************************************************
void spi_cs_pin_set(uint8_t new_pin);


//*****************************************************************************
//
//! @brief Sets CS signal to High.
//!
//
//*****************************************************************************
void spi_cs_set();

//*****************************************************************************
//
//! @brief Sets CS signal to Low.
//!
//
//*****************************************************************************
void spi_cs_clear();

#endif /* SD_DRIVER_SD_SPI_H_ */
