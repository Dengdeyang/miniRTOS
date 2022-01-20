#ifndef __MINI_LINC_H
#define __MINI_LINC_H

#define printf_buf_size 1000
#define NULL 0

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
	unsigned int queue_head;
	unsigned int queue_tail;
	unsigned int queue_index;
	unsigned int queue_length;
}queue;
	
void *mini_memset(void *dest, int data, unsigned int length);
void *mini_memcpy(void *dest, const void *src, unsigned int length);
void *mini_memmove(void *dest, const void *src, unsigned int length);
int mini_memcmp(const void *cmp1 ,const void *cmp2, unsigned int length);
const void *mini_memchr(const void *dest ,int chr, unsigned int length);
const char* mini_strstr(const char* src, const char* sub);
int mini_strcasecmp(const char *str1, const char *str2);
char *mini_strncpy(char *dest, const char *src, unsigned int length);
char *mini_strcpy(char *dest, const char *src);
int mini_strncmp(const char *cmp1, const char *cmp2, unsigned int length);
int mini_strcmp(const char *cmp1, const char *cmp2);
unsigned int mini_strnlen(const char *str, unsigned int maxlen);
unsigned int mini_strlen(const char *str);
char *mini_strcat(char *dest, const char *src);
char *mini_strncat(char *dest, const char *src, unsigned int length);
const char *mini_strchr(const char *str, int chr);
void mini_sprintf(queue *buf , const char *fmt, ...);
void mini_printf(const char *fmt, ...);
void queue_init(queue *buf,unsigned int length);
void mini_strout(queue *buf);
extern queue printf_buf;
#endif
