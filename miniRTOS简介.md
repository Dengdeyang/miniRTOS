# 0、miniRTOS(designed by ddy)
代码包含四个部分：
1.Kernel部分，包含RTOS内核，IPC组件，软件定时器。
2.Memory部分，包含堆空间管理算法，重定义malloc与free。
3.Service部分，包含重定义libc库与重定义printf
4.Driver部分，包含Driver驱动API和Demo

操作系统源码和STM32F103C8T6 Demo工程下载地址：

https://gitee.com/ddyusst/miniRTOS/

已完成如下开发：

1）内核抢占式任务的调度（优先级链表排序）

2）临界段代码保护（特权模式直接开关中断，非特权模式先触发SVC，再开关中断）

3）消息队列（支持不定长度消息，实时调用heap区申请内存）

4）信号量（二值信号量，互斥信号量，支持优先级翻转解决，计数信号量）

5）mini_libc库（string+memory操作函数，sprintf和printf函数，采用ringbuffer缓冲机制）

6）软件定时器（采用软件定时器守护任务+软件定时器回调函数机制）

7）device抽象层（register，unregister，find，open，close，read，write，iocontrol等API） 

8）Heap内存管理（内存使用双链表管理空闲和非空闲block，支持内存碎片合并，单链表管理空闲block，按照空闲block容量由小到大排序，malloc时搜寻容量最接近的空闲内存块，最佳匹配法，提高内存使用效率）

# 1、RTOS操作系统的组成
    对于目前主流的RTOS，比如ucos，freeRTOS，RT-thread等等，都是属于并发的线程，其实从RT-thread名字上看，其表示的就是实时的线程。
首先对于MCU上的资源每个任务都是共享的，可以认为主流RTOS是单进程多线程模型，已完成内核抢占式任务的调度，临界段代码保护，消息队列，二值信号量，互斥信号量，计数信号量，mini_libc库，软件定时器和device抽象层，Heap内存管理，该miniRTOS可使用于资源紧张型的MCU，根据实际需求也可进行裁剪，选择性编译，移除不需要的组件。

# 2、RTOS内核
````
struct Task_node
{
			 u16 *task_id_point;
			 char *task_name;
	volatile u8 task_state;
	volatile u8 task_pend_state;
			 u16 task_priority; 
	volatile u32 task_delay_ms;
	volatile u32 task_tick_count;
			 u32 task_tick_reload;
			 unsigned int *task_stack;
			 u16 task_stack_size_words;
			 void *task_function;
	struct Task_node* timer_list_next;
    struct Task_node* priority_list_next;	
};
typedef struct Task_node Task_Unit;
````
一个任务的控制块所包含的信息有：
1，该任务的操作句柄(任务控制块在任务列表的序号变量)地址——task_id_point
2，该任务的名称——task_name
3，该任务的状态（block、readly）——task_state
4、该任务的Pend状态（pend、unpend）——task_pend_state
5，该任务的优先级——task_priority
6，该任务主动放弃CPU使用权进入delay的所需延时时间变量——task_delay_ms
7，该任务时间片任务调度执行任务的时间变量——task_tick_count
8，该任务时间片任务调度执行任务的systick数值——task_tick_reload
9，该任务的私有栈地址——task_stack
10，该任务的私有栈大小(单位：字，4byte)。——task_stack_size_words
11，该任务的执行函数入口地址——task_function
12,任务的timer链表next指针——timer_list_next
13、任务的优先级链表next指针——priority_list_next
````
#define	USER_TASK_NUM   4
#define	TASK_NUM   (USER_TASK_NUM+2)
Task_Unit Task_List[TASK_NUM]={0};
````
用户根据实际需要修改USER_TASK_NUM 变量值，编译器根据TASK_NUM定义一个数组，数组元素为各个任务控制块结构体 ，构成任务列表
RTOS的所有任务由任务创建函数一个一个创建，创建函数如下：
需要向该函数传递的参数有：存储任务ID的变量地址，任务的名称(字符串)，任务的初始化状态，任务的优先级、任务的初始延时值，任务的私有栈大小，任务函数入口地址
该函数根据创建任务的先后顺序将各个任务的控制块信息填入任务列表，其中任务栈的内存空间采用动态内存申请的方式申请MCU的堆区，根据需要可在启动文件中修改堆区大小。
````
//---------------------------------RTOS 任务创建函数-----------------------------------------//
/**
  * @brief  动态申请任务栈内存，并对任务列表中对应的任务结构体变量进行初始化。
  * @param  u16 *task_id:              任务id变量地址
			char *task_name:           任务名称，字符串
			u8 task_state:            任务状态
			u16 task_priority:         任务优先级
			u32 task_delay_ms:         任务延时值
			u16 task_stack_size_words: 任务栈大小(单位:32bit word)
			void *task_function:       任务函数地址
  * @retval 任务创建成功或失败标志                       
  */
u8 Task_Create( 
				u16 *task_id,
				char *task_name,
				u8 task_state,
				u16 task_priority,
				u32 task_delay_ms,
				u32 task_tick_ms,
				u16 task_stack_size_words,
				void *task_function	
			  )
{
	u32 i;
	static u16 task_id_index=0;
	
	configASSERT(task_id_index < TASK_NUM);
	
	Task_List[task_id_index].task_id_point = task_id;
	Task_List[task_id_index].task_name = task_name;
	Task_List[task_id_index].task_state = task_state;
	Task_List[task_id_index].task_pend_state = task_state;
	Task_List[task_id_index].task_priority = task_priority;
	Task_List[task_id_index].task_delay_ms = task_delay_ms;
	Task_List[task_id_index].task_tick_reload = task_tick_ms;
	Task_List[task_id_index].task_tick_count = task_tick_ms;
	Task_List[task_id_index].task_stack = (unsigned int *) mini_malloc((task_stack_size_words)*sizeof(unsigned int));
	Task_List[task_id_index].task_stack_size_words = task_stack_size_words;
	Task_List[task_id_index].task_function = task_function;
	if(Task_List[task_id_index].task_stack != NULL) 
	{
		for(i=0;i<Task_List[task_id_index].task_stack_size_words;i++)
		{
			*(Task_List[task_id_index].task_stack+i) = 0xdeadbeef;
		}
		task_id_index++;
		return Success;
	}
	else
	{
		mini_printf("Not enough memory for task stack!\r\n");
		return Fail;
	}
}
````
根据任务个数定义用于存放各个任务私有栈顶入口地址的数组
````
uint32_t PSP_array[TASK_NUM]; 

//--------------初始化任务ID和关联任务列表与系统栈---------------//
for(i=0;i<TASK_NUM;i++)
{

	*(Task_List[i].task_id_point) = i;
	PSP_array[i] = ((unsigned int) Task_List[i].task_stack)+(Task_List[i].task_stack_size_words * sizeof(unsigned int))-16*4;
	HW32_REG((PSP_array[i] + (14<<2))) = (unsigned long) Task_List[i].task_function;
	HW32_REG((PSP_array[i] + (15<<2))) = 0x01000000;
}
````
1，PSP_array[0] = ((unsigned int) task0_stack)+(sizeof task0_stack)-16*4;
task0的任务私有堆栈为数组task0_stack，分布如下：低地址在下，高地址在上，PSP_array[0]记录如下图的pxTopOfStack位置
2，HW32_REG((PSP_array[0] + (14<<2))) = (unsigned long) task0;
PSP_array[0] + (14<<2)指针指向R15（PC），对PC赋值task0任务函数地址入口
3，HW32_REG((PSP_array[0] + (15<<2))) = 0x01000000;
PSP_array[0] + (15<<2)指针指向xPSR。初始化xPSR为24bit为1（该位代表thumb状态，若为0会引起错误异常）

