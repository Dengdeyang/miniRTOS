#ifndef __HEAP_H
#define	__HEAP_H

#define HEAP_SIZE  4096
#define ADDR_CRC(addr) (0x7FFFFFFF & (~((unsigned int)(addr))))
#define Isolate_Zone_Flag  0xabcdef5a

typedef enum 
{
	FreeBlock,
	BusyBlock,
}BlockStatue;

typedef struct
{   
    unsigned int crc  : 31;  
    unsigned int flag : 1;  
}MemoryAddrCrc;

typedef struct MemoryInforBlock
{
	unsigned int Isolate_Zone;
	MemoryAddrCrc MemoryBlockInfor;
	unsigned int MemoryBlockSize;
	struct MemoryInforBlock *DoubleListNext;
	struct MemoryInforBlock *DoubleListPrevious;
	struct MemoryInforBlock *SoleListNext;	
}MemoryInforBlockNode; 

void mini_printf_Double_List(void);
void mini_printf_Sole_List(void);
void HeapInit(void);
void *mini_malloc(unsigned int want_size);
int mini_free(void *addr);
#endif 
