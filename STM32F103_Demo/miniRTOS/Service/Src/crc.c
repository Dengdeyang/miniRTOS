/********************************************************************************
  * @file    crc.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   计算各种类型CRC值算法
  ********************************************************************************/
  
#include "crc.h"
const CRC_Type crc4_ITU = { 4, 0x03, 0x00, true, true, 0x00 };
const CRC_Type crc5_EPC = { 5, 0x09, 0x09, false, false, 0x00 };
const CRC_Type crc5_ITU = { 5, 0x15, 0x00, true, true, 0x00 };
const CRC_Type crc5_USB = { 5, 0x05, 0x1f, true, true, 0x1f };
const CRC_Type crc6_ITU = { 6, 0x03, 0x00, true, true, 0x00 };
const CRC_Type crc7_MMC = { 7, 0x09, 0x00, false, false, 0x00 };
const CRC_Type crc8 = { 8, 0x07, 0x00, false, false, 0x00 };
const CRC_Type crc8_ITU = { 8, 0x07, 0x00, false, false, 0x55 };
const CRC_Type crc8_ROHC = { 8, 0x07, 0xff, true, true, 0x00 };
const CRC_Type crc8_MAXIM = { 8, 0x31, 0x00, true, true, 0x00 };
const CRC_Type crc16_IBM = { 16, 0x8005, 0x0000, true, true, 0x0000 };
const CRC_Type crc16_MAXIM = { 16, 0x8005, 0x0000, true, true, 0xffff };
const CRC_Type crc16_USB = { 16, 0x8005, 0xffff, true, true, 0xffff };
const CRC_Type crc16_MODBUS = { 16, 0x8005, 0xffff, true, true, 0x0000 };
const CRC_Type crc16_CCITT = { 16, 0x1021, 0x0000, true, true, 0x0000 };
const CRC_Type crc16_CCITT_FALSE = { 16, 0x1021, 0xffff, false, false, 0x0000 };
const CRC_Type crc16_X25 = { 16, 0x1021, 0xffff, true, true, 0xffff };
const CRC_Type crc16_XMODEM = { 16, 0x1021, 0x0000, false, false, 0x0000 };
const CRC_Type crc16_DNP = { 16, 0x3D65, 0x0000, true, true, 0xffff };
const CRC_Type crc32 = { 32, 0x04c11db7, 0xffffffff, true, true, 0xffffffff };
const CRC_Type crc32_MPEG2 = { 32, 0x4c11db7, 0xffffffff, false, false, 0x00000000 };



static uint32_t ReflectedData(uint32_t data, REFLECTED_MODE mode)
{
	data = ((data & 0xffff0000) >> 16) | ((data & 0x0000ffff) << 16);
	data = ((data & 0xff00ff00) >> 8) | ((data & 0x00ff00ff) << 8);
	data = ((data & 0xf0f0f0f0) >> 4) | ((data & 0x0f0f0f0f) << 4);
	data = ((data & 0xcccccccc) >> 2) | ((data & 0x33333333) << 2);
	data = ((data & 0xaaaaaaaa) >> 1) | ((data & 0x55555555) << 1);
	
	switch (mode)
	{
		case REF_32BIT:
		return data;
		case REF_16BIT:
		return (data >> 16) & 0xffff;
		case REF_8BIT:
		return (data >> 24) & 0xff;
		case REF_7BIT:
		return (data >> 25) & 0x7f;
		case REF_6BIT:
		return (data >> 26) & 0x7f;
		case REF_5BIT:
		return (data >> 27) & 0x1f;
		case REF_4BIT:
		return (data >> 28) & 0x0f;
	}
	return 0;
}


static uint8_t CheckCrc4(uint8_t poly, uint8_t init, bool refIn, bool refOut, uint8_t xorOut,
 const uint8_t *buffer, uint32_t length)
{
 uint8_t i;
 uint8_t crc;

 if (refIn == true)
 {
  crc = init;
  poly = ReflectedData(poly, REF_4BIT);

  while (length--)
  {
   crc ^= *buffer++;
   for (i = 0; i < 8; i++)
   {
    if (crc & 0x01)
    {
     crc >>= 1;
     crc ^= poly;
    }
    else
    {
     crc >>= 1;
    }
   }
  }

  return crc ^ xorOut;
 }
 else
 {
  crc = init << 4;
  poly <<= 4;

  while (length--)
  {
   crc ^= *buffer++;
   for (i = 0; i < 8; i++)
   {
    if (crc & 0x80)
    {
     crc <<= 1;
     crc ^= poly;
    }
    else
    {
     crc <<= 1;
    }
   }
  }

  return (crc >> 4) ^ xorOut;
 }
}