# 3、RTOS初始化：
用变量current_task和next_task指示当前任务和上一个任务的私有栈地址在PSP_array中的位置，实质是私有栈的切换变量
current_task=0,则使用task0的私有栈，
__set_PSP((PSP_array[current_task] + 16*4));//将MCU的PSP指向task0的栈顶位置
NVIC_SetPriority(PendSV_IRQn,0xFF);//设置PendSV中断的优先级为最低优先级（MCU中断编号规则是，值越大，优先级越低）
SysTick_Config(SysTick_Rhythm*(SystemCoreClock/1000000));
定义systick时间片的时间单元，单位us
SysTick_Config(SysTick_Rhythm*(SystemCoreClock/1000000));
SystemCoreClock为系统内核时钟频率，1/SystemCoreClock秒产生一次SysTick计数，SysTick_Rhythm*(SystemCoreClock/1000000)次产生一次中断，
即SysTick_Rhythm微秒产生一次中断，操作系统时间片为SysTick_Rhythm微秒。

__set_CONTROL(0x03);
control[0] control[1] 选择 1 1 模式
即：使用PSP（进程栈），非特权模式
Idle_task();
开始执行Idle_Task任务
````
//---------------初始化操作系统-------------------------------//
	current_task_id = 0;
	__set_PSP((PSP_array[current_task_id] + 16*4));
	NVIC_SetPriority(PendSV_IRQn,0xFF);
	NVIC_SetPriority(SVCall_IRQn,0);
	SysTick_Config(SysTick_Rhythm*(SystemCoreClock/1000000));
	Exit_Critical();
	__set_CONTROL(0x02);
	Idle_task();
````	
对任务的优先级链表和timer链表初始化
````
//-----------任务优先级链表和timer链表初始化函数------------------------//
void Task_list_init(void)
{
	u16 i;	
	Timer_list_head = &Task_List[0];
	Priority_list_head= &Task_List[0];

	for(i=0;i<TASK_NUM-1;i++)
	{
		Task_List[i].timer_list_next = &Task_List[i+1];
		Task_List[i].priority_list_next = &Task_List[i+1];
	}
	Task_List[i].timer_list_next = NULL;
    Task_List[i].priority_list_next = NULL;
	
	for(i=0;i<TASK_NUM;i++)
	{
        List_insert(priority_list,&Task_List[i]);
		List_insert(timer_list,&Task_List[i]);
	}
}
````
main函数中先完成板级硬件的初始化，后完成上述的任务创建和RTOS的初始化
````
//-------------------------板级支持包硬件外设初始化--------------------//

void BSP_Init(void)
{
	USART1_Config();
	HeapInit();
	//queue_init(&printf_buf,printf_buf_size);
    LED_Device_Register();	
}

//----------------------main主函数-----------------------------------//

int main(void)
{	
    BSP_Init();
	RTOS_Init();
	while(1)
	{
		stop_cpu;
	}
}

//------------RTOS 消息队列、软件定时器、任务创建初始化-----------------//

void Task_Creat_Init(void)
{
	Queue_test = Creat_queue();
	if(Queue_test==NULL) mini_printf("Creat Queue Fail\r\n");
	
	//*timer_id,timer_switch_flag,Timer_mode,timer_priority,user_tick_count,*timer_function
	Soft_Timer_Creat(&soft_timer0,stop_timer,repeat_mode,1,500,Timer0_callback);
	Soft_Timer_Creat(&soft_timer1,stop_timer,repeat_mode,2,1000,Timer1_callback);
	
	//*task_id,*task_name,task_state,task_priority,task_delay_ms,task_tick_ms,task_stack_size_words,*task_function
	Task_Create(&Task1,"task1",TASK_READY,1,0,1000, 40,task1);
	Task_Create(&Task2,"task2",TASK_READY,1,0,1000,100,task2);
}
````
# 4、任务延时：
RTOS_delay函数中根据tcurrent_task_id选择对当前任务进行状态设置和对应的延时变量赋值，并进入任务阻塞状态
1，将当前任务设置为Block状态，从优先级链表移除（不参与后续的任务优先级判决），释放CPU使用权。
2，对当前任务的延时变量赋值，赋值为当前滴答定时器的计数值+所需延时的值，并重新插入timer链表，参与延时排序。
3，任务进入阻塞态，等待滴答定时器中断判决延时时间到，修改该任务的状态为Ready态
4、每次时间片中断都会检测延时计数变量是否等于当前任务的延时变量值，若是则会将当前任务的状态设置为Ready状态，从而结束阻塞状态，进入优先级判决，等待运行
````
//-------------任务延时函数------------------------------------//
void RTOS_Delay(uint32_t delay_ms)
{
	Enter_Critical();
	Task_List[current_task_id].task_state = TASK_BLOCK;
	
	if(delay_ms != MAX_DELAY)  
	{
		Task_List[current_task_id].task_delay_ms = SysTick_count + delay_ms;
		List_insert(timer_list,&Task_List[current_task_id],order_insert);
	}
	else  
	{
		Task_List[current_task_id].task_pend_state = TASK_BLOCK;
	}
	
	List_remove_node(priority_list,&Task_List[current_task_id]);
	Launch_Task_Schedule();
	Exit_Critical();
}
````
# 5、任务调度：
0，在RTOS时间片中断函数中对延时计数变量++操作，代表一个心跳时钟计数
1，判断当前任务的时间片是否到期，如果到期且下一个ready任务优先级相同，将当前任务逆序插入链表切换下一个同优先级任务，实现时间片任务调度
2，对任务列表头部延时变量（最小）判断延时时间是否已到，若某任务延时时间到且该任务未被设置为挂起态，则将该任务解除阻塞，设置为ready态
3，判断优先级链表头部（优先级最大）任务是否ready态，出现Ready态的任务为当前ready态任务中优先级最高的任务。
4，对返回的最高优先级任务编号与当前正在运行的任务编号进行比较，若相同，则无需启动PensSV中断，systick中断退出，继续执行当前的任务
5，若判决出更高优先级的任务，设置PendSV为Ready 状态，待后续进入PendSV中断函数，进行任务的上下文切换。
````
//-------------RTOS心跳中断服务函数-----------------------//
void SysTick_Handler(void)
{
	Enter_Critical();
	SysTick_count++;
   
	if((SysTick_count > Task_List[current_task_id].task_tick_count) && \
	   (Task_List[current_task_id].task_state == TASK_READY) && \
	   (Task_List[current_task_id].priority_list_next->task_priority == Task_List[current_task_id].task_priority))
	{
		List_insert(priority_list,&Task_List[current_task_id],reverse_insert);
	}

	if(SysTick_count > Timer_list_head->task_delay_ms)
	{
		Timer_list_head->task_state = TASK_READY;
		List_insert(priority_list,Timer_list_head,order_insert);
		if(Timer_list_head->timer_list_next != NULL) Timer_list_head = Timer_list_head->timer_list_next;
	}
	
	if(SysTick_count > Softimer_list_head->timer_count)
	{
		Softimer_list_head->timer_state = run_timer;
		if(Softimer_list_head->timer_runflag == run_timer) Release_Task(timer_guard_task_id);
		if(Softimer_list_head->softimer_next != NULL) Softimer_list_head = Softimer_list_head->softimer_next;
	}
	
	Launch_Task_Schedule();
	Exit_Critical();
}

