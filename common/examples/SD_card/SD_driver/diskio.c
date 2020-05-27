/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "SD.h"

/* Definitions of physical drive number for each drive */
#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status ( BYTE pdrv ) {
	if(pdrv){
		return STA_NOINIT;
    }
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize ( BYTE pdrv ) {
	DSTATUS stat;
	SD_CS(0);
	stat = SD_init();  //SD card initialization
	SD_CS(1);
	if(STA_NODISK == stat) {
		return STA_NODISK;
	} else if(0 != stat) {
		return STA_NOINIT; 
	} else {
		return 0;          
	}
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
    DRESULT res;
    if (pdrv || !count) {   
		return RES_PARERR; 
    }

	SD_CS(0);
	if (1 == count) {
		res = SD_ReadSingleBlock(sector, buff);
	} else {
		res = SD_ReadMultipleBlock(sector, buff, count);
	}
	SD_CS(1);

    if(0x00 == res) {
        return RES_OK;
    } else {
    	am_util_stdio_printf("Read error.\n");
        return RES_ERROR;
    }
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;
	if(pdrv || !count) {   
		return RES_PARERR; 
	}

	SD_CS(0);
	if(1 == count) {
		res = SD_WriteSingleBlock(sector, buff);
	}else{
		res = SD_WriteMultipleBlock(sector, buff, count);
	}
	SD_CS(1);
	
	if(0 == res) {
		return RES_OK;
	}else{
    	am_util_stdio_printf("Write error: %d\n", res);
    	am_util_stdio_printf("Args: %d ; %d\n", sector, count);
		return RES_ERROR;
	}
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	BYTE n, csd[16];
	DWORD csize;
	if (pdrv) {   
		return RES_PARERR; 
	}

	SD_CS(0);
	SD_CSD_CID(0x49, csd);
	res = RES_ERROR;
	switch (cmd) {
		case CTRL_SYNC:
			res = RES_OK; 
			break;

		case GET_SECTOR_COUNT: /* Get number of sectors on the disk (WORD) */
			if((SD_WriteCmd(0x49,0x00,0x95) == 0) && (SD_CSD_CID(0x49, csd)==0)) {
				if((csd[0] >> 6) == 1) { //SD ver 2.0
					csize = csd[9] + ((WORD)csd[8] << 8) + 1;
					*(DWORD*)buff = (DWORD)csize << 10;
				}else { //SD ver. 1.x
					n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
					csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
					*(DWORD*)buff = (DWORD)csize << (n - 9);
				}
				res = RES_OK;
			}
			break;

		case GET_SECTOR_SIZE : /* Get sectors on the disk (WORD) */
			*(WORD*)buff = 512;
			res = RES_OK;
			break;

		case GET_BLOCK_SIZE  :
			if ((SD_WriteCmd(0x49,0x00,0x95) == 0) && (SD_CSD_CID(0x49, csd)==0)) {
				*(DWORD*)buff = (((csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
				res = RES_OK;
			}
			break;

		default: 
	    	am_util_stdio_printf("IO error.\n");
			res = RES_PARERR; 
			break;
	}
	SD_CS(1);
	return res;
}

DWORD get_fattime (void)
{
	return 0;
}
