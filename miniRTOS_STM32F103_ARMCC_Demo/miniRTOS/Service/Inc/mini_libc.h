#ifndef __MINI_LINC_H
#define __MINI_LINC_H

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;

#define printf_buf_size 1000

#if defined (__GNUC__)
	#define va_start(v,l)           __builtin_va_start(v,l)
	#define va_end(v)               __builtin_va_end(v)
	#define va_arg(v,l)             __builtin_va_arg(v,l)
    typedef __builtin_va_list       __gnuc_va_list;
    typedef __gnuc_va_list          va_list;
#else
	#define va_start(ap, parmN) 	__va_start(ap, parmN)
	#define va_arg(ap, type) 		__va_arg(ap, type)
	#define va_end(ap) 				__va_end(ap)
	typedef struct __va_list { void *__ap; } va_list;
#endif

typedef struct 
{
	char queue_data[printf_buf_size];
	uint32 queue_head;
	uint32 queue_tail;
	uint32 queue_index;
	uint32 queue_length;
}queue;
	
void *mini_memset(void *dest, int data, uint32 length);
void *mini_memcpy(void *dest, const void *src, uint32 length);
void *mini_memmove(void *dest, const void *src, uint32 length);
int mini_memcmp(const void *cmp1 ,const void *cmp2, uint32 length);
const void *mini_memchr(const void *dest ,int chr, uint32 length);
const char* mini_strstr(const char* src, const char* sub);
int mini_strcasecmp(const char *str1, const char *str2);
char *mini_strncpy(char *dest, const char *src, uint32 length);
char *mini_strcpy(char *dest, const char *src);
int mini_strncmp(const char *cmp1, const char *cmp2, uint32 length);
int mini_strcmp(const char *cmp1, const char *cmp2);
uint32 mini_strnlen(const char *str, uint32 maxlen);
uint32 mini_strlen(const char *str);
char *mini_strcat(char *dest, const char *src);
char *mini_strncat(char *dest, const char *src, uint32 length);
const char *mini_strchr(const char *str, int chr);
void mini_sprintf(queue *buf , const char *fmt, ...);
void mini_printf(const char *fmt, ...);
void queue_init(queue *buf,uint32 length);
void mini_strout(queue *buf);
extern queue printf_buf;
#endif
