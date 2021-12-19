/********************************************************************************
  * @file    heap.c
  * @author  ddy
  * @version V0.2
  * @date    2021-04-29
  * @brief   ��̬�ڴ����
  *1)�����OS����ΪHEAP�����飬��СΪHEAP_SIZE�ֽ�
  *2)�ѿռ�ʹ��ǰ��Ҫ��ʼ���ڴ���Ϣ�鹹�ɰ���ַ��С�������е�˫����Ͱ������ڴ���С�����еĵ�����
  *3)mini_printf_Double_List��mini_printf_Sole_List�����ڴ����debug��
  *4)mini_freeʱ�����ͷ��ڴ��ַ�Ƿ�Ϊ֮ǰmini_malloc�ĵ�ַ���ͷŵ��Ƿ�Ϊbusy�飬�˿ռ��Ƿ���һ���ڴ��Ƿ���̤
  *5)mini_free�ͷŵ�ǰ�ڴ�����Ϊfree�飬�ڼ����һ���ڴ���Ƿ�Ϊfree״̬���������ϲ�����
  *6)Ȼ������һ���ڴ���Ƿ�Ϊfree״̬��������ٴκϲ�����Ч����ڴ���Ƭ�����⡣
  *7)mini_free��ִ�й����лὫ�ϵ�free���free�������Ƴ������ϲ�����¿鰴���ڴ���С����С�������²��뵽free������
  *8)mini_mallo���free�ڴ浥�����д�С�鵽�����һ���Ҹպ÷��ϴ�С���ڴ��(���ƥ�䷨)���û���ͬʱ�����˫����͵�����
  *9)HeapInit��������ʼ��ַ����8�ֽڶ������������mini_malloʱ�������want_size�����8�ֽڶ����size���û�()
  *10)mini_mallo��mini_free�ķ��ʶ�������ٽ籣������(�رյ�����)��
  ********************************************************************************/
#include "mini_libc.h"
#include "heap.h"
#include "kernel.h"
unsigned char HEAP[HEAP_SIZE]={0};

MemoryInforBlockNode *DoubleListHead = NULL;
MemoryInforBlockNode *SoleListHead = NULL;
//----------------------------------------------------------------------------------//
void HeapInit(void)
{
	MemoryInforBlockNode *temp = (MemoryInforBlockNode *) (((unsigned int)(&HEAP[7])) & (~0x00000007));//��Heap��ʼ��ַ����4�ֽڵ�ַ���� 
	mini_memset(HEAP,0,HEAP_SIZE);
	temp->MemoryBlockSize       = HEAP_SIZE - (unsigned int)temp + (unsigned int)HEAP - sizeof(MemoryInforBlockNode); 
	temp->MemoryBlockInfor.crc  = ADDR_CRC(temp);
    temp->MemoryBlockInfor.flag = FreeBlock;
	temp->Isolate_Zone          = Isolate_Zone_Flag;//ȫFF������
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
		if(free_block->MemoryBlockSize < SoleListHead->MemoryBlockSize)  //�׽ڵ���� 
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
			if(temp->SoleListNext == NULL) //β������ 
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
static MemoryInforBlockNode *search_best_freeblock(unsigned int size)
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
	freeblock2 = (MemoryInforBlockNode *) ((unsigned int)addr - sizeof(MemoryInforBlockNode));
	
	//configASSERT(freeblock2->Isolate_Zone == Isolate_Zone_Flag);
	if(freeblock2->Isolate_Zone != Isolate_Zone_Flag)   result = -1;//�����ڴ��̤�¹ʻ�
	else if((freeblock2->MemoryBlockInfor.crc != ADDR_CRC(freeblock2)) || (freeblock2->MemoryBlockInfor.flag != BusyBlock)) result = 1; //free��ַ�Ƿ� 
	else
	{
		freeblock2->MemoryBlockInfor.flag = FreeBlock;
		freeblock4 = freeblock2;
		//���ºϲ���Ƭ�ڴ� 
		if(freeblock2->DoubleListNext->MemoryBlockInfor.flag == FreeBlock)
		{
			freeblock3 = freeblock2->DoubleListNext;
			sole_list_remove(freeblock3);//remove freeblock3
			freeblock2->MemoryBlockSize += freeblock3->MemoryBlockSize + sizeof(MemoryInforBlockNode);
			
			freeblock2->DoubleListNext = freeblock3->DoubleListNext;
			if(freeblock3->DoubleListNext != NULL) freeblock3->DoubleListNext->DoubleListPrevious = freeblock2;
			freeblock4 = freeblock2;	
		}
		//���Ϻϲ���Ƭ�ڴ�
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
void *mini_malloc(unsigned int want_size) 
{
	void *result = NULL;
	unsigned int size;
	MemoryInforBlockNode *new_block;
	MemoryInforBlockNode *temp;
	
	Pend_Schedule();
	size = ((want_size/8) + ((want_size % 8) ? 1 : 0)) * 8;//���봦�� 
	temp = search_best_freeblock(size);
	if(NULL == temp) result = NULL;
	else
	{
		sole_list_remove(temp);
		result = (void *) ((unsigned int)temp + sizeof(MemoryInforBlockNode));
		new_block = (MemoryInforBlockNode *) ((unsigned int)temp + size + sizeof(MemoryInforBlockNode));

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

