#ifndef __DEVICE_H
#define	__DEVICE_H

#include "kernel.h"
//---------------------------------------------------------------------//
struct Device_node
{
	char *name;
	char open_flag;
	char init_flag;
	void (*init)(void);
	void (*open)(void);
	void (*close)(void);
	u32 (*read)(u32 pos,void *buffer,u32 size);
	u32 (*write)(u32 pos,const void *buffer,u32 size);
	void (*control)(u32 cmd, void *arg);	
	void (*tx_complete_callback)(void *buffer,u32 size);
    void (*rx_complete_callback)(void *buffer,u32 size);	
    struct Device_node* next;	
};
typedef struct Device_node* Device_Handle;
typedef struct Device_node  Device_Unit;

//---------------------------------------------------------------------//
typedef enum  
{
	open_state = 0,
	close_state,
}Device_open_state;

typedef enum  
{
	Is_init = 0,
	No_init,
}Device_init_state;

//---------------------------------------------------------------------//
Device_Handle device_find(char *name);
return_flag device_register(Device_Handle device);
return_flag device_unregister(Device_Handle device);
return_flag device_init(Device_Handle device);
return_flag device_open(Device_Handle device);
return_flag device_close(Device_Handle device);
u32 device_read(Device_Handle device,u32 pos,void *buffer,u32 size);
u32 device_write(Device_Handle device,u32 pos,const void *buffer,u32 size);
return_flag device_control(Device_Handle device,u32 cmd, void *arg);
void device_set_rx_indicate(Device_Handle device,void(*rx_ind)(void *buffer,u32 size));
void device_set_tx_complete(Device_Handle device,void(*tx_done)(void *buffer,u32 size));

#endif 
