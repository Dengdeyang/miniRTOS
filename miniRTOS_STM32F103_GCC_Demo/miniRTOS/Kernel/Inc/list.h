#ifndef __LIST_H
#define	__LIST_H

#include "soft_timer.h"
#include "kernel.h"

void printf_List(List_type type);
void List_remove_node(List_type type,Task_Unit *node);
void List_insert(List_type type,Task_Unit *node,List_insert_type insert_type);
void Task_list_init(void);

void printf_Softimer_List(void);
void Softimer_list_remove_node(Softimer_Unit *node);
void Softimer_list_insert(Softimer_Unit *node);
void Softimer_List_init(void);
#endif 
