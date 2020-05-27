/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "SD.h"		/* Declarations of disk functions */
#include "am_bsp.h"  /*board specific pin configuration*/

/* Definitions for MMC/SDC commands */
#define CMD0    (0x40+0)    /* GO_IDLE_STATE */
#define CMD1    (0x40+1)    /* SEND_OP_COND */
#define CMD8    (0x40+8)    /* SEND_IF_COND */
#define CMD9    (0x40+9)    /* SEND_CSD */
#define CMD10    (0x40+10)    /* SEND_CID */
#define CMD12    (0x40+12)    /* STOP_TRANSMISSION */
#define CMD16    (0x40+16)    /* SET_BLOCKLEN */
#define CMD17    (0x40+17)    /* READ_SINGLE_BLOCK */
#define CMD18    (0x40+18)    /* READ_MULTIPLE_BLOCK */
#define CMD23    (0x40+23)    /* SET_BLOCK_COUNT */
#define CMD24    (0x40+24)    /* WRITE_BLOCK */
#define CMD25    (0x40+25)    /* WRITE_MULTIPLE_BLOCK */
#define CMD41    (0x40+41)    /* SEND_OP_COND (ACMD) */
#define CMD55    (0x40+55)    /* APP_CMD */
#define CMD58    (0x40+58)    /* READ_OCR */

// pre-declaration of private functions
uint32_t SD_SSP0LowSpeed(void);
uint32_t SD_SSP0HighSpeed(void);
void SD_DelayUs(uint32_t tt);
void SD_Send_Byte(uint8_t data);
uint8_t SD_Get_Byte(void);
uint8_t SD_WaitReady( void );
void SD_CS(uint8_t cs);

// type to store SD card type
enum {
	SD_TYPE_V2 = 0,
	SD_TYPE_V2HC = 1,
};

uint32_t SD_Type = SD_TYPE_V2HC;

// dummies for xfers
static uint32_t rxval = 0;
static uint32_t txval = 0xFF;

// repeat counter for various waiting tasks
uint32_t repeat;
#define REPORT_REPEAT_ERROR_SD(err) if(--repeat == 0) {am_util_stdio_printf("Error: %d\n", __LINE__ ); return err;}
#define SET_REPEAT_SD(cnt) repeat=(uint32_t)cnt
// wait until card is ready
#define SD_WAIT_READY() if(SD_WaitReady()) return 1;


void SD_DelayUs(uint32_t tt)
{
	am_util_delay_us(tt);
}

void SD_Send_Byte (uint8_t data)
{
	_transfer_tx(&data, NULL, 1);
}


uint8_t SD_Get_Byte(void)
{
	txval = 0xFF;
	_transfer_fd(&txval, &rxval, 1);
	return rxval;
}


uint32_t SD_WriteCmd( uint8_t cmd, uint32_t arg, uint8_t crc )
{
	uint32_t cnt=512;
	uint8_t  sta;
	//am_util_stdio_printf("SD_WriteCmd: %d : ", cmd-0x40);
	SD_WAIT_READY();

	SD_Send_Byte( cmd | 0x40 );
	SD_Send_Byte( (uint8_t)(arg>>24) );
	SD_Send_Byte( (uint8_t)(arg>>16) );
	SD_Send_Byte( (uint8_t)(arg>>8) );
	SD_Send_Byte( (uint8_t)(arg) );
	SD_Send_Byte( crc );   // CRC

	do
	{
		sta = SD_Get_Byte();
		cnt--;
	} while ( (cnt)&&(sta & 0x80) );

	return (uint32_t) sta;
}

uint32_t SD_CSD_CID(uint8_t cmd, uint8_t *buf)
{
	SET_REPEAT_SD(1000);
	while(SD_WriteCmd(cmd,0x00,0xFF) != 0x00){
		REPORT_REPEAT_ERROR_SD(1)
	}
	SD_WAIT_READY();
	_transfer_rx(NULL, buf, 16);

	return 0;
}

uint8_t SD_WaitReady( void )
{
	uint32_t cnt = 0x00fffff;
	uint8_t sta;
	do
	{
		sta = SD_Get_Byte( );
		if ( sta == 0xFF ) //
		{
			return 0;
		}
		cnt--;
	} while ( cnt );
	return 1;
}

uint8_t SD_WaitBusy( void ) {
	uint32_t retVal;
	SET_REPEAT_SD(10000);
	while(SD_Get_Byte() == 0) { REPORT_REPEAT_ERROR_SD(1) }
	return 0;
}

void SD_CS(uint8_t cs)
{
	if (cs == 1)
	{
		spi_cs_set();
	}
	else
	{
		spi_cs_clear();
	}
}


uint32_t SD_SSP0LowSpeed(void)
{
	return spi_setspeed(AM_HAL_IOM_400KHZ);
}

uint32_t SD_SSP0HighSpeed(void)
{
	return spi_setspeed(AM_HAL_IOM_24MHZ);
}

