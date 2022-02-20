/********************************************************************************
  * @file    mini_libc.c
  * @author  ddy
  * @version V0.2
  * @date    2021-07-10
  * @brief   mini_libc,包含基本的string和memory相关操作函数和sprintf和printf函数的实现。
  ********************************************************************************/
#include "mini_libc.h"
#include "miniRTOSport.h"

queue printf_buf;

//--------------------------------------------------------------//
//dest所指空间应该为RAM区(data或bss段)，dest不可指向string或const修饰区(存放于rodata区)
void *mini_memset(void *dest, int data, uint32 length)
{
    char *temp = (char *)dest;
	if((dest == NULL)||(length == 0)) return NULL;
	
    while(length--)
    {
        *temp++ = data;	
	}   
    return dest;
}

//--------------------------------------------------------------//
//不考虑dest与src空间重叠情况
void *mini_memcpy(void *dest, const void *src, uint32 length)
{
    char *dest_temp = (char *)dest, *src_temp = (char *)src;
	if((dest == NULL)||(src == NULL)||(length == 0)) return NULL;
	
	//Forward copy
	while (length--)
		*dest_temp++ = *src_temp++;
    
    return dest;
}

//--------------------------------------------------------------//
//考虑dest与src空间重叠情况
void *mini_memmove(void *dest, const void *src, uint32 length)
{
    char *dest_temp = (char *)dest, *src_temp = (char *)src;
	if((dest == NULL)||(src == NULL)||(length == 0)) return NULL;
	
    if (dest_temp <= src_temp || dest_temp > (src_temp + length))
    {
    	//Forward copy
        while (length--)
            *dest_temp++ = *src_temp++;
    }
    else
    {
    	//Backward copy
        do
        {
        	*(dest_temp + length - 1) = *(src_temp + length - 1);
		}
		while (--length);
    }
    return dest;
}

//--------------------------------------------------------------//
int mini_memcmp(const void *cmp1 ,const void *cmp2, uint32 length)
{
    const uint8 *cmp1_temp = (uint8 *)cmp1;
	const uint8 *cmp2_temp = (uint8 *)cmp2;
    int res = 0;

    while(length--)
    {
    	if ((res = *cmp1_temp++ - *cmp2_temp++) != 0)
        break;
	}
	if(res < 0) return -1;
	else if(res > 0) return 1;
	else return 0;
}

//--------------------------------------------------------------//
const void *mini_memchr(const void *dest ,int chr, uint32 length)
{
    const uint8 *dest_temp = (uint8 *)dest;
	if((dest == NULL)||(length == 0)) return NULL;
	
    while(length--)
    {
    	if ((uint8)chr == *dest_temp)  return dest_temp;
    	dest_temp++;
	}
	return NULL;
}

//--------------------------------------------------------------//
const char* mini_strstr(const char* src, const char* sub)
{
    const char *src_temp;
    const char *sub_temp;
    if((src == NULL)||(sub == NULL)) return NULL;
    
    for(;*src != '\0';src++)
    {
    	src_temp = src;
		sub_temp = sub;
        while(*src_temp++ == *sub_temp++)
        {
        	if(*sub_temp == '\0')  return src;
		}
    }
    return NULL;
}

//--------------------------------------------------------------//
int mini_strcasecmp(const char *str1, const char *str2)
{
    char temp_str1, temp_str2;

    do
    {
        temp_str1 = *str1++;
        temp_str2 = *str2++;
        if (temp_str1 >= 'A' && temp_str1 <= 'Z')  temp_str1 += 'a' - 'A';
        if (temp_str2 >= 'A' && temp_str2 <= 'Z')  temp_str2 += 'a' - 'A';
    }
    while (temp_str1 == temp_str2 && temp_str1 != '\0');

	if(temp_str1 > temp_str2) return 1;
	else if(temp_str1 < temp_str2) return -1;
	else return 0;
}

//--------------------------------------------------------------//
char *mini_strncpy(char *dest, const char *src, uint32 length)
{ 
    char *temp = dest;
	if((dest == NULL)||(src == NULL)||(length == 0)) return NULL;
    
    while (length--)
    {
    	*dest++ = *src++;
	}
    return temp;
}

//--------------------------------------------------------------//
char *mini_strcpy(char *dest, const char *src)
{
	char *dest_temp = dest;
	if((dest == NULL)||(src == NULL)) return NULL;
	
	do
	{
		*dest_temp++ = *src;
	}
	while(*src++ != '\0');
		
    return dest;
}

