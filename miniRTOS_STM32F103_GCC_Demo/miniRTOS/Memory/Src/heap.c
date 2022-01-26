/********************************************************************************
  * @file    heap.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   动态内存管理
  *1)定义的OS堆区为HEAP的数组，大小为HEAP_SIZE字节
  *2)堆空间使用前需要初始化内存信息块构成按地址从小到大排列的双链表和按空闲内存块从小打到排列的单链表
  *3)mini_printf_Double_List与mini_printf_Sole_List用于内存分析debug用
  *4)mini_free时会检查释放内存地址是否为之前mini_malloc的地址，释放的是否为busy块，此空间是否被上一个内存块非法踩踏
  *5)mini_free释放当前内存块后标记为free块，在检查下一个内存块是否为free状态，如果是则合并处理，
  *6)然后检查上一个内存块是否为free状态，如果是再次合并，有效解决内存碎片化问题。
  *7)mini_free在执行过程中会将老的free块从free单链表移除，将合并后的新块按照内存块大小，从小到大重新插入到free单链表
  *8)mini_mallo会从free内存单链表中从小块到大块逐一查找刚好符合大小的内存块(最佳匹配法)给用户，同时会更新双链表和单链表
  *9)HeapInit对数组起始地址做了8字节对齐操作，后续mini_mallo时都会根据want_size计算出8字节对齐的size给用户()
  *10)mini_mallo和mini_free的访问都添加了临界保护机制(关闭调度器)。
  ********************************************************************************/
#include "mini_libc.h"
#include "heap.h"
#include "kernel.h"
uint8 HEAP[HEAP_SIZE]={0};

MemoryInforBlockNode *DoubleListHead = NULL;
MemoryInforBlockNode *SoleListHead = NULL;
//----------------------------------------------------------------------------------//
void HeapInit(void)
{
	MemoryInforBlockNode *temp = (MemoryInforBlockNode *) (((uint32)(&HEAP[7])) & (~0x00000007));//对Heap起始地址进行4字节地址对齐 
	mini_memset(HEAP,0,HEAP_SIZE);
	temp->MemoryBlockSize       = HEAP_SIZE - (uint32)temp + (uint32)HEAP - sizeof(MemoryInforBlockNode); 
	temp->MemoryBlockInfor.crc  = ADDR_CRC(temp);
    temp->MemoryBlockInfor.flag = FreeBlock;
	temp->Isolate_Zone          = Isolate_Zone_Flag;//全FF隔离区
	temp->DoubleListNext        = NULL;
	temp->DoubleListPrevious    = NULL;
	temp->SoleListNext          = NULL;
	
	DoubleListHead = temp;
	SoleListHead = temp;
}

//----------------------------------------------------------------------------------//
void mini_printf_Double_List(void)
{
	MemoryInforBlockNode *temp;
	mini_printf("\r\nDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD\r\n");
	for(temp = DoubleListHead; temp != NULL ;temp = temp->DoubleListNext)
	{
		mini_printf("size = %d,",temp->MemoryBlockSize);
		if(temp->MemoryBlockInfor.flag) mini_printf("statue = busy,");
		else 							mini_printf("statue = free,");
			mini_printf("\r\n");
	}
	mini_printf("\r\nDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD\r\n");
}

//----------------------------------------------------------------------------------//
void mini_printf_Sole_List(void)
{
	MemoryInforBlockNode *temp;
	mini_printf("\r\nSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS\r\n");
	for(temp = SoleListHead; temp != NULL ;temp = temp->SoleListNext)
	{
		mini_printf("size = %d,",temp->MemoryBlockSize);
		if(temp->MemoryBlockInfor.flag) mini_printf("statue = busy,");
		else 							mini_printf("statue = free,");
			mini_printf("\r\n");
	}
	mini_printf("\r\nSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS\r\n");
}

//----------------------------------------------------------------------------------//
static void sole_list_insert(MemoryInforBlockNode *free_block)
{
	MemoryInforBlockNode *temp;
	if(SoleListHead == NULL) SoleListHead = free_block;
	else
	{
		if(free_block->MemoryBlockSize < SoleListHead->MemoryBlockSize)  //首节点插入 
		{
			free_block->SoleListNext =  SoleListHead;
			SoleListHead = free_block;
		}
		else
		{
			for(temp = SoleListHead; temp->SoleListNext != NULL ;temp = temp->SoleListNext)
			{
				if((free_block->MemoryBlockSize>=temp->MemoryBlockSize)&&(free_block->MemoryBlockSize<=temp->SoleListNext->MemoryBlockSize)) 
				{
					free_block->SoleListNext = temp->SoleListNext;
					temp->SoleListNext = free_block;
					break;
				}
			}
			if(temp->SoleListNext == NULL) //尾结点插入 
			{
				temp->SoleListNext = free_block;
				free_block->SoleListNext = NULL;
			}
		}
	}
}

