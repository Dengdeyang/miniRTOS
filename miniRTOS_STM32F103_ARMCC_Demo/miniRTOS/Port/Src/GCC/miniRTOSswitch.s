.syntax unified
.thumb
.text

//---------------------------------PendSV中断服务函数,任务上下文切换------------------------------------//	
.global PendSV_Handler
.type PendSV_Handler, %function
PendSV_Handler:
	MRS R2,PRIMASK 
	CPSID I       //关中断
	MRS R0 , PSP  //把PSP值读到R0
	STMDB R0!,{R4 - R11} //R4~R11中的数据依次存入PSP所指地址处，每存一次R0更新地址，记录PSP当前值
	LDR R1,=current_task_id //把C语言中全局变量current_task_id的地址存入R1，汇编调用C变量需用cpp函数
	LDR R4,[R1] //将current_task_id数值存入R4中
	LDR R3, =PSP_array //把C语言中全局变量PSP_array的地址存入R1，汇编调用C变量需用cpp函数
	STR R0,[R3,R4,LSL #2] //R0数据加载到地址=R3+R2<<2处(PSP当前地址存到PSP_array+current_task_id*2处)
	
	LDR R4,=next_task_id //把C语言中全局变量next_task_id的地址存入R4，汇编调用C变量需用cpp函数
	LDR R4,[R4] //将next_task_id数值存入R4中
	STR R4,[R1] //将current_task_id = next_task_id
	LDR R0,[R3,R4,LSL #2] //将地址=R3+R2<<2处数据加载到R0，(PSP_array+next_task_id*2处数据加载到PSP处)
	LDMIA R0!,{R4 - R11} //PSP所指地址处读出数据加载到R4~R11中，每存一次R0更新地址，记录PSP当前值
	MSR PSP,R0   //R0所指地址加载到PSP
	MSR PRIMASK,R2 
	BX LR       //PC返回到LR所指处，该函数调用前地址