//--------------------抢占式任务调度函数------------------------//
//在调度器为pend时，发起抢占式任务调度，Priority_list_head指向当前优先级最高的ready任务，同时更新下一个即将运行的任务时间片
void Launch_Task_Schedule(void)
{
	if(scheduler_pend_flag == NO_Pend)
	{
		next_task_id = *(Priority_list_head->task_id_point);
		if(current_task_id != next_task_id)
		{
			Task_List[next_task_id].task_tick_count = SysTick_count + Task_List[next_task_id].task_tick_reload;
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		}
	}
}
````
任务的上下文切换：（分两个部分，当前任务的现场保护，下一个任务的现场恢复）
现场保护：
1，当前任务的R4-R11中的数据压入当前任务的私有栈
2，把当前任务的编号保存到R2中
3，把当前任务的私有栈PSP所指地址存取PSP_array数组中对应元素中。
其实质是：保存了当前任务的R4-R11中的数据压入当前任务的私有栈，和当前任务的私有栈PSP所指地址存取PSP_array数组中对应元素中

现场恢复：
1，将current_task = next_task
2,  将next_task任务的私有栈PSP所对应取PSP_array数组中对应元素加载到R0。
3，将R0（PSP_array所存储的指针地址）读取数据恢复到R4-R11
4，将R0（PSP_array所存储的指针地址）恢复到系统PSP
5，PC跳转到进入PendSV函数前的入口地址
其实质是：从任务的私有栈恢复数据到R4-R11，并将当前任务的堆栈地址赋值给系统PSP，将LR地址加载到PC实现函数返回
````
//--------------PendSV中断服务函数,任务上下文切换-----------------------//
__ASM void PendSV_Handler(void)//有硬件fault
{
	cpsid i
    MRS R0 , PSP //把PSP值读到R0
	STMDB R0!,{R4 - R11}//R4~R11中的数据依次存入PSP所指地址处，每存一次R0更新地址，记录PSP当前值
	LDR R1,=__cpp(&current_task_id);//把C语言中全局变量current_task_id的地址存入R1，汇编调用C变量需用cpp函数
	LDR R2,[R1]//将current_task_id数值存入R2中
	LDR R3, =__cpp(&PSP_array);//把C语言中全局变量PSP_array的地址存入R1，汇编调用C变量需用cpp函数
	STR R0,[R3,R2,LSL #2]//R0数据加载到地址=R3+R2<<2处(PSP当前地址存到PSP_array+current_task_id*2处)
	
	LDR R4,=__cpp(&next_task_id);//把C语言中全局变量next_task_id的地址存入R4，汇编调用C变量需用cpp函数
	LDR R4,[R4]//将next_task_id数值存入R4中
	STR R4,[R1]//将current_task_id = next_task_id
	LDR R0,[R3,R4,LSL #2]//将地址=R3+R2<<2处数据加载到R0，(PSP_array+next_task_id*2处数据加载到PSP处)
	LDMIA R0!,{R4 - R11}//PSP所指地址处读出数据加载到R4~R11中，每存一次R0更新地址，记录PSP当前值
	MSR PSP,R0  //R0所指地址加载到PSP
	cpsie i
	BX LR      //PC返回到LR所指处，该函数调用前地址
	ALIGN 4    //下一条指令或数据对齐到4字节地址处，空位补0
}
````
# 6、任务挂起与恢复：
通过同时修改任务的运行状态和挂起状态，可实现任务的挂起和解除操作，
1，设置任务的运行状态和挂起状态为阻塞态，则任务不在参与优先级判决，挂起的任务对于调度器而言，相当于不可见
2，解除任务的挂起态，同时让该任务的运行态为ready态，参与优先级的判决，等待CPU使用权
````
//------------------RTOS任务的挂起-----------------------------//
void Pend_Task(Task_Handle Task_x)
{
	taskENTER_CRITICAL();
	Task_List[Task_x].task_pend_state = TASK_BLOCK;
	Task_List[Task_x].task_state = TASK_BLOCK;
	List_remove_node(priority_list,&Task_List[Task_x]);
	Launch_Task_Schedule();
	taskEXIT_CRITICAL();
}

//---------------------RTOS任务恢复------------------------------//
void Release_Task(Task_Handle Task_x)
{

	taskENTER_CRITICAL();
	Task_List[Task_x].task_pend_state = TASK_READY;
	Task_List[Task_x].task_state = TASK_READY;
	List_insert(priority_list,&Task_List[Task_x]);
	Launch_Task_Schedule();
	taskEXIT_CRITICAL();
}
````
# 7、临界段保护：
进入临界段代码保护，置PRIMASK 1bit寄存器1，屏蔽除NMI，硬fault以外的所有异常。
退出临界段代码保护，清除PRIMASK 1bit寄存器0，中断正常响应
对应临界段嵌套，有全局变量递增与递减指示
````
//进入临界段代码保护
void Enter_Critical(void)
{
	if(Critical_count == 0)  __set_PRIMASK(0x01); //进入临界段代码保护，置PRIMASK 1bit寄存器1，屏蔽除NMI，硬fault以外的所有异常
	Critical_count++;
}

//退出临界段代码保护
void Exit_Critical(void)
{
	if(Critical_count == 1) __set_PRIMASK(0x00); //退出临界段代码保护，清除PRIMASK 1bit寄存器0，中断正常响应
	Critical_count--;
}
````
# 8、空闲线程：
空闲线程，在创建之初就已设置为优先级最低0，无延时，永远处于ready或run态，可在IDLE_Task函数中添加内存回收或CPU进给休眠，或系统运行参数监控等相关操作
空闲线程中执行WFI指令，CPU进入睡眠模式，在systick中断中唤醒
````
//---------------空闲任务----------------//
void Idle_task(void)
{

	while(1)
	{
		//printf("Idle_task\r\n");
		__WFI();
	}
}
````
# 9、用户线程：
任务的组成由while(1)中任务实体执行部分和主动释放CPU后延时部分组成，若任务无需延时，2000可改为0，但此任务的优先级过高会完全抢占比其低优先级任务的CPU使用权。
````
//---------------task3-----------------//
void task3(void)
{
	while(1)
	{
		printf("333333333333333333333333333333333\r\n");
		
		if(CMD =='4') 
		{
			CMD = 0;
			Set_Event_Bit(&test_event,1);
		}
		if(CMD =='5') 
		{
			CMD = 0;
			QueueSend(Queue_test,65,0);
		}
		
		RTOS_Delay(1000);
	}
}
````
# 10、中断服务函数：
在BSP初始化函数中对各种硬件中断进行初始化，优先级设置操作，在对应中断服务函数中编写中断所需执行代码，注意快进快出，任何中断的优先级都大于Task的优先级

# 11、消息队列：
消息队列采用单链表式，实现数据的先进先出FIFO功能，RTOS的消息队列发送与接收操作都具备延时阻塞机制，会调用上述入队与出队操作函数，程序参数mode判断当前是在应用在中断中还是在线程中，故消息队列API可以用于任务和中断服务函数。
````
//----------------------------------入队-------------------------------------//
/**
 * @brief  动态内存申请方式增加单项链表长度，实现入队操作
 * @param  
			queue_data：     指向一条队列的指针
			push_data：      需要压入该队列的数据
			byteNum：需要入队的数据所占字节数
 * @retval 入队成功或失败标志                      
 */
static char Queue_message_push(Queue_message *queue_data,void *push_data,unsigned int byteNum)
{
    unsigned char *temp = (unsigned char *)push_data;
	unsigned char *data_temp = (unsigned char *)mini_malloc(byteNum);
	Queue_point *queue_temp = (Queue_point *)mini_malloc(sizeof(Queue_point));

	if((temp == NULL)||(queue_temp == NULL)||(data_temp == NULL)) return Fail;
	else
	{
		if((queue_data->tail == NULL)&&(queue_data->head == NULL))  queue_data->head = queue_temp;
		else	queue_data->tail->next = queue_temp;

		queue_data->tail = queue_temp;
		queue_temp->next = NULL;
		queue_temp->data = data_temp;
		while(byteNum--)
		{
			*data_temp++ = *temp++; 
		}
		return Success;
	}
}

//----------------------------------出队-------------------------------------//
/**
 * @brief  动态内存释放方式减小单项链表长度，实现出队操作
 * @param  
			queue_data：     指向一条队列的指针
			*pull_data：     队列弹出数据存放变量地址
			byteNum：需要入队的数据所占字节数
 * @retval 出队成功或失败标志                      
 */
static char Queue_message_pull(Queue_message *queue_data,void *pull_data,unsigned int byteNum)
{
	unsigned char *data_temp;
	Queue_point *queue_temp = queue_data->head;
	unsigned char *temp = (unsigned char *)pull_data;
	if((queue_data->tail == NULL)&&(queue_data->head == NULL)) 	return Fail;
	else
	{
		data_temp = queue_data->head->data;
		while(byteNum--)
		{
			*temp++ = *data_temp++;
		}
		queue_data->head = queue_data->head->next;
		mini_free(queue_temp->data);
		mini_free(queue_temp);
		if(queue_data->head == NULL)  queue_data->tail = NULL;
		return Success;
	}
}

//-------------------------消息队列创建与初始化-------------------------------------//
Queue_Handle Creat_queue(void)
{
	Queue_message *queue_x = (Queue_message *)mini_malloc(sizeof(Queue_message));
	
	if(queue_x != NULL)
	{	
		queue_x->head = NULL;
		queue_x->tail = NULL;
		mini_memset(queue_x->task_block_list,0,TASK_NUM);
	}
	return  queue_x;
}

//--------------------------------发送消息队列--------------------------------------------//
char QueueSend(Queue_message *queue,void *message,unsigned int byteNum,u32 delay_ms,MCU_mode mode)
{
	u16 i;
	char flag = Fail;
	Task_Unit *task_temp = &Task_List[idle_task];
	
	Enter_Critical();
	flag = Queue_message_push(queue,message,byteNum);
	
	if(flag != Success)
	{
		if(mode != Handle_mode)
		{
			if(delay_ms == 0)  flag = Fail;
			else
			{
					queue->task_block_list[current_task_id] = Semaphore_Block;
					Exit_Critical();
					RTOS_Delay(delay_ms);
					Enter_Critical();
					queue->task_block_list[current_task_id] = Semaphore_Unblock;

					flag = Queue_message_push(queue,message,byteNum);
			}
		}
		else flag = Fail;
	}
	
	if(flag == Success)
	{
		for(i=0;i<TASK_NUM;i++)
		{
			if((queue->task_block_list[i]==Semaphore_Block)\
				&&(*(Task_List[i].task_id_point) != timer_guard_task_id)\
			    &&(task_temp->task_priority < Task_List[i].task_priority))  
				{
					task_temp = &Task_List[i];
				}
		}
		if(task_temp != &Task_List[idle_task])
		{
			queue->task_block_list[*(task_temp->task_id_point)] = Semaphore_Unblock;
			task_temp->task_state = TASK_READY;
			task_temp->task_pend_state = TASK_READY;
			List_insert(priority_list,task_temp,order_insert);
			Launch_Task_Schedule();
		}
	}
	
	Exit_Critical();
	return flag;
}

//--------------------------------接收消息队列--------------------------------------------//
char QueueReceive(Queue_message *queue,void *message,unsigned int byteNum,u32 delay_ms,MCU_mode mode)
{
	char flag = Fail;
	
	Enter_Critical();
	flag = Queue_message_pull(queue,message,byteNum);

	if(flag != Success)
	{
		if(mode != Handle_mode)
		{
			if(delay_ms == 0)  flag = Fail;
			else
			{
				queue->task_block_list[current_task_id] = Semaphore_Block;
				Exit_Critical();
				RTOS_Delay(delay_ms);
				Enter_Critical();
				queue->task_block_list[current_task_id] = Semaphore_Unblock;
				flag = Queue_message_pull(queue,message,byteNum);
			}
		}
		else
		{
			flag = Fail;
		}
	}
	Exit_Critical();
	return flag;
}
````

# 12、二值，互斥，计数信号量：
二值信号量主要实现任务间的同步，可以是两个任务间同步，也可以是一个任务与多个任务之间同步。当多个任务阻塞在同一个二值信号量时，信号量释放后，解除所有阻塞在该二值信号量上的任务。
互斥信号量主要实现两个任务间的互斥运行，用于共享资源保护，实现互斥访问，也可应用于一个任务与多个任务间的互斥，当多个任务阻塞在同一个互斥信号量时，信号量解除后，率先解除的是阻塞任务中优先级最高的任务，具备优先级翻转问题的解决方案。
计数信号量主要用于资源的管理统计，对某一共享资源的数量进行管理，当资源数为0时，任务可能会阻塞在该计数信号量上，当资源数大于0时，阻塞在该计数信号量上的任务中，优先级最高的任务率先解除阻塞，获得资源。
````
//-------------------------二值+互斥+计数信号量Take函数-------------------------------------//
/**
 * @brief  信号量获取函数，可根据需要设置信号量的种类(二值、互斥、计数)，并设置阻塞延时
 * @param  
			type：       操作信号量的种类
            *Semaphore： 信号量地址
            delay_ms：   阻塞延时值
 * @retval 信号量获取成功或失败标志                      
 */
char Semaphore_Take(Semaphore_Type type,Semaphore_Handle *Semaphore,u32 delay_ms,MCU_mode mode)
{
	char flag = Fail;
	
	Enter_Critical();

	if(Semaphore->Semaphore)  
	{
		if(type == Count_Semaphore)  Semaphore->Semaphore -= 1;
		else 						 Semaphore->Semaphore = 0;
		flag = Success;
		if(mode != Handle_mode)//非中断模式才会更新Semaphore->task_id
		{
			Semaphore->task_id = current_task_id;
			Semaphore->task_priority = Task_List[current_task_id].task_priority;
		}
	}
	else                  
	{
		if(mode != Handle_mode)
		{
			if(delay_ms == 0)  flag = Fail;
			else
			{
				//互斥信号量的优先级翻转问题解决方案
				//优先级提升
				if((Semaphore->task_id != 0)\
					&&(Semaphore->task_priority != 0)\
					&&(Task_List[current_task_id].task_priority > Semaphore->task_priority)
				    &&(type == Mutex_Semaphore))  
					{
						Task_List[Semaphore->task_id].task_priority = Task_List[current_task_id].task_priority;
					}

				Semaphore->task_block_list[current_task_id] = Semaphore_Block;
				Exit_Critical();
				RTOS_Delay(delay_ms);
				Enter_Critical();
				Semaphore->task_block_list[current_task_id] = Semaphore_Unblock;
				//优先级还原
				if((Semaphore->task_id != 0)\
					&&(Semaphore->task_priority != 0)\
					&&(Task_List[current_task_id].task_priority > Semaphore->task_priority)
				    &&(type == Mutex_Semaphore))				
				{
					Task_List[Semaphore->task_id].task_priority = Semaphore->task_priority;
				}
					
				if(Semaphore->Semaphore) 
				{
					if(type == Count_Semaphore)  Semaphore->Semaphore -= 1;
					else 						 Semaphore->Semaphore = 0;
					flag = Success;
					Semaphore->task_id = current_task_id;
					Semaphore->task_priority = Task_List[current_task_id].task_priority;
				}
				else  flag = Fail;
			}
		}
		else                  
		{
			flag = Fail;
		}
	}
	Exit_Critical();
	return flag;
}


//-------------------------二值+互斥+计数信号量Give函数-------------------------------------//
/**
 * @brief  信号量释放函数，无阻赛
 * @param  
			type：       操作信号量的种类
            *Semaphore： 信号量地址
 * @retval  NO                      
 */
void Semaphore_Give(Semaphore_Type type,Semaphore_Handle *Semaphore)
{
	u16 i;
	Task_Unit *task_temp = &Task_List[idle_task];
	Enter_Critical();
	
	if(type == Count_Semaphore)  Semaphore->Semaphore += 1;
	else 						 Semaphore->Semaphore = 1;

	if(type ==Binary_Semaphore)
	{
		for(i=0;i<TASK_NUM;i++)
		{
			if((Semaphore->task_block_list[i]==Semaphore_Block)&&(*(Task_List[i].task_id_point) != timer_guard_task_id))  
			{
				Semaphore->task_block_list[i] = Semaphore_Unblock;
				Task_List[i].task_state = TASK_READY;
				Task_List[i].task_pend_state = TASK_READY;
				List_insert(priority_list,&Task_List[i],order_insert);
			}
		}
	}
	else
	{
		for(i=0;i<TASK_NUM;i++)
		{
			if((Semaphore->task_block_list[i]==Semaphore_Block)\
				&&(*(Task_List[i].task_id_point) != timer_guard_task_id)\
			    &&(task_temp->task_priority < Task_List[i].task_priority))  
				{
					task_temp = &Task_List[i];
				}
		}
		if(task_temp != &Task_List[idle_task])
		{
			Semaphore->task_block_list[*(task_temp->task_id_point)] = Semaphore_Unblock;
			task_temp->task_state = TASK_READY;
			task_temp->task_pend_state = TASK_READY;
			List_insert(priority_list,task_temp,order_insert);
		}
	}				
	Launch_Task_Schedule();
	Exit_Critical();
}
````
# 13、事件标志组：
事件标志组可用于一个事件与多个事件，或多个事件与多个事件之间的同步。事件标志组定义为32bit，一个标志组最多支持32个事件触发关系，任务可设置其需要关注事件标志组的bit(最大32bit)，也可设置所关注的bit之间是“与”关系触发，还是“或”关系触发，等待到事件发生后，也可设置事件发生后，保持事件标志bit，还是清除事件标志bit，事件未发生时，读取事件标志组的任务可进入阻塞延时。
````
//-------------------------------------------------------------------------//
/**
 * @brief  任务等待事件标志组，可设置阻塞时间或非阻塞。
 * @param  
			event：      事件标志组变量地址
			care_bit：   任务需要关注事件标志组的bit(最大32bit)
			relate_flag：关心的bit之间是“与”关系，还是“或”关系
			action_flag：等待到事件发生后，保持事件标志，还是清除事件标志
			delay_ms：   任务读取事件标志组，事件未发生，任务需要阻塞的延时时间，若为0，不等待，若为MAX_DELAY，一直等待。
 * @retval 等待该事件标志组成功或失败标志                       
 */
char xEventGroupWaitBits(Event_Handle * event, u32 care_bit,Relate_Type relate_flag,Action_Type action_flag,u32 delay_ms,MCU_mode mode) 
{
	char flag = Fail;
	
	Enter_Critical();
	flag = (char) ((relate_flag == and_type) ? ((event->Event & care_bit) == care_bit):((event->Event & care_bit) != 0));
	
	if(flag != Success) 
	{
		if(mode != Handle_mode)
		{
			if(delay_ms == 0)  flag = Fail;
			else
			{
				event->task_care_bit_list[current_task_id] = care_bit;
				event->task_relate_type_list[current_task_id] = relate_flag;
				
				event->task_block_list[current_task_id] = Semaphore_Block;
				Exit_Critical();
				RTOS_Delay(delay_ms);
				Enter_Critical();
				event->task_block_list[current_task_id] = Semaphore_Unblock;

				flag = (char) ((relate_flag == and_type) ? ((event->Event & care_bit) == care_bit):((event->Event & care_bit) != 0));
			}
		}
		else flag = Fail;
	}
	
	if((action_flag == release_type) && (flag == Success)) event->Event &= ~care_bit;
	Exit_Critical();
	return flag;
}

//-----------------------------置位事件标志组对应bit----------------------------------------//
void Set_Event_Bit(Event_Handle * event,u8 bit)
{
	u16 i;
	u32 care_bit;
	Relate_Type relate_flag;
	char flag = Fail;

	Enter_Critical();
	event->Event |= (1<<bit);

	for(i=0;i<TASK_NUM;i++)
	{
		if((event->task_block_list[i]==Semaphore_Block)&&(*(Task_List[i].task_id_point) != timer_guard_task_id))  
		{
			care_bit = event->task_care_bit_list[i];
			relate_flag = (Relate_Type) event->task_relate_type_list[i];
			
			flag = (char)((relate_flag == and_type) ? ((event->Event & care_bit) == care_bit):((event->Event & care_bit) != 0));
			
			if(flag == Success)
			{
				event->task_block_list[i] = Semaphore_Unblock;
				Task_List[i].task_state = TASK_READY;
				Task_List[i].task_pend_state = TASK_READY;
				List_insert(priority_list,&Task_List[i],order_insert);
			}
		}
	}
	Launch_Task_Schedule();
	Exit_Critical();
}

//-----------------------------清除事件标志组对应bit----------------------------------------//
void Reset_Event_Bit(Event_Handle * event,u8 bit)
{
	u16 i;
	u32 care_bit;
	Relate_Type relate_flag;
	char flag = Fail;
	
	Enter_Critical();
	event->Event &= ~(1<<bit); 

	for(i=0;i<TASK_NUM;i++)
	{
		if((event->task_block_list[i]==Semaphore_Block)&&(*(Task_List[i].task_id_point) != timer_guard_task_id))  
		{
			care_bit = event->task_care_bit_list[i];
			relate_flag = (Relate_Type) event->task_relate_type_list[i];
			
			flag = (char)((relate_flag == and_type) ? ((event->Event & care_bit) == care_bit):((event->Event & care_bit) != 0));
			
			if(flag == Success)
			{
				event->task_block_list[i] = Semaphore_Unblock;
				Task_List[i].task_state = TASK_READY;
				Task_List[i].task_pend_state = TASK_READY;
				List_insert(priority_list,&Task_List[i],order_insert);
			}
		}
	}
	Launch_Task_Schedule();
	Exit_Critical();
}
````
# 14、软件定时器：
软件定时器数量可在内存允许条件下根据需要定义，突破硬件定时器的数量限制。在所有RTOS任务中，软件定时器守护任务的优先级最高，RTOS心跳中断中判断到有软件定时器ready定时时间到会触发该任务，
在该任务中进行调用软件定时器对应的回调函数
````
//--------------------------------软件定时器列表定义-----------------------------------//
Softimer_Unit Softimer_List[Softimer_NUM]={0};
Softimer_Unit *Softimer_list_head;
Task_Handle timer_guard_task_id;
Soft_Timer_Handle idle_softimer;


//--------------------------------软件定时器守护任务-----------------------------------//
/**
 * @brief   在所有RTOS任务中，软件定时器守护任务的优先级最高，RTOS心跳中断中判断到有软件定时器ready会触发该任务，
            在该任务中进行调用软件定时器对应的回调函数
 * @param   NO
 * @retval  NO                      
 */
void soft_timer_guard_task(void)
{
	u16 i;
	while(1)
	{
		for(i=0;i<Softimer_NUM;i++)
		{
			if((Softimer_List[i].timer_state == run_timer)&&(Softimer_List[i].timer_runflag == run_timer))
			{
				(*Softimer_List[i].callback_function)();
				Softimer_List[i].timer_state = stop_timer;
				
				if(Softimer_List[i].timer_mode == repeat_mode)
				{
					Softimer_List[i].timer_count = SysTick_count + Softimer_List[i].timer_reload;
					Softimer_list_insert(&Softimer_List[i]);
				}
				else
				{
					Softimer_List[i].timer_runflag = stop_timer;
				}
			}
		}
		Pend_Task(timer_guard_task_id);
	}
}

//-------------------------------启动对应软件定时器-----------------------------------//
void Start_Soft_Timer(Soft_Timer_Handle timer_id)
{
	Enter_Critical();
	Softimer_List[timer_id].timer_count = SysTick_count + Softimer_List[timer_id].timer_reload;
	Softimer_list_insert(&Softimer_List[timer_id]);
	Softimer_List[timer_id].timer_state = stop_timer;
	Softimer_List[timer_id].timer_runflag = run_timer;
	Exit_Critical();
}

//-------------------------------停止对应软件定时器------------------------------------//
void Stop_Soft_Timer(Soft_Timer_Handle timer_id)
{
	Enter_Critical();
	Softimer_list_remove_node(&Softimer_List[timer_id]);
	Softimer_List[timer_id].timer_state = stop_timer;
	Softimer_List[timer_id].timer_runflag = stop_timer;
	Exit_Critical();
}

//---------------------设置对应软件定时器的定时值(RTOS心跳节拍数)------------------------//
void Set_Soft_Timer(Soft_Timer_Handle timer_id,u32 tick_count)
{
	Enter_Critical();
	Softimer_List[timer_id].timer_reload = tick_count;
	Softimer_List[timer_id].timer_count = SysTick_count + tick_count;
    Softimer_list_insert(&Softimer_List[timer_id]);
	Softimer_List[timer_id].timer_state = stop_timer;
	Softimer_List[timer_id].timer_runflag = run_timer;
	Exit_Critical();
}

//-------------------------------软件定时器的初始化------------------------------------//
/**
 * @brief   在所有RTOS任务中，软件定时器守护任务的优先级最高，RTOS心跳中断中判断到有软件定时器ready会触发该任务，
            在该任务中进行调用软件定时器对应的回调函数
 * @param   
			*timer_id：         软件定时器的id变量地址
			timer_switch_flag： 开启或关闭软件定时器
			user_tick_count：   用户指定软件定时器的定时值(RTOS心跳节拍数)
			timer_function      软件定时器对应回调函数
 * @retval  NO                      
 */
void Soft_Timer_Creat(Soft_Timer_Handle *timer_id,Timer_Switch timer_state,Timer_mode mode,u16 priority,u32 timer_reload_value,void (*timer_function)(void))
{
	static u16 timer_id_init=0;
	configASSERT(timer_id_init < Softimer_NUM);
	
	Softimer_List[timer_id_init].softimer_id = timer_id;
	Softimer_List[timer_id_init].timer_state = timer_state;
	Softimer_List[timer_id_init].timer_mode = mode;
	Softimer_List[timer_id_init].timer_priority = priority;
	Softimer_List[timer_id_init].timer_reload = timer_reload_value;
	Softimer_List[timer_id_init].timer_runflag = timer_state;
	Softimer_List[timer_id_init].timer_count = timer_reload_value;
	Softimer_List[timer_id_init].callback_function = timer_function;
	timer_id_init++;
}

````
# 15、设备驱动抽象层：
device驱动抽象层,由以下函数组成:

                                device_register
                                device_unregister
                                device_find
                                device_init
                                device_open
                                device_close
                                device_read
                                device_write
                                device_control
                                device_set_rx_indicate
                                device_set_tx_complete

使用方法：
        在对应的外设驱动层中定义设备的以下函数，并初始化设备结构体成员，init，open，close，read，write，control
        调用device_register将设备插入到设备链表。
        调用device_find通过设备名称字符串，在设备链表中查找设备结构体指针。
        调用device_init，device_open，device_read，device_write，device_control，device_close完成设备的操作。
        调用device_set_rx_indicate，device_set_tx_complete注册设备读或写完成回调函数
        可在其回调函数中发送信号量或通知，告知数据处理任务进行数据的处理。
````    
Device_Handle Device_list_head=NULL;

//----------------------------------------------------------------------//
Device_Handle device_find(char *name)
{
	Device_Handle temp=NULL,result=NULL;
	configASSERT(name != NULL);
	
	for(temp=Device_list_head;temp != NULL;temp=temp->next)
	{
		if(0 == strcmp(temp->name,name))
		{
			result = temp;
			break;
		}
	}
	return result;
}

//----------------------------------------------------------------------//
return_flag device_register(Device_Handle device)
{
	return_flag flag = Fail;
	Device_Handle temp=NULL;
	configASSERT(device != NULL);
	
	if(NULL == device_find(device->name)) 
	{
		if(Device_list_head == NULL)
		{
			Device_list_head = device;
			device->next = NULL;
		}
		else
		{
			for(temp=Device_list_head;temp->next != NULL;temp=temp->next)
			{};
			temp->next = device;
			device->next = NULL;
		}
		device->open_flag = close_state;
		device->init_flag = No_init;
		flag = Success;
	}
    return flag;
}

//----------------------------------------------------------------------//
return_flag device_unregister(Device_Handle device)
{
	return_flag flag = Fail;
	Device_Handle temp=NULL;
	configASSERT(device != NULL);
	
	if(Device_list_head != NULL)
	{
		if(0 == strcmp(Device_list_head->name,device->name))
		{
			Device_list_head = Device_list_head->next;
			flag = Success;
		}
		else
		{
			for(temp=Device_list_head;temp->next != NULL;temp=temp->next)
			{
				if(0 == strcmp(temp->next->name,device->name))
				{
					temp->next = temp->next->next;
					flag = Success;
					break;
				}
			}
		}
	}
	return flag;
}

//----------------------------------------------------------------------//
return_flag device_init(Device_Handle device)
{
	return_flag flag = Fail;
	configASSERT(device != NULL);
	if((device->init_flag == No_init)&&(device->init != NULL)) 
	{
		(*device->init)();
		device->init_flag = Is_init;
		flag = Success;
	}
	return flag;
}

//----------------------------------------------------------------------//
return_flag device_open(Device_Handle device)
{
	return_flag flag = Fail;
	configASSERT(device != NULL);
	if(device->open_flag == close_state)
	{
		if(device->init_flag == No_init) 
		{
		    device_init(device);
		}
		if(device->open != NULL) (*device->open)();
		device->open_flag = open_state;
		flag = Success;
	}
	return flag;
}
 
//----------------------------------------------------------------------//
return_flag device_close(Device_Handle device)
{
	return_flag flag = Fail;
	configASSERT(device != NULL);
	if((device->open_flag == open_state)&&(device->close != NULL))
	{
		(*device->close)();
		device->open_flag = close_state;
		flag = Success;
	}
	return flag;
}

//----------------------------------------------------------------------//
u32 device_read(Device_Handle device,u32 pos,void *buffer,u32 size)
{
	u32 temp=0;
	configASSERT((device != NULL)&&(buffer != NULL));
    if((device->open_flag == open_state)&&(device->read != NULL))
	{
		temp = device->read(pos,buffer,size);
	}
	return temp;	 
}

//----------------------------------------------------------------------//
u32 device_write(Device_Handle device,u32 pos,const void *buffer,u32 size)
{
	u32 temp=0;
	configASSERT((device != NULL)&&(buffer != NULL));
    if((device->open_flag == open_state)&&(device->write != NULL))
	{
		temp = device->write(pos,buffer,size);
	}
	return temp;
}

//----------------------------------------------------------------------//
return_flag device_control(Device_Handle device,u32 cmd, void *arg)
{
	return_flag flag = Fail;
	configASSERT(device != NULL);
	if((device->open_flag == open_state)&&(device->control != NULL))
	{
		device->control(cmd,arg);
		flag = Success;
	}
	return flag;
}

//----------------------------------------------------------------------//
void device_set_rx_indicate(Device_Handle device,void(*rx_ind)(void *buffer,u32 size))
{
	configASSERT((device != NULL)&&(rx_ind != NULL));
	device->rx_complete_callback = rx_ind;
}

//----------------------------------------------------------------------//
void device_set_tx_complete(Device_Handle device,void(*tx_done)(void *buffer,u32 size))
{
	configASSERT((device != NULL)&&(tx_done != NULL));
	device->rx_complete_callback = tx_done;
}

//----------------------------------------------------------------------//
void printf_device_List(void)
{
	Device_Handle temp=NULL;
	
	printf("Device_list_head name =%s,init_flag=%d,open_flag=%d\r\n",Device_list_head->name,Device_list_head->init_flag,Device_list_head->open_flag);
	for(temp = Device_list_head; temp != NULL ;temp = temp->next)
	{
		printf("name =%s,init_flag=%d,open_flag=%d\r\n",temp->name,temp->init_flag,temp->open_flag);
	}
	printf("\r\n");
}
````
# 16、链表操作：
优先级链表和timer链表，软件定时器链表的操作包括：链表的初始化，链表节点的移除，节点插入，链表的打印（用于debug）
````
//-------------------------任务优先级链表和timer链表打印函数--------------------------------//
void printf_List(List_type type)
{
	Task_Unit *temp;

	switch(type)
	{
		case timer_list:
		{
			mini_printf("time_head name =%s,delay=%d\r\n",Task_List[*(Timer_list_head->task_id_point)].task_name,Timer_list_head->task_delay_ms);
			for(temp = Timer_list_head; temp != NULL ;temp = temp->timer_list_next)
			{
				mini_printf("id = %s,time = %d\r\n",temp->task_name,temp->task_delay_ms);
			}
		};break;
		
		case priority_list:
		{
			mini_printf("Priority_list_head name =%s,Priority=%d\r\n",Priority_list_head->task_name,Priority_list_head->task_priority);
			for(temp = Priority_list_head; temp != NULL ;temp = temp->priority_list_next)
			{
				mini_printf("name =%s,Priority=%d,delay=%d,tick=%d,\r\n",temp->task_name,temp->task_priority,temp->task_delay_ms,temp->task_tick_count);
			}
		};break;
		
		default:break;
	}
	mini_printf("\r\n");
}

//---------------------------任务优先级链表和timer链表移除节点函数-------------------------------//
void List_remove_node(List_type type,Task_Unit *node)
{
	Task_Unit *temp;
	
	switch(type)
	{
		case timer_list:
		{
			if(node->task_id_point == Timer_list_head->task_id_point) 
			{
				Timer_list_head = Timer_list_head->timer_list_next;
			}
			else
			{
				for(temp = Timer_list_head; temp->timer_list_next != NULL ;temp = temp->timer_list_next)
				{
					if(temp->timer_list_next->task_id_point == node->task_id_point)  
					{
						temp->timer_list_next = temp->timer_list_next->timer_list_next;
						break;
					}
				}
			}
		};break;
		
		case priority_list:
		{
			if(node->task_id_point == Priority_list_head->task_id_point) 
			{
				Priority_list_head = Priority_list_head->priority_list_next;
			}
			else
			{
				for(temp = Priority_list_head; temp->priority_list_next != NULL ;temp = temp->priority_list_next)
				{
					if(temp->priority_list_next->task_id_point == node->task_id_point)  
					{
						temp->priority_list_next = temp->priority_list_next->priority_list_next;
						break;
					}
				}
			}
		};break;
		
	    default:break;
	}
}

//------------------------任务优先级链表和timer链表插入节点函数-------------------------//
void List_insert(List_type type,Task_Unit *node,List_insert_type insert_type)
{
	Task_Unit *temp;
	
	List_remove_node(type,node);
	switch(type)
	{
		case timer_list:
		{
			if(node->task_delay_ms < Timer_list_head->task_delay_ms)  //首节点插入 
			{
				node->timer_list_next =  Timer_list_head;
				Timer_list_head = node;
			}
			else
			{
				for(temp = Timer_list_head; temp->timer_list_next != NULL ;temp = temp->timer_list_next)
				{
					if((node->task_delay_ms>=temp->task_delay_ms)&&(node->task_delay_ms<=temp->timer_list_next->task_delay_ms)) 
					{
						node->timer_list_next = temp->timer_list_next;
						temp->timer_list_next = node;
						break;
					}
				}
				if(temp->timer_list_next == NULL) //尾结点插入 
				{
					temp->timer_list_next = node;
					node->timer_list_next = NULL;
				}
			}
		};break;
		
		case priority_list:
		{
			if((node->task_state != TASK_BLOCK)&&(node->task_pend_state != TASK_BLOCK)) 
			{
				if(node->task_priority > Priority_list_head->task_priority)  //首节点插入 
				{
					node->priority_list_next =  Priority_list_head;
					Priority_list_head = node;
				}
				else
				{

					for(temp = Priority_list_head; temp->priority_list_next != NULL ;temp = temp->priority_list_next)
					{
						if(insert_type == reverse_insert)//同优先级逆序插入
						{
							if((node->task_priority <= temp->task_priority)&&(node->task_priority > temp->priority_list_next->task_priority)) 
							{
								node->priority_list_next = temp->priority_list_next;
								temp->priority_list_next = node;
								break;
							}
						}
						else//同优先级顺序插入
						{
							if((node->task_priority <= temp->task_priority)&&(node->task_priority >= temp->priority_list_next->task_priority)) 
							{
								node->priority_list_next = temp->priority_list_next;
								temp->priority_list_next = node;
								break;
							}
						}
					}
					if(temp->priority_list_next == NULL) //尾结点插入 
					{
						temp->priority_list_next = node;
						node->priority_list_next = NULL;
					}
				}
			}
		};break;
		default:break;
	}
}

//---------------------------任务优先级链表和timer链表初始化函数------------------------------------//
void Task_list_init(void)
{
	u16 i;	
	
	Timer_list_head = &Task_List[0];
	Priority_list_head= &Task_List[0];

	for(i=0;i<TASK_NUM-1;i++)
	{
		Task_List[i].timer_list_next = &Task_List[i+1];
		Task_List[i].priority_list_next = &Task_List[i+1];
	}
	Task_List[i].timer_list_next = NULL;
    Task_List[i].priority_list_next = NULL;
	
	for(i=0;i<TASK_NUM;i++)
	{
        List_insert(priority_list,&Task_List[i],order_insert);
		List_insert(timer_list,&Task_List[i],order_insert);
	}
}

//----------------------------------软件定时器链表打印函数----------------------------------//
void printf_Softimer_List(void)
{
	Softimer_Unit *temp;

	mini_printf("Softimer_list_head id =%d,timer_count=%d\r\n",*(Softimer_list_head->softimer_id),Softimer_list_head->timer_count);
	for(temp = Softimer_list_head; temp != NULL ;temp = temp->softimer_next)
	{
		mini_printf("id =%d,count=%d,\r\n",*(temp->softimer_id),temp->timer_count);
	}
	mini_printf("\r\n");
}

//-------------------------------软件定时器列表打印函数------------------------------------//
void printf_Softimer_array(void)
{
    u16 i;
	
	for(i=0;i<Softimer_NUM;i++)
	{
		mini_printf("array id =%d,priority=%d,\r\n",*Softimer_List[i].softimer_id,Softimer_List[i].timer_priority);
	}
}

//-----------------------------软件定时器链表移除节点函数-----------------------------------//
void Softimer_list_remove_node(Softimer_Unit *node)
{
	Softimer_Unit *temp;
	
	if(node->softimer_id == Softimer_list_head->softimer_id) 
	{
		Softimer_list_head = Softimer_list_head->softimer_next;
	}
	else
	{
		for(temp = Softimer_list_head; temp->softimer_next != NULL ;temp = temp->softimer_next)
		{
			if(temp->softimer_next->softimer_id == node->softimer_id)  
			{
				temp->softimer_next = temp->softimer_next->softimer_next;
				break;
			}
		}
	}
}

//---------------------------软件定时器插入节点函数-----------------------------------------//
void Softimer_list_insert(Softimer_Unit *node)
{
	Softimer_Unit *temp;
	
	Softimer_list_remove_node(node);

	if(node->timer_count < Softimer_list_head->timer_count)  //首节点插入 
	{
		node->softimer_next =  Softimer_list_head;
		Softimer_list_head = node;
	}
	else
	{
		for(temp = Softimer_list_head; temp->softimer_next != NULL ;temp = temp->softimer_next)
		{
			if((node->timer_count >= temp->timer_count)&&(node->timer_count <= temp->softimer_next->timer_count)) 
			{
				node->softimer_next = temp->softimer_next;
				temp->softimer_next = node;
				break;
			}
		}
		if(temp->softimer_next == NULL) //尾结点插入 
		{
			temp->softimer_next = node;
			node->softimer_next = NULL;
		}
	}
}

//----------------------------软件定时器链表初始化函数---------------------------------------//
void Softimer_List_init(void)
{
	u16 i,j;	
	Softimer_Unit Softimer_temp;
	for(i=0;i<Softimer_NUM;i++)
	{
		for(j=i+1;j<Softimer_NUM;j++)
		{
			if(Softimer_List[i].timer_priority < Softimer_List[j].timer_priority) 
			{
				Softimer_temp = Softimer_List[j];
				Softimer_List[j] = Softimer_List[i];
				Softimer_List[i] = Softimer_temp;
			}
		}
	}

	for(i=0;i<Softimer_NUM;i++)
	{
		*(Softimer_List[i].softimer_id) = i;
	}

	Softimer_list_head = &Softimer_List[0];
	for(i=0;i<Softimer_NUM-1;i++)
	{
		Softimer_List[i].softimer_next = &Softimer_List[i+1];
	}
	Softimer_List[i].softimer_next = NULL;

	for(i=0;i<Softimer_NUM;i++)
	{
		Softimer_list_insert(&Softimer_List[i]);
	}
}
````

# 17、mini_libc库：
mini_libc,包含基本的string和memory相关操作函数和snprintf和printf函数的实现，其中snprintf采用了ringbuffer数据结构，调用snprintf处进行入队，空闲任务中进行出队。
````    
//--------------------------------------------------------------//
//dest所指空间应该为RAM区(data或bss段)，dest不可指向string或const修饰区(存放于rodata区)
void *mini_memset(void *dest, int data, unsigned int length)
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
void *mini_memcpy(void *dest, const void *src, unsigned int length)
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
void *mini_memmove(void *dest, const void *src, unsigned int length)
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
int mini_memcmp(const void *cmp1 ,const void *cmp2, unsigned int length)
{
    const unsigned char *cmp1_temp = (unsigned char *)cmp1;
	const unsigned char *cmp2_temp = (unsigned char *)cmp2;
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
const void *mini_memchr(const void *dest ,int chr, unsigned int length)
{
    const unsigned char *dest_temp = (unsigned char *)dest;
	if((dest == NULL)||(length == 0)) return NULL;
	
    while(length--)
    {
    	if ((unsigned char)chr == *dest_temp)  return dest_temp;
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
char *mini_strncpy(char *dest, const char *src, unsigned int length)
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
int mini_strncmp(const char *cmp1, const char *cmp2, unsigned int length)
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
unsigned int mini_strnlen(const char *str, unsigned int maxlen)
{
    const char *str_temp;
 
    for (str_temp = str; *str_temp != '\0' && str_temp - str < maxlen; str_temp++);
 
    return str_temp - str;
}
 
//--------------------------------------------------------------//
unsigned int mini_strlen(const char *str)
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
char *mini_strncat(char *dest, const char *src, unsigned int length)
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
void queue_init(queue *buf,unsigned int length)
{
	unsigned int i;
	for(i=0;i<length;i++)
		buf->queue_data[i] = 0;
    buf->queue_head=0;
    buf->queue_tail=0;
	buf->queue_index=0;
	buf->queue_length = length;
}
 
//入队
char push_queue(queue *data,const char *push_data,unsigned int length)
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
char pull_queue(queue *data,char *pull_data,unsigned int length)
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
void mini_sendchr(char ch)
{  
	UART_putch(ch);		
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
char data_format_switch(int value, char *str, int format)
{
	char data;
	int temp;
	char *str_head  = str; 
	char *str_tail  = str; 
	
	if ((NULL == str) || (format < 2))  return 1;
	
	if ((0 > value) && (10 == format))
	{
		*str_tail++ = '-';
		str_head++;
		value = -value;
	}
 
	do
	{
		temp = (unsigned int)(value % format);
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
		
				if(fd > 0) xd = (unsigned int)((fd - xd)*10000000);
				else       xd = (unsigned int)((xd - fd)*10000000);
				
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
		
				if(fd > 0) xd = (unsigned int)((fd - xd)*10000000);
				else       xd = (unsigned int)((xd - fd)*10000000);
				
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
````    

 # 18、heap内存管理算法： 

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

````    
#define HEAP_SIZE  4096
#define ADDR_CRC(addr) (0x7FFFFFFF & (~((unsigned int)(addr))))
 
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
 
unsigned char HEAP[HEAP_SIZE]={0};
 
MemoryInforBlockNode *DoubleListHead = NULL;
MemoryInforBlockNode *SoleListHead = NULL;
//----------------------------------------------------------------------------------//
void HeapInit(void)
{
	MemoryInforBlockNode *temp = (MemoryInforBlockNode *) (((unsigned int)(&HEAP[7])) & (~0x00000007));//对Heap起始地址进行4字节地址对齐 
	mini_memset(HEAP,0,HEAP_SIZE);
	temp->MemoryBlockSize       = HEAP_SIZE - (unsigned int)temp + (unsigned int)HEAP - sizeof(MemoryInforBlockNode); 
	temp->MemoryBlockInfor.crc  = ADDR_CRC(temp);
    temp->MemoryBlockInfor.flag = FreeBlock;
	temp->Isolate_Zone          = 0xFFFFFFFF;//全FF隔离区
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
static MemoryInforBlockNode *search_best_freeblock(unsigned int size)
{
	MemoryInforBlockNode *temp = NULL;
 
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
	
	if(freeblock2->Isolate_Zone != 0xFFFFFFFF)   result = -1;//发生内存踩踏事故或
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
void *mini_malloc(unsigned int want_size) 
{
	void *result = NULL;
	unsigned int size;
	MemoryInforBlockNode *new_block;
	MemoryInforBlockNode *temp;
	
	Pend_Schedule();
	size = ((want_size/8) + ((want_size % 8) ? 1 : 0)) * 8;//对齐处理 
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
		new_block->Isolate_Zone = 0xFFFFFFFF;
		
		new_block->DoubleListPrevious = temp;
		new_block->DoubleListNext = temp->DoubleListNext;
		if(temp->DoubleListNext != NULL) temp->DoubleListNext->DoubleListPrevious = new_block;
		temp->DoubleListNext = new_block;
		
		sole_list_insert(new_block);
	}
	Release_Schedule();
	return result;
}
````    