static uint8_t CheckCrc5(uint8_t poly, uint8_t init, bool refIn, bool refOut, uint8_t xorOut,
 const uint8_t *buffer, uint32_t length)
{
 uint8_t i;
 uint8_t crc;

 if (refIn == true)
 {
  crc = init;
  poly = ReflectedData(poly, REF_5BIT);

  while (length--)
  {
   crc ^= *buffer++;
   for (i = 0; i < 8; i++)
   {
    if (crc & 0x01)
    {
     crc >>= 1;
     crc ^= poly;
    }
    else
    {
     crc >>= 1;
    }
   }
  }

  return crc ^ xorOut;
 }
 else
 {
  crc = init << 3;
  poly <<= 3;

  while (length--)
  {
   crc ^= *buffer++;
   for (i = 0; i < 8; i++)
   {
    if (crc & 0x80)
    {
     crc <<= 1;
     crc ^= poly;
    }
    else
    {
     crc <<= 1;
    }
   }
  }

  return (crc >> 3) ^ xorOut;
 }
}

static uint8_t CheckCrc6(uint8_t poly, uint8_t init, bool refIn, bool refOut, uint8_t xorOut,
 const uint8_t *buffer, uint32_t length)
{
 uint8_t i;
 uint8_t crc;

 if (refIn == true)
 {
  crc = init;
  poly = ReflectedData(poly, REF_6BIT);

  while (length--)
  {
   crc ^= *buffer++;
   for (i = 0; i < 8; i++)
   {
    if (crc & 0x01)
    {
     crc >>= 1;
     crc ^= poly;
    }
    else
    {
     crc >>= 1;
    }
   }
  }

  return crc ^ xorOut;
 }
 else
 {
  crc = init << 2;
  poly <<= 2;

  while (length--)
  {
   crc ^= *buffer++;
   for (i = 0; i < 8; i++)
   {
    if (crc & 0x80)
    {
     crc <<= 1;
     crc ^= poly;
    }
    else
    {
     crc <<= 1;
    }
   }
  }

  return (crc >> 2) ^ xorOut;
 }
}

static uint8_t CheckCrc7(uint8_t poly, uint8_t init, bool refIn, bool refOut, uint8_t xorOut,
 const uint8_t *buffer, uint32_t length)
{
 uint8_t i;
 uint8_t crc;

 if (refIn == true)
 {
  crc = init;
  poly = ReflectedData(poly, REF_7BIT);

  while (length--)
  {
   crc ^= *buffer++;
   for (i = 0; i < 8; i++)
   {
    if (crc & 0x01)
    {
     crc >>= 1;
     crc ^= poly;
    }
    else
    {
     crc >>= 1;
    }
   }
  }

  return crc ^ xorOut;
 }
 else
 {
  crc = init << 1;
  poly <<= 1;

  while (length--)
  {
   crc ^= *buffer++;
   for (i = 0; i < 8; i++)
   {
    if (crc & 0x80)
    {
     crc <<= 1;
     crc ^= poly;
    }
    else
    {
     crc <<= 1;
    }
   }
  }

  return (crc >> 1) ^ xorOut;
 }
}

static uint8_t CheckCrc8(uint8_t poly, uint8_t init, bool refIn, bool refOut, uint8_t xorOut,
 const uint8_t *buffer, uint32_t length)
{
 uint32_t i = 0;
 uint8_t crc = init;

 while (length--)
 {
  if (refIn == true)
  {
   crc ^= ReflectedData(*buffer++, REF_8BIT);
  }
  else
  {
   crc ^= *buffer++;
  }

  for (i = 0; i < 8; i++)
  {
   if (crc & 0x80)
   {
    crc <<= 1;
    crc ^= poly;
   }
   else
   {
    crc <<= 1;
   }
  }
 }

 if (refOut == true)
 {
  crc = ReflectedData(crc, REF_8BIT);
 }

 return crc ^ xorOut;
}

