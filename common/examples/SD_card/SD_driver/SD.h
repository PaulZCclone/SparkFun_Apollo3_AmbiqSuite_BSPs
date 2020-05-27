/*-----------------------------------------------------------------------/
/  SD interface for the Apollo3 Blue          							 /
/-----------------------------------------------------------------------*/

#ifndef _SD_DEFINED
#define _SD_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "am_bsp.h"
#include "am_util.h"
#include "SD_SPI.h"

uint32_t SD_init(void);
uint32_t SD_ReadSingleBlock(uint32_t addr, uint8_t *buf);
uint32_t SD_ReadMultipleBlock(uint32_t addr, uint8_t *buf, uint32_t count);
uint32_t SD_WriteSingleBlock(uint32_t addr,const uint8_t *buf);
uint32_t SD_WriteMultipleBlock(uint32_t addr, const uint8_t *buf, uint32_t count);
uint32_t SD_WriteCmd( uint8_t cmd, uint32_t arg, uint8_t crc );
uint32_t SD_CSD_CID(uint8_t cmd, uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif
