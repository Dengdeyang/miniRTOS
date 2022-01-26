#ifndef __HEAP_H
#define	__HEAP_H

#define HEAP_SIZE  4096
#define ADDR_CRC(addr) (0x7FFFFFFF & (~((uint32)(addr))))
#define Isolate_Zone_Flag  0xabcdef5a

typedef enum 
{
	FreeBlock,
	BusyBlock,
}BlockStatue;

typedef struct
{   
    uint32 crc  : 31;  
    uint32 flag : 1;  
}MemoryAddrCrc;

typedef struct MemoryInforBlock
{
	uint32 Isolate_Zone;
	MemoryAddrCrc MemoryBlockInfor;
	uint32 MemoryBlockSize;
	struct MemoryInforBlock *DoubleListNext;
	struct MemoryInforBlock *DoubleListPrevious;
	struct MemoryInforBlock *SoleListNext;	
}MemoryInforBlockNode; 

void mini_printf_Double_List(void);
void mini_printf_Sole_List(void);
void HeapInit(void);
void *mini_malloc(uint32 want_size);
int mini_free(void *addr);
#endif 