//--------------------------------------------------------------//
int mini_strncmp(const char *cmp1, const char *cmp2, uint32 length)
{
    int res = *cmp1 - *cmp2;
    while(length--)
    {
    	if ((res = *cmp1++ - *cmp2++) != 0)
        break;
	}

	return res;
} 

//--------------------------------------------------------------//
int mini_strcmp(const char *cmp1, const char *cmp2)
{
    while (*cmp1 == *cmp2 && *cmp1 != '\0')
    {
    	cmp1++;
    	cmp2++;
	}
	if(*cmp1 > *cmp2) return 1;
	else if(*cmp1 < *cmp2) return -1;
	else return 0;
}

//--------------------------------------------------------------//
uint32 mini_strnlen(const char *str, uint32 maxlen)
{
    const char *str_temp;

    for (str_temp = str; *str_temp != '\0' && str_temp - str < maxlen; str_temp++);

    return str_temp - str;
}

//--------------------------------------------------------------//
uint32 mini_strlen(const char *str)
{
    const char *str_temp;

    for (str_temp = str; *str_temp != '\0'; str_temp++);

    return str_temp - str;
}

//--------------------------------------------------------------//
char *mini_strcat(char *dest, const char *src)
{
	char *temp = dest;
	if((src == NULL)||(dest == NULL)) return NULL;
	for(;*temp != '\0';temp++);
	while(*src != '\0')
	{
		*temp++ = *src++;
	}
	*temp = *src;
	
	return dest;
} 

//--------------------------------------------------------------//
char *mini_strncat(char *dest, const char *src, uint32 length)
{
	char *temp = dest;
	if((src == NULL)||(dest == NULL)||(length == 0)) return NULL;
	for(;*temp != '\0';temp++);
	while(length--)
	{
		*temp++ = *src++;
	}
	*temp = '\0';
	
	return dest;
} 

//--------------------------------------------------------------//
const char *mini_strchr(const char *str, int chr)
{
	if(str == NULL) return NULL;
	
	for(;*str != '\0' || (char)chr == '\0';str++)
	{
		if ((char)chr == *str)  return str;
	}
	
	return NULL;
}

//--------------------------------------------------------------//
void queue_init(queue *buf,uint32 length)
{
	uint32 i;
	for(i=0;i<length;i++)
		buf->queue_data[i] = 0;
    buf->queue_head=0;
    buf->queue_tail=0;
	buf->queue_index=0;
	buf->queue_length = length;
}

//入队
char push_queue(queue *data,const char *push_data,uint32 length)
{
	if(length > (data->queue_length) - (data->queue_index))
	{
		return 0;
	}
	else
	{
		while(length--)
		{
			data->queue_data[data->queue_tail] = *push_data++;
			if(data->queue_tail == data->queue_length-1) data->queue_tail = 0;
			else data->queue_tail++;
			data->queue_index++;
		}
		return 1;
	}
}

//出队
char pull_queue(queue *data,char *pull_data,uint32 length)
{
	if(length > (data->queue_index))
	{
		return 0;
	}
	else 
	{
		while(length--)
		{
			*pull_data++ = data->queue_data[data->queue_head];
			if(data->queue_head == data->queue_length-1) data->queue_head = 0;
			else data->queue_head++;
			data->queue_index--;
		}
		return 1;
	}
}

//--------------------------------------------------------------//
void mini_sendstr(char *str)
{
	if(!str) return;
	while (*str != '\0')
	{
		mini_sendchr(*str++);
	}
}

//--------------------------------------------------------------//
void mini_strout(queue *buf)
{
	char data;
	if(pull_queue(buf,&data,1)) mini_sendchr(data);
}

//--------------------------------------------------------------//
char data_format_switch(uint32 value, char *str, uint32 format)
{
	char data;
	int temp;
	char *str_head  = str; 
	char *str_tail  = str; 
	
	if ((NULL == str) || (format < 2))  return 1;
	
	if ((0 > (int)value) && (10 == format))
	{
		*str_tail++ = '-';
		str_head++;
		value = -value;
	}

	do
	{
		temp = (uint32)(value % format);
		value /= format; 

		if (temp < 10)
		{
			*str_tail++ = (temp + '0' - 0); // a digit
		}
		else
		{
			*str_tail++ = (temp + 'A' - 10); // a letter
		}
	} while(value > 0);
    *str_tail = '\0';
	for(--str_tail;str_head<=str_tail;str_tail--,str_head++)
	{
		data = *str_tail;
		*str_tail = *str_head;
		*str_head = data;
	}
	return 0;
}

