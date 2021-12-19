#ifndef __CRC_H_
#define __CRC_H_

#define true 1
#define false 0

typedef unsigned char uint8_t; 
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef char bool;

typedef struct 
{
	uint8_t width;
	uint32_t poly;
	uint32_t init;
	bool refIn;
	bool refOut;
	uint32_t xorOut;
}CRC_Type;

typedef enum 
{
	REF_4BIT = 4,
	REF_5BIT = 5,
	REF_6BIT = 6,
	REF_7BIT = 7,
	REF_8BIT = 8,
	REF_16BIT = 16,
	REF_32BIT = 32
}REFLECTED_MODE;

extern const CRC_Type crc4_ITU;           
extern const CRC_Type crc5_EPC;           
extern const CRC_Type crc5_ITU;           
extern const CRC_Type crc5_USB;           
extern const CRC_Type crc6_ITU;           
extern const CRC_Type crc7_MMC;           
extern const CRC_Type crc8;              
extern const CRC_Type crc8_ITU;           
extern const CRC_Type crc8_ROHC;          
extern const CRC_Type crc8_MAXIM;         
extern const CRC_Type crc16_IBM;          
extern const CRC_Type crc16_MAXIM;        
extern const CRC_Type crc16_USB;          
extern const CRC_Type crc16_MODBUS;       
extern const CRC_Type crc16_CCITT;        
extern const CRC_Type crc16_CCITT_FALSE ; 
extern const CRC_Type crc16_X25;          
extern const CRC_Type crc16_XMODEM ;      
extern const CRC_Type crc16_DNP ;         
extern const CRC_Type crc32;              
extern const CRC_Type crc32_MPEG2;        

uint32_t CrcCheck(CRC_Type crcType, const uint8_t *buffer, uint32_t length);
#endif