//----------------------------------------------------------------------------------//
static void sole_list_remove(MemoryInforBlockNode *free_block)
{
	MemoryInforBlockNode *temp;
	if(free_block->MemoryBlockInfor.crc == SoleListHead->MemoryBlockInfor.crc) 
	{
		SoleListHead = SoleListHead->SoleListNext;
	}
	else
	{
		for(temp = SoleListHead; temp->SoleListNext != NULL ;temp = temp->SoleListNext)
		{
			if(temp->SoleListNext->MemoryBlockInfor.crc == free_block->MemoryBlockInfor.crc)  
			{
				temp->SoleListNext = temp->SoleListNext->SoleListNext;
				break;
			}
		}
	}
}

//----------------------------------------------------------------------------------//
static MemoryInforBlockNode *search_best_freeblock(uint32 size)
{
	MemoryInforBlockNode *temp = NULL;
    configASSERT(SoleListHead->Isolate_Zone == Isolate_Zone_Flag);
	for(temp = SoleListHead; temp != NULL ;temp = temp->SoleListNext)
	{
		if(temp->MemoryBlockSize > (size + sizeof(MemoryInforBlockNode)))
		{
			return temp;
		}
	}
	return temp;
} 

//----------------------------------------------------------------------------------//
int mini_free(void *addr)
{
	int result;
	MemoryInforBlockNode *freeblock1;
	MemoryInforBlockNode *freeblock2;
	MemoryInforBlockNode *freeblock3;
	MemoryInforBlockNode *freeblock4;
	
	Pend_Schedule();
	freeblock2 = (MemoryInforBlockNode *) ((uint32)addr - sizeof(MemoryInforBlockNode));
	
	//configASSERT(freeblock2->Isolate_Zone == Isolate_Zone_Flag);
	if(freeblock2->Isolate_Zone != Isolate_Zone_Flag)   result = -1;//发生内存踩踏事故或
	else if((freeblock2->MemoryBlockInfor.crc != ADDR_CRC(freeblock2)) || (freeblock2->MemoryBlockInfor.flag != BusyBlock)) result = 1; //free地址非法 
	else
	{
		freeblock2->MemoryBlockInfor.flag = FreeBlock;
		freeblock4 = freeblock2;
		//向下合并碎片内存 
		if(freeblock2->DoubleListNext->MemoryBlockInfor.flag == FreeBlock)
		{
			freeblock3 = freeblock2->DoubleListNext;
			sole_list_remove(freeblock3);//remove freeblock3
			freeblock2->MemoryBlockSize += freeblock3->MemoryBlockSize + sizeof(MemoryInforBlockNode);
			
			freeblock2->DoubleListNext = freeblock3->DoubleListNext;
			if(freeblock3->DoubleListNext != NULL) freeblock3->DoubleListNext->DoubleListPrevious = freeblock2;
			freeblock4 = freeblock2;	
		}
		//向上合并碎片内存
		if((freeblock2->DoubleListPrevious != NULL) && (freeblock2->DoubleListPrevious->MemoryBlockInfor.flag == FreeBlock))
		{
			freeblock1 = freeblock2->DoubleListPrevious;
			sole_list_remove(freeblock1);//remove freeblock1
			freeblock1->MemoryBlockSize += freeblock2->MemoryBlockSize + sizeof(MemoryInforBlockNode);
			if(freeblock2->DoubleListNext != NULL) freeblock2->DoubleListNext->DoubleListPrevious = freeblock1;
			freeblock1->DoubleListNext = freeblock2->DoubleListNext;
			freeblock4 = freeblock1;
		}
		sole_list_insert(freeblock4);//insert freeblock4
		result = 0;
	}
	Release_Schedule();
	return result;
}

//----------------------------------------------------------------------------------//
void *mini_malloc(uint32 want_size) 
{
	void *result = NULL;
	uint32 size;
	MemoryInforBlockNode *new_block;
	MemoryInforBlockNode *temp;
	
	Pend_Schedule();
	size = ((want_size/8) + ((want_size % 8) ? 1 : 0)) * 8;//对齐处理 
	temp = search_best_freeblock(size);
	if(NULL == temp) result = NULL;
	else
	{
		sole_list_remove(temp);
		result = (void *) ((uint32)temp + sizeof(MemoryInforBlockNode));
		new_block = (MemoryInforBlockNode *) ((uint32)temp + size + sizeof(MemoryInforBlockNode));

		new_block->MemoryBlockSize = temp->MemoryBlockSize - size - sizeof(MemoryInforBlockNode);
		temp->MemoryBlockSize = size;
		temp->MemoryBlockInfor.flag = BusyBlock;
		new_block->MemoryBlockInfor.crc = ADDR_CRC(new_block);
		new_block->MemoryBlockInfor.flag = FreeBlock;
		new_block->Isolate_Zone = Isolate_Zone_Flag;
		
		new_block->DoubleListPrevious = temp;
		new_block->DoubleListNext = temp->DoubleListNext;
		if(temp->DoubleListNext != NULL) temp->DoubleListNext->DoubleListPrevious = new_block;
		temp->DoubleListNext = new_block;
		
		sole_list_insert(new_block);
	}
	Release_Schedule();
	return result;
}