uint32_t SD_init(void)
{
	uint8_t i = 0,tmp = 0;
	uint8_t  buff[512];

	SD_SSP0LowSpeed();                                      // low speed
	SD_DelayUs(5000);

	// send 72 clocks
	for (i=0; i<0x0F; i++)              
	{
		SD_Send_Byte(0xFF);
	}
	i=0;
	// Send Command CMD0 to SD/SD Card  enter idle
	do
	{
		tmp = SD_WriteCmd(CMD0,0x00,0x95);   // CMD0
		i++;
	}while ((tmp != 1) && (i < 200));

	if(i >= 200)
	{
		return 1;
	}                             

	//get SD card version
	tmp = SD_WriteCmd( CMD8,0x1AA,0x87 );
	//trailing 32bits
	SD_Get_Byte();
	SD_Get_Byte();
	SD_Get_Byte();
	SD_Get_Byte();

	if(tmp == 1)// 2.0 card
	{
		//init card: CMD41 is only for SDC
		i=1000;
		do
		{
			SD_WriteCmd( CMD55, 0, 0xFF );
			tmp = SD_WriteCmd( CMD41,0x40000000, 0xFF);//CMD41
			i--;
		} while ((tmp) && (i));
		if(i == 0)
		{
			return 1;
		}

		//Get OCR information
		tmp = SD_WriteCmd(CMD58, 0, 0 );
		if ( tmp != 0x00 )
		{
			return 1;
		}

		for ( i = 0; i < 4; i++ )
		{
			buff[ i ] = SD_Get_Byte();
		}

		if ( buff[0] & 0x40 )
		{
			SD_Type = SD_TYPE_V2HC;
			//am_util_stdio_printf( "card is V2.0 SDHC.....\n\n" );
		}
		else {
			SD_Type = SD_TYPE_V2;
			//am_util_stdio_printf( "card is V2.0.....\n\n" );
		}          
		SET_REPEAT_SD(1000);
		while(SD_WriteCmd(CMD16,512,0xFF) != 0){
			REPORT_REPEAT_ERROR_SD(1)
		}
	} else {
		//init older cards...
		// not supported
	}

	SD_SSP0HighSpeed();                    //back to high speed
	SD_WaitReady();
	return 0;                       
}


uint32_t SD_ReadSingleBlock(uint32_t addr, uint8_t *buf)
{
	SD_WaitReady();
	SET_REPEAT_SD(1000);
	while(SD_WriteCmd(CMD17,addr,0xFF) != 0){
		REPORT_REPEAT_ERROR_SD(1)
	}
	SET_REPEAT_SD(10000);
	while(SD_Get_Byte() != 0xFE){
		REPORT_REPEAT_ERROR_SD(1)
	}

	_transfer_rx(NULL, buf, 512);

	SD_Get_Byte();
	SD_Get_Byte();
	SD_WaitBusy();
	SD_WAIT_READY();
	//am_util_stdio_printf("SD_ReadSingleBlock: %d\n", addr);
	return 0;
}

uint32_t SD_ReadMultipleBlock(uint32_t addr, uint8_t *buf, uint32_t count)
{
	SD_WaitReady();
	SET_REPEAT_SD(1000);
	while(SD_WriteCmd(CMD18,addr,0xFF) != 0){
		REPORT_REPEAT_ERROR_SD(1)
	}
	do
	{
		SET_REPEAT_SD(1000);
		while(SD_Get_Byte() != 0xFE){
			REPORT_REPEAT_ERROR_SD(1)
		}

		_transfer_rx(NULL, buf, 512);

		SD_Get_Byte();                                                                    
		SD_Get_Byte();
		buf += 512;
	}while (--count);
	SD_WriteCmd(CMD12,0x00,0xFF);  ////needs error checking
	SD_WaitBusy();
	SD_WAIT_READY();
	//am_util_stdio_printf("SD_ReadMultipleBlock: %d\n", count);
	return 0;
}

uint32_t SD_WriteSingleBlock(uint32_t addr,const uint8_t *buf)
{
	uint8_t  temp;
	SD_WaitReady();
	SET_REPEAT_SD(100);
	while(SD_WriteCmd(CMD24,addr,0xFF) != 0){
		REPORT_REPEAT_ERROR_SD(1)
	}
	// Send start token
	SD_Send_Byte(0xFE);

	// Send data: 512 bytes
	_transfer_tx(buf, NULL, 512);

	// wait for Data response
	SET_REPEAT_SD(100);
	while((temp = SD_Get_Byte()) == 0xFF) { REPORT_REPEAT_ERROR_SD(1) }
	temp &= 0x1F;
	if (temp != 0x05) {
		//am_util_stdio_printf("Error data resp.: %d\n", temp);
		return 1;
	}
	SD_WaitBusy();
	SD_WaitReady();
	//am_util_stdio_printf("SD_WriteSingleBlock: %d\n", addr);
	return 0;
}

uint32_t SD_WriteMultipleBlock(uint32_t addr, const uint8_t *buf, uint32_t count)
{
	uint8_t temp;
	SD_WAIT_READY()
	//Write multipleBlock command
	SET_REPEAT_SD(100);
	while(SD_WriteCmd(CMD25,addr,0xFF) != 0){
		REPORT_REPEAT_ERROR_SD(1)
	}
	do
	{
		// Send start block token
		SD_Send_Byte(0xFC);

		// Send data: 512 bytes
		_transfer_tx(buf, NULL, 512);

		// wait for Data response
		SET_REPEAT_SD(100);
		while((temp = SD_Get_Byte()) == 0xFF) { REPORT_REPEAT_ERROR_SD(1) }
		temp &= 0x1F;
		// Block write failure?
		if (temp != 0x05) {
			//am_util_stdio_printf("Error data resp.: %d\n", temp);
			return 1;
		}
		// Card goes busy to write the block to the flash
		SD_WaitBusy();
		// Next block
		buf += 512;
	}while (--count);
	// Send stop transmission token
	SD_Send_Byte(0xFD);
	SD_Send_Byte(0xFF);
	// Wait card to finish
	SD_WaitBusy();
	SD_WAIT_READY();
	//am_util_stdio_printf("SD_WriteMultipleBlock: %d %d\n", addr, count);
	return 0;
}