static uint16_t CheckCrc16(uint16_t poly, uint16_t init, bool refIn, bool refOut, uint16_t xorOut,
 const uint8_t *buffer, uint32_t length)
{
 uint32_t i = 0;
 uint16_t crc = init;

 while (length--)
 {
  if (refIn == true)
  {
   crc ^= ReflectedData(*buffer++, REF_8BIT) << 8;
  }
  else
  {
   crc ^= (*buffer++) << 8;
  }

  for (i = 0; i < 8; i++)
  {
   if (crc & 0x8000)
   {
    crc <<= 1;
    crc ^= poly;
   }
   else
   {
    crc <<= 1;
   }
  }
 }

 if (refOut == true)
 {
  crc = ReflectedData(crc, REF_16BIT);
 }

 return crc ^ xorOut;
}

static uint32_t CheckCrc32(uint32_t poly, uint32_t init, bool refIn, bool refOut, uint32_t xorOut,
 const uint8_t *buffer, uint32_t length)
{
 uint32_t i = 0;
 uint32_t crc = init;

 while (length--)
 {
  if (refIn == true)
  {
   crc ^= ReflectedData(*buffer++, REF_8BIT) << 24;
  }
  else
  {
   crc ^= (*buffer++) << 24;
  }

  for (i = 0; i < 8; i++)
  {
   if (crc & 0x80000000)
   {
    crc <<= 1;
    crc ^= poly;
   }
   else
   {
    crc <<= 1;
   }
  }
 }

 if (refOut == true)
 {
  crc = ReflectedData(crc, REF_32BIT);
 }

 return crc ^ xorOut;
}

uint32_t CrcCheck(CRC_Type crcType, const uint8_t *buffer, uint32_t length)
{
	 switch (crcType.width)
	 {
		 case 4:
		  return CheckCrc4(crcType.poly, crcType.init, crcType.refIn, crcType.refOut,
		   crcType.xorOut, buffer, length);
		 case 5:
		  return CheckCrc5(crcType.poly, crcType.init, crcType.refIn, crcType.refOut,
		   crcType.xorOut, buffer, length);
		 case 6:
		  return CheckCrc6(crcType.poly, crcType.init, crcType.refIn, crcType.refOut,
		   crcType.xorOut, buffer, length);
		 case 7:
		  return CheckCrc7(crcType.poly, crcType.init, crcType.refIn, crcType.refOut,
		   crcType.xorOut, buffer, length);
		 case 8:
		  return CheckCrc8(crcType.poly, crcType.init, crcType.refIn, crcType.refOut,
		   crcType.xorOut, buffer, length);
		 case 16:
		  return CheckCrc16(crcType.poly, crcType.init, crcType.refIn, crcType.refOut,
		   crcType.xorOut, buffer, length);
		 case 32:
		  return CheckCrc32(crcType.poly, crcType.init, crcType.refIn, crcType.refOut,
		   crcType.xorOut, buffer, length);
	 }
	 return 0;
}

/*
#include <stdio.h>
void test_demo(void)
{
	uint8_t data[]={0x08,0x00};
    CRC_Type my_crc;
	
	my_crc.width = 5;
	my_crc.poly = 0x1b;
	my_crc.init = 0x00;
	my_crc.refIn = false;
	my_crc.refOut = false;
	my_crc.xorOut = 0x00;
	
	printf("0x%X\r\n",CrcCheck(my_crc,data,sizeof(data)));
	
	printf("0x%X\r\n",CrcCheck(crc4_ITU,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc5_EPC,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc5_ITU,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc5_USB,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc6_ITU,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc7_MMC,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc8,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc8_ITU,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc8_ROHC,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc8_MAXIM,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc16_IBM,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc16_MAXIM,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc16_USB,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc16_MODBUS,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc16_CCITT,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc16_CCITT_FALSE,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc16_X25,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc16_XMODEM,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc16_DNP,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc32,data,sizeof(data)));
	printf("0x%X\r\n",CrcCheck(crc32_MPEG2,data,sizeof(data)));
}
*/