void mini_sprintf(queue *buf , const char *fmt, ...)
{
	char *str; 
	char ch;
	int xd; 
	double fd;
	char switch_buf[36]={0};
	va_list ap; 

	va_start(ap, fmt); 
	
	for (; *fmt != '\0'; fmt++)
	{
		if (*fmt != '%')
		{
			push_queue(buf,fmt,1);
			continue;
		}
		switch (*++fmt)
		{
			case 's':
				str = va_arg(ap, char*);
				push_queue(buf,str,mini_strlen(str));
				break;
		
			case 'x':
			case 'X':
				xd = va_arg(ap, int);
				data_format_switch(xd, switch_buf, 16);
				push_queue(buf,switch_buf,mini_strlen(switch_buf));
				break;
		
			case 'd':
				xd  = va_arg(ap, int);
				data_format_switch(xd, switch_buf, 10);
				push_queue(buf,switch_buf,mini_strlen(switch_buf));
				break;
				
			case 'o':
				xd  = va_arg(ap, int);
				data_format_switch(xd, switch_buf, 8);
				push_queue(buf,switch_buf,mini_strlen(switch_buf));
				break;
				
			case 'b':
				xd  = va_arg(ap, int);
				data_format_switch(xd, switch_buf, 2);
				push_queue(buf,switch_buf,mini_strlen(switch_buf));
				break;
		
			case 'f':
				fd  = va_arg(ap, double);
				xd = (int)fd;
				data_format_switch(xd, switch_buf, 10);
				push_queue(buf,switch_buf,mini_strlen(switch_buf));
				
				ch = '.';
				push_queue(buf,&ch,1);
		
				if(fd > 0) xd = (uint32)((fd - xd)*10000000);
				else       xd = (uint32)((xd - fd)*10000000);
				
				data_format_switch(xd, switch_buf, 10);
				push_queue(buf,switch_buf,mini_strlen(switch_buf));
				break;
				
			case 'c':
				ch = (char)va_arg(ap, int);
				push_queue(buf,&ch,1);
				break;
		
			default:
				push_queue(buf,fmt,1);
				break;
			}
	}
	va_end(ap);
}

void mini_printf(const char *fmt, ...)
{
	char *str; 
	char ch;
	int xd; 
	double fd;
	char switch_buf[36]={0};
	va_list ap; 

	va_start(ap, fmt); 
	
	for (; *fmt != '\0'; fmt++)
	{
		if (*fmt != '%')
		{
			mini_sendchr(*fmt);
			continue;
		}
		switch (*++fmt)
		{
			case 's':
				str = va_arg(ap, char*);
				mini_sendstr(str);
				break;
		
			case 'x':
			case 'X':
				xd = va_arg(ap, int);
				data_format_switch(xd, switch_buf, 16);
				mini_sendstr(switch_buf);
				break;
		
			case 'd':
				xd  = va_arg(ap, int);
				data_format_switch(xd, switch_buf, 10);
				mini_sendstr(switch_buf);
				break;
				
			case 'o':
				xd  = va_arg(ap, int);
				data_format_switch(xd, switch_buf, 8);
				mini_sendstr(switch_buf);
				break;
				
			case 'b':
				xd  = va_arg(ap, int);
				data_format_switch(xd, switch_buf, 2);
				mini_sendstr(switch_buf);
				break;
		
			case 'f':
				fd  = va_arg(ap, double);
				xd = (int)fd;
				data_format_switch(xd, switch_buf, 10);
				mini_sendstr(switch_buf);
		
				mini_sendchr('.');
		
				if(fd > 0) xd = (uint32)((fd - xd)*10000000);
				else       xd = (uint32)((xd - fd)*10000000);
				
				data_format_switch(xd, switch_buf, 10);
				mini_sendstr(switch_buf);
				break;
				
			case 'c':
				ch = (char)va_arg(ap, int);
				mini_sendchr(ch);
				break;
		
			default:
				mini_sendchr(*fmt);
				break;
			}
	}
	va_end(ap);
}

